#include "tm_spatial_sync_groups.h"

#include "Jolt/Physics/Body/BodyID.h"
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/math/vector3.h"
#include "core/object/object.h"
#include "core/templates/local_vector.h"
#include "modules/jolt/j_body_3d.h"
#include "modules/jolt/j_utils.h"
#include "modules/jolt/jolt.h"
#include "modules/network_synchronizer/core/object_data.h"

#include "tm_character_3d.h"
#include "tm_scene_sync.h"
#include "tm_space_object_base.h"
#include "tm_spatial_sync_groups.h"
#include <cstddef>
#include <vector>

void TMSpatialSyncGroups::register_controller(
		int p_peer,
		TMCharacter3D &p_character,
		NS::ObjectData &p_character_nd,
		Vector<NS::ObjectData *> p_other_controlled_nodes) {
	Ref<TMListenerMetadata> meta;
	meta.instantiate();

	meta->peer = p_peer;
	meta->character = &p_character;
	meta->character_nd = &p_character_nd;
	meta->other_controlled_nodes = p_other_controlled_nodes;

	world_listeners.push_back(meta);
}

void TMSpatialSyncGroups::add_new_node_data(NS::ObjectData *p_nd) {
}

void TMSpatialSyncGroups::remove_node_data(NS::ObjectData *p_node_data) {
	for (int i = 0; i < int(world_listeners.size()); i++) {
		if (world_listeners[i].is_valid()) {
			if (world_listeners[i]->character_nd == p_node_data) {
				remove_controller(world_listeners[i]);
				return;
			}
		}
	}
}

void TMSpatialSyncGroups::remove_controller(Ref<TMListenerMetadata> p_controller_meta) {
	ERR_FAIL_COND(p_controller_meta.is_null());
	if (p_controller_meta->sync_group.is_valid()) {
		p_controller_meta->sync_group->listeners.erase(p_controller_meta);
		p_controller_meta->sync_group = Ref<TMSyncGroupMetadata>();
	}
	world_listeners.erase(p_controller_meta);
}

void TMSpatialSyncGroups::on_new_sync_group_created(uint32_t p_id) {
	Ref<TMSyncGroupMetadata> meta;
	meta.instantiate();
	CRASH_COND(!meta.is_valid());

	meta->sync_group_id = p_id;
	const uint32_t meta_index = sync_groups_metadata.size();
	sync_groups_metadata.push_back(meta);
	scene_sync.sync_group_set_user_data(p_id, meta_index);
}

void TMSpatialSyncGroups::update() {
	update_sync_groups_listeners();

	// Updates the sync groups objects now.
	update_sync_groups_location_and_tracked_objects();
}

const TMStateRecorderFilter *TMSpatialSyncGroups::get_sync_group_updated_recorder_filter(const NS::SyncGroup &p_sg) {
	const uint32_t meta_index = p_sg.user_data;
	ERR_FAIL_COND_V_MSG(meta_index >= sync_groups_metadata.size(), nullptr, "[FATAL] The sync group doesn't metadata.");

	Ref<TMSyncGroupMetadata> meta = sync_groups_metadata[meta_index];
	ERR_FAIL_COND_V_MSG(!meta.is_valid(), nullptr, "[FATAL] The sync group doesn't have metadata.");

	if (p_sg.is_realtime_node_list_changed()) {
		meta->recorder_filter.Clear();

		const LocalVector<NS::SyncGroup::SimulatedObjectInfo> &nodes_info = p_sg.get_simulated_sync_objects();
		for (const NS::SyncGroup::SimulatedObjectInfo &info : nodes_info) {
			if (info.od) {
				const Node *nd = scene_sync.scene_synchronizer.from_handle(info.od->app_object_handle);
				const JBody3D *jbody = dynamic_cast<const JBody3D *>(nd);
				if (jbody) {
					meta->recorder_filter.MarkAsSync(jbody->get_jph_body_id());
				}
			}
		}
	}

	return &meta->recorder_filter;
}

void TMSpatialSyncGroups::clear() {
	sync_groups_metadata.clear();
}

Ref<TMSyncGroupMetadata> TMSpatialSyncGroups::get_empty_sync_group() {
	// Search for an empty sync group.
	for (int i = 0; i < int(sync_groups_metadata.size()); i++) {
		if (sync_groups_metadata[i].is_valid()) {
			if (sync_groups_metadata[i]->listeners.is_empty()) {
				return sync_groups_metadata[i];
			}
		}
	}

	return create_empty_sync_group();
}

Ref<TMSyncGroupMetadata> TMSpatialSyncGroups::create_empty_sync_group() {
	const uint32_t id = scene_sync.sync_group_create();
	const uint32_t sg_meta_index = scene_sync.sync_group_get_user_data(id);
	CRASH_COND_MSG(sg_meta_index >= sync_groups_metadata.size(), "This can't be triggered, because the `TMSceneSync` creates the metadata already.");
	CRASH_COND_MSG(sync_groups_metadata[sg_meta_index].is_null(), "This can't be triggered, because the `TMSceneSync` creates the metadata already.");
	return sync_groups_metadata[sg_meta_index];
}

void TMSpatialSyncGroups::assign_controller_to_sync_group(Ref<TMSyncGroupMetadata> p_sg, Ref<TMListenerMetadata> p_controller) {
	if (p_sg->listeners.find(p_controller) == -1) {
		if (p_controller->sync_group.is_valid()) {
			p_controller->sync_group->listeners.erase(p_controller);
		}

		p_sg->listeners.push_back(p_controller);
		p_controller->sync_group = p_sg;

		scene_sync.sync_group_move_peer_to(p_controller->peer, p_sg->sync_group_id);
	}
}

void TMSpatialSyncGroups::update_sync_groups_listeners() {
#ifdef DEBUG_ENABLED
	const bool use_server_camera = ProjectSettings::get_singleton()->get_setting("mirror/use_server_camera", false);
#endif

	// TODO optimize this, in a way that UPDATES the objects, rather than re-creating this from scratch.
	// Group all controllers under a single array first.
	LocalVector<TMListenerMetadata *> ungrouped_listeners;
	ungrouped_listeners.reserve(world_listeners.size());
	for (int c = 0; c < int(world_listeners.size()); c++) {
		Ref<TMListenerMetadata> controller = world_listeners[c];
		if (controller.is_null()) {
			continue;
		}
		ungrouped_listeners.push_back(*controller);
	}

	LocalVector<LocalVector<TMListenerMetadata *>> groups;

	// Untill this array is empty.
	while (ungrouped_listeners.size() > 0) {
		// Put the first controller into this group already.
		LocalVector<TMListenerMetadata *> near_listeners;
		near_listeners.push_back(ungrouped_listeners[0]);

		for (int uc = 1; uc < int(ungrouped_listeners.size()); uc++) {
			TMListenerMetadata *testing_listener = ungrouped_listeners[uc];

			for (int nc = 0; nc < int(near_listeners.size()); nc++) {
				TMListenerMetadata *grouped_controller = near_listeners[nc];

				const Vector3 grouped_controller_origin = grouped_controller->character->get_global_transform().get_origin();
				const Vector3 testing_controller_origin = testing_listener->character->get_global_transform().get_origin();
				const real_t distance = grouped_controller_origin.distance_to(testing_controller_origin);
				if (distance <= scene_sync.character_group_threshold_distance) {
					// Cool, these two controllers are near: put on the same group.
					near_listeners.push_back(testing_listener);
					break;
				}
			}
		}

		// Remove all the grouped controllers from the ungrouped array.
		for (int i = 0; i < int(near_listeners.size()); i++) {
			ungrouped_listeners.erase(near_listeners[i]);
		}

		// Store this group into the groups array.
		groups.push_back(near_listeners);
	}

	// Make sure we have enough sync groups
	while (sync_groups_metadata.size() < groups.size()) {
		create_empty_sync_group();
	}

	// Put these controllers into a SyncGroup
	for (int g = 0; g < int(groups.size()); g++) {
		Ref<TMSyncGroupMetadata> best_sync_group = sync_groups_metadata[g];

		// Make sure these controllers are listening to this sync group now.
		for (int c = 0; c < int(groups[g].size()); c++) {
			TMListenerMetadata *controller = groups[g][c];
			assign_controller_to_sync_group(best_sync_group, controller);

#ifdef DEBUG_ENABLED
			if (use_server_camera) {
				controller->character->update_3d_label(best_sync_group->sync_group_id);
			}
#endif
		}
	}
}

void TMSpatialSyncGroups::update_sync_groups_location_and_tracked_objects() {
	for (int i = 0; i < int(sync_groups_metadata.size()); i++) {
		if (sync_groups_metadata[i].is_null()) {
			continue;
		}
		if (sync_groups_metadata[i]->listeners.is_empty()) {
			// No controllers/clients listening on this SyncGroup.
			continue;
		}

		// Update the group origin
		update_sync_group_fetch_group_origin(i);

		// Now it's time to update the objects tracked by this sync_group
		update_sync_group_tracked_objects(i);
	}
}

void TMSpatialSyncGroups::update_sync_group_fetch_group_origin(int p_group_index) {
	// Fetch the group origin based on the AVG controllers locations
	real_t count = 0.0;
	Vector3 median_point;
	for (int c = 0; c < int(sync_groups_metadata[p_group_index]->listeners.size()); c++) {
		Ref<TMListenerMetadata> controller = sync_groups_metadata[p_group_index]->listeners[c];
		if (controller.is_null()) {
			continue;
		}

		const Vector3 character_origin = controller->character->get_global_transform().origin;
		median_point += character_origin;
		count += 1;
	}

	median_point /= count;
	sync_groups_metadata[p_group_index]->origin = median_point;

	// Fetch the radius that encloses all the characters in this group.
	real_t max_distance = 0.0;
	for (int c = 0; c < int(sync_groups_metadata[p_group_index]->listeners.size()); c++) {
		Ref<TMListenerMetadata> controller = sync_groups_metadata[p_group_index]->listeners[c];
		if (controller.is_null()) {
			continue;
		}

		const Vector3 character_origin = controller->character->get_global_transform().origin;
		const real_t dis = character_origin.distance_to(sync_groups_metadata[p_group_index]->origin);
		max_distance = MAX(max_distance, dis);
	}

	// Expand that radius by the `physics_simulation_thresholt`.
	sync_groups_metadata[p_group_index]->realtime_relevancy_radius = max_distance + scene_sync.physics_simulation_radius;
}

void TMSpatialSyncGroups::update_sync_group_tracked_objects(int p_group_index) {
#ifdef DEBUG_ENABLED
	const bool use_server_camera = ProjectSettings::get_singleton()->get_setting("mirror/use_server_camera", false);
#endif

	const uint64_t now = OS::get_singleton()->get_ticks_msec();

	Ref<TMSyncGroupMetadata> sync_group = sync_groups_metadata[p_group_index];

	// Fetches the bodies into the realtime radius.
	std::vector<JPH::BodyID> bodies_into_simulated_radius;
	Jolt::singleton()->collide_sphere_broad_phase(
			convert(sync_group->origin),
			sync_group->realtime_relevancy_radius,
			bodies_into_simulated_radius);

	LocalVector<NS::SyncGroup::SimulatedObjectInfo> simulated_nodes;
	LocalVector<NS::SyncGroup::TrickledObjectInfo> trickled_nodes;
	simulated_nodes.reserve(bodies_into_simulated_radius.size());
	trickled_nodes.reserve(scene_sync.scene_synchronizer.get_all_object_data().size());

	// Before starting, set the TMSceneSync as simulated. This NODE is always simulated.
	simulated_nodes.push_back(scene_sync.scene_synchronizer.get_object_data(scene_sync.tm_scene_sync_object_local_id));

	for (NS::ObjectData *object_data : scene_sync.scene_synchronizer.get_all_object_data()) {
		if (!object_data) {
			continue;
		}

		Node *node = GdSceneSynchronizer::SyncClass::from_handle(object_data->app_object_handle);
		JBody3D *body = Object::cast_to<JBody3D>(node);
		if (!body || !body->get_jph_body()) {
			continue;
		}

		bool is_simulated;
		uint64_t last_simulation_time;
		bool is_character = Object::cast_to<TMCharacter3D>(body);
		TMSpaceObjectBase *space_object = Object::cast_to<TMSpaceObjectBase>(body);
		{
			if (is_any_player_editing_the_space_object(**sync_group, *body)) {
				NS::VecFunc::insert_at_position_expand(sync_group->into_simulated_radius_timestamp, object_data->get_local_id().id, now, uint64_t(0));
				is_simulated = true;
				last_simulation_time = 0;
			} else {
				const bool into_simulating_radius = std::find(bodies_into_simulated_radius.begin(), bodies_into_simulated_radius.end(), body->get_jph_body()->GetID()) != bodies_into_simulated_radius.end();
				if (space_object && space_object->is_npc()) {
					// Never simulate an NPC. The movement logic can only run on
					// the server (due to a scripting limitation) so NPC are
					// always trickled.
					// NOTE: This prevent many things, like moving platforms, but it's the easiest way to support NPC at the moment.
					// TODO: Once npc are implemented correctly, this should be removed.
					is_simulated = false;
					last_simulation_time = 0;
					is_character = true;
				} else if (into_simulating_radius && (is_character || (!body->is_static() && !body->is_sensor()))) {
					NS::VecFunc::insert_at_position_expand(sync_group->into_simulated_radius_timestamp, object_data->get_local_id().id, now, uint64_t(0));
					is_simulated = true;
					last_simulation_time = 0;
				} else {
					const uint64_t simulated_timestamp = NS::VecFunc::at(sync_group->into_simulated_radius_timestamp, object_data->get_local_id().id, uint64_t(0));
					last_simulation_time = now - simulated_timestamp;
					is_simulated = last_simulation_time < (is_character ? scene_sync.simulated_character_timeout : scene_sync.simulated_timeout);
				}
			}
		}

		real_t update_rate = 0.0;
		real_t size_over_distance = 0.0;
		real_t distance = 0.0;
		if (!is_simulated) {
			bool skip_trickled = true;
			compute_update_rate(
					*body,
					*(*sync_group),
					last_simulation_time,
					is_character,
					skip_trickled,
					update_rate,
					size_over_distance,
					distance);

			if (skip_trickled) {
#ifdef DEBUG_ENABLED
				if (space_object) {
					if (use_server_camera) {
						space_object->update_3d_label(is_simulated, size_over_distance, distance, update_rate);
					}
				}
#endif
				continue;
			}
		}

		if (space_object) {
			if (is_simulated) {
				simulated_nodes.push_back(object_data);
			} else {
				NS::SyncGroup::TrickledObjectInfo dm;
				dm.update_rate = update_rate;
				dm.od = object_data;
				if (object_data->can_trickled_sync()) {
					trickled_nodes.push_back(dm);
				}
			}

#ifdef DEBUG_ENABLED
			if (use_server_camera) {
				space_object->update_3d_label(is_simulated, size_over_distance, distance, update_rate);
			}
#endif

		} else if (is_character) {
			TMCharacter3D *character = static_cast<TMCharacter3D *>(body);
			if (is_simulated) {
				for (NS::ObjectData *od : character->objects_data) {
					simulated_nodes.push_back(od);
				}
			} else {
				trickled_nodes.push_back(character->object_data);
			}
		} else {
			// Not sure what this object is, so skip it.
		}
	}

	scene_sync.sync_group_replace_nodes(
			sync_groups_metadata[p_group_index]->sync_group_id,
			std::move(simulated_nodes),
			std::move(trickled_nodes));
}

bool TMSpatialSyncGroups::is_any_player_editing_the_space_object(TMSyncGroupMetadata &sg, class JBody3D &p_body) const {
	TMSpaceObjectBase *space_object = Object::cast_to<TMSpaceObjectBase>(&p_body);
	if (!space_object) {
		return false;
	}

	for (auto &listener : sg.listeners) {
		const int listener_peer = listener->character->get_multiplayer_authority();
		if (space_object->get_selected_by_peers().has(listener_peer)) {
			return true;
		}
	}

	return false;
}

void TMSpatialSyncGroups::compute_update_rate(
		JBody3D &p_jbody,
		TMSyncGroupMetadata &p_group,
		uint64_t p_last_simulation_time,
		bool p_is_character,
		bool &r_skip_trickled,
		real_t &r_update_rate,
		real_t &r_size_over_distance,
		real_t &r_distance) const {
	r_skip_trickled = false;

	TMSpaceObjectBase *so = Object::cast_to<TMSpaceObjectBase>(&p_jbody);

	if (p_jbody.is_static() || p_jbody.is_sensor()) {
		const bool need_update = so->static_space_object_sync_group_needs_update(p_group);
		if (need_update) {
			r_update_rate = 1.0;
		} else if (so && so->get_selected_by_peers().size() <= 0) {
			r_update_rate = 0.0;
			r_skip_trickled = true;
		}
		return;
	}

	// This body is dynamic, then use the body size over distance to calculate
	// the update rate.

	const real_t distance = p_group.origin.distance_to(p_jbody.get_global_transform().origin);
	r_distance = distance;

	const real_t object_size = p_jbody.get_aabb().GetSize().Length();
	const real_t size_over_distance = object_size / distance; // NOTE, this function is not called for simulated bodies, so for sure the distance is not 0.
	r_size_over_distance = size_over_distance;

	if (size_over_distance <= scene_sync.trickled_size_over_distance_stop_sync) {
		// The object is too small to matter at this point.
		r_skip_trickled = true;
		return;
	} else if (so && so->get_selected_by_peers().size() > 0) {
		// The object is small but we still want to receive update.
		// Though use really slow update rate.
		r_update_rate = scene_sync.trickled_high_precision_rate;
		return;
	} else if (size_over_distance <= scene_sync.trickled_size_over_distance_min) {
		// The object is small but we still want to receive update.
		// Though use really slow update rate.
		r_update_rate = scene_sync.deferred_relevancy_low_rate;
		return;
	} else if (p_jbody.is_sleeping()) {
		r_update_rate = scene_sync.deferred_relevancy_low_rate;
		return;
	} else if (p_last_simulation_time < (p_is_character ? scene_sync.simulated_character_timeout : scene_sync.simulated_timeout) + scene_sync.trickled_high_precision_timeout) {
		// Full update rate.
		r_update_rate = scene_sync.trickled_high_precision_rate;
		return;
	}

	const real_t alpha = (CLAMP(size_over_distance - scene_sync.trickled_size_over_distance_min, 0.0, scene_sync.trickled_size_over_distance_max - scene_sync.trickled_size_over_distance_min) / (scene_sync.trickled_size_over_distance_max - scene_sync.trickled_size_over_distance_min));
	r_update_rate = Math::lerp(scene_sync.deferred_relevancy_min_rate, scene_sync.deferred_relevancy_max_rate, alpha);

	if (p_is_character) {
		// This body is a character, so never go below 0.4
		r_update_rate = MAX(0.4, r_update_rate);
	}
}
