#include "tm_scene_sync.h"

#include "Jolt/Physics/Body/BodyID.h"
#include "core/error/error_macros.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/variant/variant.h"
#include "modules/jolt/j_body_3d.h"
#include "modules/jolt/jolt.h"
#include "modules/network_synchronizer/core/object_data.h"

#include "tm_character_3d.h"
#include "tm_spatial_sync_groups.h"

TMSceneSync *TMSceneSync::the_singleton = nullptr;

TMSceneSync *TMSceneSync::singleton() {
	return the_singleton;
}

#define __CLASS__ TMSceneSync

void TMSceneSync::_bind_methods() {
	ClassDB::bind_method(D_METHOD("fetch_free_unsync_body_id"), &TMSceneSync::fetch_free_unsync_body_id);
	ClassDB::bind_method(D_METHOD("fetch_free_sync_body_id"), &TMSceneSync::fetch_free_sync_body_id);
	ClassDB::bind_method(D_METHOD("release_body_id", "body_id"), &TMSceneSync::release_body_id);

	ClassDB::bind_method(D_METHOD("start_sync", "world_context"), &TMSceneSync::start_sync);

	ClassDB::bind_method(D_METHOD("register_controller", "peer", "node_composing_the_controller"), &TMSceneSync::register_controller);
}

#undef __CLASS__

TMSceneSync::TMSceneSync() :
		GdSceneSynchronizer(),
		body_id_manager(500),
		spatial_sync_groups(*this) {
	if (the_singleton != nullptr) {
		CRASH_NOW_MSG("You can't have two TMSceneSync singletons at the same time.");
	}

	the_singleton = this;
	set_frame_confirmation_timespan(1.0 / 5.0);
	scene_synchronizer.set_frames_per_seconds(30);
	scene_synchronizer.set_max_predicted_intervals(5);
	set_max_trickled_nodes_per_update(5);

	Jolt::singleton()->set_body_id_manager(&body_id_manager);
}

TMSceneSync::~TMSceneSync() {
	if (the_singleton != this) {
		return;
	}

	the_singleton = nullptr;
	Jolt::singleton()->set_body_id_manager(nullptr);
}

bool TMSceneSync::snapshot_get_custom_data(const NS::SyncGroup *p_group, NS::VarData &r_custom_data) {
	NS_PROFILE

	TMStateRecorderFilter const *filter = nullptr;
	if (is_server()) {
		// On the server this is ALWAY not nullptr.
		CRASH_COND(p_group == nullptr);

		// GLOBAL SYNC GROUP: nothing to filter.
		if (sync_group_get(GdSceneSynchronizer::GLOBAL_SYNC_GROUP_ID) != p_group) {
			filter = spatial_sync_groups.get_sync_group_updated_recorder_filter(*p_group);
			CRASH_COND(filter == nullptr);
		}

	} else if (is_client()) {
		NS_PROFILE_NAMED("Setup the filter.")
		client_jolt_recorder_filter.Clear();
		for (auto net_id : *scene_synchronizer.client_get_simulated_objects()) {
			const NS::ObjectHandle handle = scene_synchronizer.get_app_object_from_id_const(net_id, false);
			if (handle != NS::ObjectHandle::NONE) {
				const Node *node = scene_synchronizer.from_handle(handle);
				const JBody3D *body = Object::cast_to<const JBody3D>(node);
				if (body) {
					client_jolt_recorder_filter.MarkAsSync(body->get_jph_body_id());
				}
			}
		}
		filter = &client_jolt_recorder_filter;
	} else {
		filter = &client_jolt_recorder_filter;
	}

	{
		NS_PROFILE_NAMED("Fetch the data from Jolt.")
		Jolt::singleton()->world_get_state(
				0,
				jolt_state,
				JPH::EStateRecorderState::Contacts,
				filter);
	}

	{
		NS_PROFILE_NAMED("Converts the Jolt Buffer to a VarData.")

		Vector<uint8_t> js(jolt_state.get_data());
		CRASH_COND(js.size() <= 0);
		Variant v_js(js);
		CRASH_COND(v_js.get_type() != Variant::PACKED_BYTE_ARRAY);
		GdSceneSynchronizer::convert(r_custom_data, v_js);
	}

	return true;
}

void TMSceneSync::snapshot_set_custom_data(const NS::VarData &p_custom_data) {
	// This is always executed on the client, there is no way to trigger this
	// otherwise.
	CRASH_COND(!is_client());

	Variant v_js;
	GdSceneSynchronizer::convert(v_js, p_custom_data);
	// The parser fails if this is different.
	CRASH_COND(v_js.get_type() != Variant::PACKED_BYTE_ARRAY);

	if (is_resetted() || is_recovered() || is_rewinding() || is_end_sync()) {
		jolt_state.set_data(v_js);
		jolt_state.BeginRead();
	}
}

int TMSceneSync::fetch_free_unsync_body_id() {
	return body_id_manager.fetch_free_unsync_body_id().GetIndex();
}

int TMSceneSync::fetch_free_sync_body_id() {
	return body_id_manager.fetch_free_sync_body_id().GetIndex();
}

void TMSceneSync::release_body_id(int p_id) {
	body_id_manager.release_body_id(JPH::BodyID(p_id));
}

void TMSceneSync::start_sync(Object *p_world_context) {
	stop_sync();

	Node *wc = Object::cast_to<Node>(p_world_context);
	if (wc) {
		set_name("TMSceneSync");
		wc->add_child(this);
		wc->connect("tree_exiting", callable_mp(this, &TMSceneSync::stop_sync));
	}

	reset_synchronizer_mode();
}

void TMSceneSync::stop_sync() {
	clear();

	// Remove from parent.
	if (get_parent()) {
		get_parent()->disconnect("tree_exiting", callable_mp(this, &TMSceneSync::stop_sync));
		get_parent()->remove_child(this);
	}
}

void TMSceneSync::process_jolt_physics(double p_delta) {
	if (jolt_state.get_data().size() > 0) {
		// There is a pending state.
		// NOTE: This is applied here, instead of `snapshot_set_custom_data`,
		// because at this point all the object's state is set and we can safely
		// set the contacts for the active bodies.
		Jolt::singleton()->world_set_state(0, jolt_state, JPH::EStateRecorderState::Contacts);
		jolt_state.set_data(Vector<uint8_t>());
	}

	Jolt::singleton()->world_process(0, p_delta);
	// Sync the physics transforms with godot transform right away at the end of
	// the processing.
	Jolt::singleton()->world_sync(0);
}

void TMSceneSync::register_controller(int p_peer, Vector<Variant> p_controller_nodes) {
	TMCharacter3D *character = nullptr;
	NS::ObjectData *character_od = nullptr;

	Vector<NS::ObjectData *> other_controlled_nodes;
	Vector<NS::ObjectData *> all_controlled_nodes;

	for (const Variant &v : p_controller_nodes) {
		Object *o = v;
		Node *n = Object::cast_to<Node>(o);
		ERR_CONTINUE_MSG(n == nullptr, "Please pass only `Node`s to `register_controller`.");

		NS::ObjectLocalId lid = register_node(n);
		NS::ObjectData *od = scene_synchronizer.get_object_data(lid);

		all_controlled_nodes.push_back(od);

		if (!character) {
			character = Object::cast_to<TMCharacter3D>(n);
			if (character) {
				character_od = od;
				continue;
			}
		}

		other_controlled_nodes.push_back(od);
	}

	CRASH_COND(character == nullptr);
	CRASH_COND(character_od == nullptr);

	spatial_sync_groups.register_controller(
			p_peer,
			*character,
			*character_od,
			other_controlled_nodes);

	character->object_data = character_od;
	character->objects_data = all_controlled_nodes;
}

void TMSceneSync::on_init_synchronizer(bool p_was_generating_ids) {
	GdSceneSynchronizer::on_init_synchronizer(p_was_generating_ids);

	scene_synchronizer.register_app_object(scene_synchronizer.to_handle(this), &tm_scene_sync_object_local_id);
	CRASH_COND(tm_scene_sync_object_local_id == NS::ObjectLocalId::NONE); // At this point, this is never expected to be `NONE`.

	handler_jolt_process =
			scene_synchronizer.register_process(tm_scene_sync_object_local_id, PROCESS_PHASE_POST, [this](float p_delta) {
				process_jolt_physics(p_delta);
			});
}

void TMSceneSync::on_uninit_synchronizer() {
	const NS::ObjectLocalId id = scene_synchronizer.find_object_local_id(scene_synchronizer.to_handle(this));
	if (id != NS::ObjectLocalId::NONE) {
		scene_synchronizer.unregister_process(id, PROCESS_PHASE_POST, handler_jolt_process);
	}
	handler_jolt_process = NS::NullPHandler;

	spatial_sync_groups.clear();

	GdSceneSynchronizer::on_uninit_synchronizer();
}

void TMSceneSync::on_add_object_data(NS::ObjectData &p_node_data) {
	spatial_sync_groups.add_new_node_data(&p_node_data);
}

void TMSceneSync::on_drop_object_data(NS::ObjectData &p_node_data) {
	spatial_sync_groups.remove_node_data(&p_node_data);
}

void TMSceneSync::update_objects_relevancy() {
	spatial_sync_groups.update();
	GdSceneSynchronizer::update_objects_relevancy();
}

void TMSceneSync::on_sync_group_created(NS::SyncGroupId p_group_id) {
	spatial_sync_groups.on_new_sync_group_created(p_group_id.id);
}
