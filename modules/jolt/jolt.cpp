#include "jolt.h"

#include "Jolt/Physics/Body/BodyType.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Collision/Shape/ScaledShape.h"
#include "Jolt/Physics/EActivation.h"
#include "Jolt/Physics/StateRecorder.h"
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/math/math_defs.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/string/print_string.h"
#include "core/templates/local_vector.h"
#include "core/variant/dictionary.h"
#include "core/variant/variant.h"
#include "j_body_3d.h"
#include "j_body_id_manager.h"
#include "j_utils.h"
#include "modules/jolt/j_contact_listener.h"
#include "modules/jolt/j_query_results.h"
#include "modules/jolt/table_broad_phase_layer.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Core/JobSystemThreadPool.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Core/TempAllocator.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Math/DVec3.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Math/Quat.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Math/Real.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Math/Vec3.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Body/Body.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Body/BodyCreationSettings.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Body/BodyID.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Body/BodyInterface.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Character/CharacterVirtual.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Collision/CastResult.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Collision/Shape/BoxShape.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Collision/ShapeCast.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/PhysicsSystem.h"
#include "modules/network_synchronizer/core/net_utilities.h"
#include "modules/network_synchronizer/core/scene_synchronizer_debugger.h"
#include "object_layers_filter.h"
#include "scene/resources/particle_process_material.h"
#include "scene/resources/physics_material.h"
#include "state_recorder_sync.h"

Jolt *Jolt::the_singleton = nullptr;

Jolt *Jolt::singleton() {
	return the_singleton;
}

void Jolt::_bind_methods() {
	ClassDB::bind_method(D_METHOD("world_process", "world_id", "delta"), &Jolt::world_process);
	ClassDB::bind_method(D_METHOD("cast_ray", "world_id", "from", "direction", "distance", "layers_name", "ignore_bodies"), &Jolt::cast_ray_gd, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("cast_shape", "world_id", "shape", "from", "move", "collide_with_layers", "ignore_bodies"), &Jolt::cast_shape_gd, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("collide_shape", "world_id", "shape", "from", "collide_with_layers", "ignore_bodies"), &Jolt::collide_shape_gd, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("high_level_ray_or_shape_cast", "from", "direction", "length", "sphere_radius", "collide_with_layers", "ignore_bodies"), &Jolt::high_level_ray_or_shape_cast, DEFVAL(Array()), DEFVAL(Array()));

	ClassDB::bind_method(D_METHOD("set_layers_table", "layers_table"), &Jolt::set_layers_table);
	ClassDB::bind_method(D_METHOD("get_layers_table"), &Jolt::get_layers_table);
}

Jolt::Jolt() :
		Object() {
	if (the_singleton != nullptr) {
		CRASH_NOW_MSG("You can't have two Jolt singletons at the same time.");
	}

	the_singleton = this;

	// Init the layers table with the default one.
	set_layers_table(Ref<JLayersTable>());
}

void Jolt::__init() {
	const uint32_t default_world_index = create_world(
			ProjectSettings::get_singleton()->get_setting("jolt/main_world_config/max_bodies", 10'000),
			ProjectSettings::get_singleton()->get_setting("jolt/main_world_config/num_body_mutex", 0),
			ProjectSettings::get_singleton()->get_setting("jolt/main_world_config/max_body_pairs", 10'000),
			ProjectSettings::get_singleton()->get_setting("jolt/main_world_config/max_contact_constraints", 10'000));

	CRASH_COND_MSG(default_world_index != 0, "The default world is always set to 0.");
}

void Jolt::set_body_id_manager(class JBodyIdManager *p_manager) {
	body_id_manager = p_manager;
}

class JBodyIdManager *Jolt::get_body_id_manager() const {
	return body_id_manager;
}

Jolt::~Jolt() {
	if (the_singleton != this) {
		return;
	}

	the_singleton = nullptr;

	for (int i = worlds.size() - 1; i >= 0; i--) {
		if (worlds[i] != nullptr) {
			__destroy_world(i);
		}
	}
}

void Jolt::set_layers_table(const Ref<JLayersTable> &p_layers_table) {
	layers_table = p_layers_table;

	if (layers_table.is_null()) {
		layers_table.instantiate();
	}

	CRASH_COND(layers_table.is_null());

	object_layer_pair_filter.table = *layers_table;
	object_vs_broad_phase_layer_filter.table = *layers_table;
	broad_phase_layer.table = *layers_table;
}

Ref<JLayersTable> Jolt::get_layers_table() const {
	return layers_table;
}

String Jolt::get_layers_comma_separated() const {
	// This is never null.
	CRASH_COND(layers_table.is_null());

	String s = "";
	for (uint32_t i = 0; i < layers_table->layers_name.size(); i++) {
		s = s + String(layers_table->layers_name[i]) + ",";
	}
	return s;
}

uint32_t Jolt::create_world(uint32_t p_max_bodies, uint32_t p_num_body_mutexes, uint32_t p_max_body_pairs, uint32_t p_max_contact_constraints) {
	JPH::PhysicsSystem *physics_system = new JPH::PhysicsSystem();

	// Put the world into an empty space.
	uint32_t index = UINT32_MAX;
	for (uint32_t i = 0; i < worlds.size(); i++) {
		if (worlds[i] == nullptr) {
			worlds[i] = physics_system;
			index = i;
			break;
		}
	}

	if (index == UINT32_MAX) {
		// No empty space, create a new one.
		index = worlds.size();
		worlds.push_back(physics_system);

		contact_listeners.push_back(nullptr);

		// We need a temp allocator for temporary allocations during the physics update. We're
		// pre-allocating 10 MB to avoid having to do allocations during the physics update.
		// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
		// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
		// malloc / free.
		JPH::TempAllocatorImpl *temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
		worlds_temp_allocators.push_back(temp_allocator);

		// We need a job system that will execute physics jobs on multiple threads. Typically
		// you would implement the JobSystem interface yourself and let Jolt Physics run on top
		// of your own job scheduler. JobSystemThreadPool is an example implementation.
		JPH::JobSystemThreadPool *job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);
		worlds_job_systems.push_back(job_system);
	}

	// Initialize the World.
	max_bodies = closest_power_of_2(p_max_bodies);
	physics_system->Init(
			max_bodies,
			closest_power_of_2(CLAMP(p_num_body_mutexes, uint32_t(0), uint32_t(64))),
			closest_power_of_2(p_max_body_pairs),
			closest_power_of_2(p_max_contact_constraints),
			broad_phase_layer,
			object_vs_broad_phase_layer_filter,
			object_layer_pair_filter);

	JContactListener *listener = new JContactListener();
	physics_system->SetContactListener(listener);
	contact_listeners[index] = listener;
	NS::VecFunc::insert_at_position_expand(worlds_need_broadphase_opt, index, true, false);

	return index;
}

void Jolt::world_destroy(uint32_t p_index) {
	if (p_index > 0 && p_index < worlds.size()) {
		__destroy_world(p_index);
	}
}

JContactListener *Jolt::get_contact_listener(uint32_t p_world_id) {
	ERR_FAIL_COND_V(p_world_id >= contact_listeners.size(), nullptr);
	return contact_listeners[p_world_id];
}

const JContactListener *Jolt::get_contact_listener(uint32_t p_world_id) const {
	ERR_FAIL_COND_V(p_world_id >= contact_listeners.size(), nullptr);
	return contact_listeners[p_world_id];
}

void Jolt::__destroy_world(uint32_t p_index) {
	delete worlds[p_index];
	worlds[p_index] = nullptr;

	delete contact_listeners[p_index];
	contact_listeners[p_index] = nullptr;

	delete worlds_temp_allocators[p_index];
	worlds_temp_allocators[p_index] = nullptr;

	delete worlds_job_systems[p_index];
	worlds_job_systems[p_index] = nullptr;
}

JPH::ObjectLayer Jolt::get_layer_from_layer_name(const StringName &p_layer) const {
	return layers_table->get_layer(p_layer);
}

void Jolt::world_register_sensor(JBody3D *p_sensor) {
#ifdef DEBUG_ENABLED
	// This function MUST never be called if the body is not marked as sensor.
	CRASH_COND(!p_sensor->is_sensor());
#endif
	if (sensors_list.find(p_sensor) < 0) {
		sensors_list.push_back(p_sensor);
	}
}

void Jolt::world_unregister_sensor(JBody3D *p_sensor) {
	const int index = sensors_list.find(p_sensor);
	if (index >= 0) {
		sensors_list.remove_at_unordered(index);
	}
}

void Jolt::world_process(uint32_t p_world_id, double p_delta) {
	JPH::PhysicsSystem *physics_system = worlds[p_world_id];

	if (worlds_need_broadphase_opt[p_world_id]) {
		worlds_need_broadphase_opt[p_world_id] = false;
		physics_system->OptimizeBroadPhase();
	}

	// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
	const int cCollisionSteps = 1;

	physics_system->Update(
			p_delta,
			cCollisionSteps,
			worlds_temp_allocators[p_world_id],
			worlds_job_systems[p_world_id]);
}

void Jolt::world_sync(uint32_t p_world_id, bool p_force_all) {
	JPH::PhysicsSystem *physics_system = worlds[p_world_id];

	// TODO refactor this, so to fetch the raw and unsafe vector of Active Bodies IDs
	// No need to waste time with locking at this point...
	// BodyManager::GetActiveBodiesUnsafe()
	const JPH::BodyInterface &body_interface = physics_system->GetBodyInterfaceNoLock();

	if (p_force_all) {
		JPH::BodyIDVector out_body_ids;
		physics_system->GetBodies(out_body_ids);
		for (JPH::BodyID id : out_body_ids) {
			JBody3D *body = reinterpret_cast<JBody3D *>(body_interface.GetUserData(id));
			if (body) {
				body->__update_godot_transform();
			}
		}
	} else {
		const uint32_t count = physics_system->GetNumActiveBodies(JPH::EBodyType::RigidBody);
		const JPH::BodyID *ids = physics_system->GetActiveBodiesUnsafe(JPH::EBodyType::RigidBody);
		for (uint32_t i = 0; i < count; i++) {
			JBody3D *body = reinterpret_cast<JBody3D *>(body_interface.GetUserData(ids[i]));
			if (body) {
				body->__update_godot_transform();
			}
		}
	}

	world_sync_overlap_events();
}

void Jolt::world_sync_overlap_events() {
	for (JBody3D *s : sensors_list) {
#ifdef DEBUG_ENABLED
		// When the body is not a sensor, it's never contained by this list.
		CRASH_COND(!s->is_sensor());
#endif
		// Emit the overlap events.
		s->sensor_notify_overlap_events();
	}
}

bool Jolt::world_is_processing(uint32_t p_world_id) const {
	// At the moment we are using `Sync` processing.
	return false;
}

void Jolt::world_get_state(uint32_t p_world_id, class StateRecorderSync &p_state, JPH::EStateRecorderState p_recorder_state, const JPH::StateRecorderFilter *p_filter) {
	JPH::PhysicsSystem *physics_system = worlds[p_world_id];

	CRASH_COND_MSG(p_state.IsFailed(), "The state shound not be failed at this point.");

	p_state.Clear();
	{
		physics_system->SaveState(p_state, p_recorder_state, p_filter);
	}

	CRASH_COND_MSG(p_state.IsFailed(), "The state shound not be failed at this point.");

#ifdef DEBUG_ENABLED
	if (p_filter == nullptr && p_recorder_state == JPH::EStateRecorderState::All) {
		// We can validate the data right away, when the `filter` is disabled.
		// NOTE: With filtering, the Validation can't be used.
		p_state.BeginRead();
		p_state.SetValidating(true);
		physics_system->RestoreState(p_state);

		CRASH_COND_MSG(p_state.IsFailed(), "The state failed.");

		p_state.SetValidating(false);
	}
#endif
}

void Jolt::world_set_state(uint32_t p_world_id, class StateRecorderSync &p_state, JPH::EStateRecorderState p_recorder_state) {
	JPH::PhysicsSystem *physics_system = worlds[p_world_id];

	p_state.BeginRead();
	p_state.SetValidating(false);
	const bool success = physics_system->RestoreState(p_state);
	if (!success) {
		SceneSynchronizerDebugger::singleton()->print(NS::ERROR, "world_set_state failed.");
	}

	if (!p_state.IsEOF()) {
		SceneSynchronizerDebugger::singleton()->print(NS::ERROR, "world_set_state failed because the state is not yet fully read.");
	}

	CRASH_COND_MSG(p_state.IsFailed(), "The state shound not be failed at this point.");

	/*
	This is not supposed at the moment with partial reading state..

	#ifdef DEBUG_ENABLED
		// Validate the data.
		p_state.BeginRead();
		p_state.SetValidating(true);
		physics_system->RestoreState(p_state);

		CRASH_COND_MSG(p_state.IsFailed(), "The state failed.");

		StateRecorderSync current_state;
		physics_system->SaveState(current_state);

		CRASH_COND_MSG(!current_state.IsEqual(p_state), "The states should be the same at this point.");
	#endif
	*/
}

JPH::Body *Jolt::create_body(
		JBody3D *p_owner,
		uint32_t p_world_id,
		JPH::BodyID p_desired_body_id,
		JPH::ObjectLayer p_layer_id,
		JPH::ShapeRefC p_shape,
		const Transform3D &p_body_initial_transform,
		real_t p_friction,
		real_t p_bounciness,
		real_t p_mass,
		bool p_is_sensor,
		bool p_kinematic_detect_static,
		JPH::EMotionType p_motion_type,
		bool p_allow_sleeping,
		bool p_report_precise_touch_location,
		real_t p_linear_damping,
		real_t p_angular_damping,
		real_t p_max_linear_velocity,
		real_t p_max_angular_velocity_deg,
		real_t p_gravity_scale,
		bool p_use_ccd) {
	JPH::RVec3 origin = convert_r(p_body_initial_transform.get_origin());
	JPH::Quat rot = convert(p_body_initial_transform.get_basis().get_rotation_quaternion());
	JPH::Vec3 scale = convert(p_body_initial_transform.get_basis().get_scale_abs());

	p_shape = get_scaled_shape(p_shape, scale);
	p_owner->set_scaled_shape(p_shape, scale);

	JPH::BodyCreationSettings settings(
			p_shape,
			origin,
			rot,
			p_motion_type,
			p_layer_id);

	settings.mUserData = reinterpret_cast<JPH::uint64>(p_owner);
	settings.mAllowDynamicOrKinematic = true;
	settings.mAllowSleeping = p_allow_sleeping;
	settings.mFriction = p_friction;
	settings.mRestitution = p_bounciness;
	settings.mIsSensor = p_is_sensor;
	settings.mCollideKinematicVsNonDynamic = p_kinematic_detect_static;
	settings.mUseManifoldReduction = !p_report_precise_touch_location;
	settings.mLinearDamping = p_linear_damping;
	settings.mAngularDamping = p_angular_damping;
	settings.mMaxLinearVelocity = p_max_linear_velocity;
	settings.mMaxAngularVelocity = Math::deg_to_rad(p_max_angular_velocity_deg);
	settings.mGravityFactor = p_gravity_scale;
	settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
	settings.mMassPropertiesOverride.mMass = p_mass;
	settings.mMotionQuality = p_use_ccd ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;

	JPH::PhysicsSystem *physics_system = worlds[p_world_id];

	JPH::BodyInterface &body_interface = physics_system->GetBodyInterface();
	JPH::Body *body = nullptr;
	if (p_desired_body_id.IsInvalid()) {
		if (body_id_manager) {
			body = body_interface.CreateBodyWithID(body_id_manager->fetch_free_body_id(), settings);
		} else {
			body = body_interface.CreateBody(settings);
		}
	} else {
		body = body_interface.CreateBodyWithID(p_desired_body_id, settings);
		ERR_FAIL_COND_V_MSG(body == nullptr, nullptr, "The desired_body_id `" + itos(p_desired_body_id.GetIndexAndSequenceNumber()) + "` is already in use. The node requesting the body_id is: `" + p_owner->get_path() + "`.");
	}
	CRASH_COND(body == nullptr);
	CRASH_COND(body->GetID().IsInvalid());

	if (body_id_manager) {
		body_id_manager->on_body_created(body->GetID());
	}

	// Add the Body into the World.
	const JPH::EActivation activation =
			p_motion_type == JPH::EMotionType::Dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;
	body_interface.AddBody(body->GetID(), activation);

	worlds_need_broadphase_opt[p_world_id] = true;

	return body;
}

void Jolt::body_destroy(JPH::Body *p_body, bool p_wakesup_near_objects) {
	int world_id = 0;
	JPH::PhysicsSystem *physics_system = worlds[world_id];

	const JPH::AABox aabox = p_body->GetWorldSpaceBounds();
	const JPH::BodyID body_id = p_body->GetID();

	JPH::BodyInterface &b_interface = physics_system->GetBodyInterface();
	b_interface.RemoveBody(body_id);
	b_interface.DestroyBody(body_id);

	if (body_id_manager) {
		body_id_manager->on_body_destroyed(body_id);
	}

	if (p_wakesup_near_objects) {
		// Wake up near bodies, Jolt doesn't do that automatically.
		// Check this for more info about it: https://github.com/jrouwe/JoltPhysics/discussions/270
		std::vector<JPH::BodyID> overlapped_bodies;
		BroadphaseCollector bpc(overlapped_bodies);
		physics_system->GetBroadPhaseQuery().CollideAABox(aabox, bpc);

		std::vector<JPH::BodyID> overlapped_dynamic_bodies;
		overlapped_dynamic_bodies.reserve(overlapped_bodies.size());

		for (auto &ob : overlapped_bodies) {
			JPH::Body *b = physics_system->GetBodyLockInterfaceNoLock().TryGetBody(ob);
			if (b && b->GetMotionType() == JPH::EMotionType::Dynamic && b->GetMotionPropertiesUnchecked()) {
				NS::VecFunc::insert_unique(overlapped_dynamic_bodies, ob);
			}
		}

		b_interface.ActivateBodies(overlapped_dynamic_bodies.data(), overlapped_dynamic_bodies.size());
	}

	worlds_need_broadphase_opt[world_id] = true;
}

void Jolt::notify_body_3d_destroy(class JBody3D *p_body) {
	if (body_id_manager && p_body) {
		body_id_manager->on_body_3d_destroyed(*p_body);
	}
}

void Jolt::body_save_state(JPH::Body &p_body, StateRecorderSync &r_jolt_state) {
	JPH::PhysicsSystem *physics_system = worlds[0];
	physics_system->SaveBodyState(p_body, r_jolt_state);
}

void Jolt::body_restore_state(JPH::Body &p_body, StateRecorderSync &p_jolt_state) {
	JPH::PhysicsSystem *physics_system = worlds[0];
	physics_system->RestoreBodyState(p_body, p_jolt_state);
}

JBody3D *Jolt::body_fetch(JPH::BodyID p_body_id) {
	JPH::PhysicsSystem *physics_system = worlds[0];
	JPH::Body *b = physics_system->GetBodyLockInterface().TryGetBody(p_body_id);
	if (b) {
		return reinterpret_cast<JBody3D *>(b->GetUserData());
	} else {
		return nullptr;
	}
}

JPH::Body *Jolt::body_jolt_fetch(JPH::BodyID p_body_id) {
	JPH::PhysicsSystem *physics_system = worlds[0];
	return physics_system->GetBodyLockInterface().TryGetBody(p_body_id);
}

void Jolt::body_set_use_CCD(JPH::BodyID p_body_id, bool p_use_ccd) {
	JPH::PhysicsSystem *physics_system = worlds[0];

	JPH::BodyInterface &b_interface = physics_system->GetBodyInterface();
	b_interface.SetMotionQuality(
			p_body_id,
			p_use_ccd ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete);
}

void Jolt::body_activate(JPH::BodyID p_body_id) {
	JPH::PhysicsSystem *physics_system = worlds[0];

	JPH::BodyInterface &b_interface = physics_system->GetBodyInterface();
	b_interface.ActivateBody(p_body_id);
}

void Jolt::body_deactivate(JPH::BodyID p_body_id) {
	JPH::PhysicsSystem *physics_system = worlds[0];

	JPH::BodyInterface &b_interface = physics_system->GetBodyInterface();
	b_interface.DeactivateBody(p_body_id);
}

void Jolt::body_set_shape(JBody3D *p_body, JPH::ShapeRefC p_shape, bool p_activate) {
	JPH::PhysicsSystem *physics_system = worlds[0];

	JPH::EActivation activation;
	if (p_activate) {
		activation = JPH::EActivation::Activate;
	} else {
		activation = JPH::EActivation::DontActivate;
	}

	JPH::BodyInterface &b_interface = physics_system->GetBodyInterface();
	p_shape = get_scaled_shape(p_shape, p_body->get_applied_scale());
	p_body->set_scaled_shape(p_shape, p_body->get_applied_scale());
	b_interface.SetShape(
			p_body->get_jph_body_id(),
			p_shape,
			true,
			activation);
}

void Jolt::body_set_transform(JBody3D *p_body, const Transform3D &p_transform) {
	if (!p_body->get_jph_body()) {
		return;
	}

	JPH::PhysicsSystem *physics_system = worlds[0];

	JPH::BodyInterface &b_interface = physics_system->GetBodyInterface();

	JPH::EActivation activation = JPH::EActivation::Activate;
	if (p_body->get_jph_body()->IsDynamic()) {
		activation = JPH::EActivation::Activate;
	} else {
		activation = JPH::EActivation::DontActivate;
	}

	const JPH::AABox pre_transform_aabox = p_body->get_jph_body()->GetWorldSpaceBounds();
	const JPH::RVec3 o = convert_r(p_transform.get_origin());
	const JPH::Quat q = convert(p_transform.get_basis().get_rotation_quaternion());
	const JPH::Vec3 s = convert(p_transform.get_basis().get_scale_abs());

	const bool is_great_move =
			!p_body->get_jph_body()->GetWorldTransform().GetTranslation().IsClose(o) ||
			!s.IsClose(p_body->get_applied_scale()) ||
			!p_body->get_jph_body()->GetWorldTransform().GetQuaternion().IsClose(q);

	b_interface.SetPositionAndRotation(
			p_body->get_jph_body_id(),
			o,
			q,
			activation);

	Ref<JShape3D> shape = p_body->get_shape();
	if (!s.IsClose(p_body->get_applied_scale())) {
		if (shape.is_valid() && shape->get_shape()) {
			JPH::ShapeRefC scaled_shape = get_scaled_shape(shape->get_shape(), s);
			p_body->set_scaled_shape(scaled_shape, s);

			b_interface.SetShape(
					p_body->get_jph_body_id(),
					scaled_shape,
					true,
					activation);
		}
	}

	if (is_great_move) {
		// Wake up near bodies, Jolt doesn't do that automatically.
		// Check this for more info about it: https://github.com/jrouwe/JoltPhysics/discussions/270
		std::vector<JPH::BodyID> overlapped_bodies;
		BroadphaseCollector bpc(overlapped_bodies);
		physics_system->GetBroadPhaseQuery().CollideAABox(
				pre_transform_aabox,
				bpc);
		physics_system->GetBroadPhaseQuery().CollideAABox(
				p_body->get_jph_body()->GetWorldSpaceBounds(),
				bpc);
		std::vector<JPH::BodyID> overlapped_dynamic_bodies;
		overlapped_dynamic_bodies.reserve(overlapped_bodies.size());
		for (auto &ob : overlapped_bodies) {
			JPH::Body *b = physics_system->GetBodyLockInterfaceNoLock().TryGetBody(ob);
			if (b && b->GetMotionType() == JPH::EMotionType::Dynamic && b->GetMotionPropertiesUnchecked()) {
				NS::VecFunc::insert_unique(overlapped_dynamic_bodies, ob);
			}
		}
		b_interface.ActivateBodies(overlapped_dynamic_bodies.data(), overlapped_dynamic_bodies.size());
	}
}

Transform3D Jolt::body_get_transform(JBody3D *p_body) const {
	const JPH::RVec3 pos = p_body->get_jph_body()->GetPosition();
	const JPH::Quat rot = p_body->get_jph_body()->GetRotation();
	const JPH::Vec3 scale = p_body->get_applied_scale();

	return Transform3D(Basis(Quaternion(convert(rot))).scaled(convert(scale)), convert_r(pos));
}

void Jolt::body_set_motion_type(JPH::BodyID p_body_id, JPH::EMotionType type) {
	JPH::PhysicsSystem *physics_system = worlds[0];

	JPH::BodyInterface &b_interface = physics_system->GetBodyInterface();

	b_interface.SetMotionType(
			p_body_id,
			type,
			type == JPH::EMotionType::Dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
}

void Jolt::body_set_layer(JPH::BodyID p_body_id, JPH::ObjectLayer p_layer_id) {
	JPH::PhysicsSystem *physics_system = worlds[0];

	JPH::BodyInterface &b_interface = physics_system->GetBodyInterface();
	b_interface.SetObjectLayer(p_body_id, p_layer_id);
}

Dictionary Jolt::cast_ray_gd(
		uint32_t p_world_id,
		const Vector3 &p_from,
		const Vector3 &p_direction,
		real_t p_distance,
		const Array &p_collide_with_layers,
		const Array &p_ignore_bodies) const {
	JRayQuerySingleResult result;
	result.ray.mOrigin = convert_r(p_from);
	result.ray.mDirection = convert(p_direction * p_distance);

	LocalVector<JPH::ObjectLayer> layers;
	for (int i = 0; i < p_collide_with_layers.size(); i++) {
		const JPH::ObjectLayer layer = get_layer_from_layer_name(p_collide_with_layers[i]);
		layers.push_back(layer);
	}

	LocalVector<uint32_t> ignore_bodies;
	for (int i = 0; i < p_ignore_bodies.size(); i++) {
		if (p_ignore_bodies[i].get_type() == Variant::INT) {
			ignore_bodies.push_back(p_ignore_bodies[i]);
		} else if (p_ignore_bodies[i].get_type() == Variant::OBJECT) {
			Object *o = p_ignore_bodies[i];
			JBody3D *b = Object::cast_to<JBody3D>(o);
			if (b) {
				ignore_bodies.push_back(b->get_body_id());
			}
		}
	}

	cast_ray(
			p_world_id,
			convert_r(p_from),
			convert(p_direction),
			p_distance,
			layers,
			result,
			ignore_bodies);

	Dictionary out;

	if (result.has_hit()) {
		JBody3D *body = Jolt::singleton()->body_fetch(result.hit.mBodyID);
		out["has_hit"] = true;
		out["collider"] = body;
		out["fraction"] = result.hit.mFraction;
		out["shape_id"] = result.hit.mSubShapeID2.GetValue();
		out["normal"] = convert(result.normal);
		out["position"] = convert_r(result.location);
	} else {
		out["has_hit"] = false;
	}

	return out;
}

Array Jolt::cast_shape_gd(
		uint32_t p_world_id,
		Ref<JShape3D> p_shape,
		const Transform3D &p_from,
		const Vector3 &p_move,
		const Array &p_collide_with_layers,
		const Array &p_ignore_bodies) const {
	JQueryVectorResult<JPH::CastShapeCollector> result;

	LocalVector<JPH::ObjectLayer> layers;
	for (int i = 0; i < p_collide_with_layers.size(); i++) {
		const JPH::ObjectLayer layer = get_layer_from_layer_name(p_collide_with_layers[i]);
		layers.push_back(layer);
	}

	LocalVector<uint32_t> ignore_bodies;
	for (int i = 0; i < p_ignore_bodies.size(); i++) {
		if (p_ignore_bodies[i].get_type() == Variant::INT) {
			ignore_bodies.push_back(p_ignore_bodies[i]);
		} else if (p_ignore_bodies[i].get_type() == Variant::OBJECT) {
			Object *o = p_ignore_bodies[i];
			JBody3D *b = Object::cast_to<JBody3D>(o);
			if (b) {
				ignore_bodies.push_back(b->get_body_id());
			}
		}
	}

	JPH::ShapeCastSettings shape_cast_settings;
	shape_cast_settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideWithAll;
	shape_cast_settings.mReturnDeepestPoint = true;

	cast_shape(
			p_world_id,
			p_shape->get_shape(),
			convert_r(p_from),
			convert(p_move),
			layers,
			result,
			ignore_bodies,
			shape_cast_settings);

	Array res;
	for (auto hit : result.hits) {
		JBody3D *body2 = Jolt::singleton()->body_fetch(hit.mBodyID2);
		Dictionary d;
		d["body2"] = body2;
		d["normal"] = convert(-hit.mPenetrationAxis.Normalized());
		d["depth"] = hit.mPenetrationDepth;
		d["contact_on1"] = convert(hit.mContactPointOn1);
		d["contact_on2"] = convert(hit.mContactPointOn2);
		d["fraction"] = hit.mFraction;
		d["is_back_face_hit"] = hit.mIsBackFaceHit;
		res.push_back(d);
	}

	return res;
}

Array Jolt::collide_shape_gd(
		uint32_t p_world_id,
		Ref<JShape3D> p_shape,
		const Transform3D &p_from,
		const Array &p_collide_with_layers,
		const Array &p_ignore_bodies) const {
	JQueryVectorResult<JPH::CollideShapeCollector> result;

	LocalVector<JPH::ObjectLayer> layers;
	for (int i = 0; i < p_collide_with_layers.size(); i++) {
		const JPH::ObjectLayer layer = get_layer_from_layer_name(p_collide_with_layers[i]);
		layers.push_back(layer);
	}

	LocalVector<uint32_t> ignore_bodies;
	for (int i = 0; i < p_ignore_bodies.size(); i++) {
		if (p_ignore_bodies[i].get_type() == Variant::INT) {
			ignore_bodies.push_back(p_ignore_bodies[i]);
		} else if (p_ignore_bodies[i].get_type() == Variant::OBJECT) {
			Object *o = p_ignore_bodies[i];
			JBody3D *b = Object::cast_to<JBody3D>(o);
			if (b) {
				ignore_bodies.push_back(b->get_body_id());
			}
		}
	}

	JPH::ShapeCastSettings shape_cast_settings;
	shape_cast_settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideWithAll;
	shape_cast_settings.mReturnDeepestPoint = true;

	collide_shape(
			p_world_id,
			p_shape->get_shape(),
			convert_r(p_from),
			layers,
			result,
			ignore_bodies);

	Array res;
	for (auto hit : result.hits) {
		JBody3D *body2 = Jolt::singleton()->body_fetch(hit.mBodyID2);
		Dictionary d;
		d["body2"] = body2;
		d["normal"] = convert(-hit.mPenetrationAxis.Normalized());
		d["depth"] = hit.mPenetrationDepth;
		d["contact_on1"] = convert(hit.mContactPointOn1);
		d["contact_on2"] = convert(hit.mContactPointOn2);
		res.push_back(d);
	}
	return res;
}

bool _is_hit_dict_closer(const Dictionary &p_hit1, const Dictionary &p_hit2) {
	real_t hit1_fraction = p_hit1["fraction"];
	real_t hit2_fraction = p_hit2["fraction"];
	if (hit1_fraction == hit2_fraction) {
		// Fractions are equal, so compare depths.
		real_t hit1_depth = p_hit1["depth"];
		real_t hit2_depth = p_hit2["depth"];
		return hit1_depth > hit2_depth; // More depth means closer.
	}
	return hit1_fraction < hit2_fraction; // Less fraction means closer.
}

TypedArray<Dictionary> _sort_hits_by_distance(const Array &p_hits_array) {
	TypedArray<Dictionary> sorted_hits;
	for (int i = 0; i < p_hits_array.size(); i++) {
		const Dictionary &hit = p_hits_array[i];
		int index_to_insert = 0;
		for (int j = 0; j < sorted_hits.size(); j++) {
			const Dictionary &existing_hit = sorted_hits[j];
			if (_is_hit_dict_closer(hit, existing_hit)) {
				break;
			}
			index_to_insert++;
		}
		sorted_hits.insert(index_to_insert, hit);
	}
	return sorted_hits;
}

void _insert_hit_if_closer(const Dictionary &p_hit, Dictionary p_best_hits) {
	// This is a JBody3D* but there's no reason to cast it since we only need Variant.
	Variant hit_body = p_hit["body2"];
	real_t hit_depth = p_hit["depth"];
	real_t hit_fraction = p_hit["fraction"];
	Variant existing_hit = p_best_hits.get(hit_body, Variant());
	if (existing_hit.get_type() == Variant::DICTIONARY) {
		Dictionary existing_hit_dict = existing_hit;
		real_t existing_hit_fraction = existing_hit_dict["fraction"];
		if (existing_hit_fraction < hit_fraction) {
			return;
		}
		real_t existing_hit_depth = existing_hit_dict["depth"];
		if (existing_hit_depth >= hit_depth) {
			return;
		}
	}
	// By here we have determined that this hit is the best so far.
	Dictionary best_hit;
	best_hit["body"] = hit_body;
	best_hit["depth"] = hit_depth;
	best_hit["fraction"] = hit_fraction;
	best_hit["normal"] = p_hit["normal"];
	best_hit["position"] = p_hit["contact_on2"];
	p_best_hits[hit_body] = best_hit;
}

TypedArray<Dictionary> Jolt::high_level_ray_or_shape_cast(
		const Vector3 &p_from,
		const Vector3 &p_direction,
		real_t p_length,
		real_t p_sphere_radius,
		const Array &p_collide_with_layers,
		const Array &p_ignore_bodies) const {
	Dictionary best_hits;
	if (p_sphere_radius > 0.0f) {
		// Shape cast.
		Ref<JSphereShape3D> shape;
		shape.instantiate();
		shape->set_radius(p_sphere_radius);
		Transform3D transform = Transform3D(Basis(), p_from);
		Vector3 move;
		if (p_length < 0.0f) {
			move = p_direction;
		} else {
			move = p_direction.normalized() * p_length;
		}
		Array shape_cast_result = cast_shape_gd(0, shape, transform, move, p_collide_with_layers, p_ignore_bodies);
		for (int i = 0; i < shape_cast_result.size(); i++) {
			Dictionary hit = shape_cast_result[i];
			_insert_hit_if_closer(hit, best_hits);
		}
	} else {
		// Ray cast.
		if (p_length < 0.0f) {
			p_length = p_direction.length();
		}
		Dictionary raycast_result = cast_ray_gd(0, p_from, p_direction, p_length, p_collide_with_layers, p_ignore_bodies);
		bool has_hit = raycast_result["has_hit"];
		if (has_hit) {
			Dictionary hit;
			// This is a JBody3D* but there's no reason to cast it since we only need Variant.
			Variant body = raycast_result["collider"];
			hit["body"] = body;
			hit["depth"] = 0.0f;
			hit["fraction"] = raycast_result["fraction"];
			hit["normal"] = raycast_result["normal"];
			hit["position"] = raycast_result["position"];
			best_hits[body] = hit;
		}
	}
	// Sort the hits with the closest first.
	return _sort_hits_by_distance(best_hits.values());
}

void Jolt::cast_ray(
		uint32_t p_world_id,
		const JPH::RVec3 &p_from,
		const JPH::Vec3 &p_direction,
		real_t p_distance,
		const LocalVector<JPH::ObjectLayer> &p_collide_with_layers,
		JPH::CastRayCollector &r_result,
		const LocalVector<uint32_t> &p_ignore_bodies,
		const JPH::ShapeCastSettings &p_shape_cast_settings) const {
	const JPH::PhysicsSystem *physics_system = worlds[0];
	const JPH::NarrowPhaseQuery &query = physics_system->GetNarrowPhaseQuery();

	JPH::RRayCast ray;
	ray.mOrigin = p_from;
	ray.mDirection = p_direction * p_distance;

	JPH::RayCastSettings ray_cast_settings;
	ray_cast_settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;

	JPH::BroadPhaseLayerFilter broad_phase_layer_filter;
	ObjectLayersFilter object_layer_filter;
	object_layer_filter.layers = p_collide_with_layers;
	JPH::IgnoreMultipleBodiesFilter ignore_bodies_filter;
	for (auto ib : p_ignore_bodies) {
		ignore_bodies_filter.IgnoreBody(JPH::BodyID(ib));
	}
	JPH::ShapeFilter shape_filter;

	query.CastRay(
			ray,
			ray_cast_settings,
			r_result,
			broad_phase_layer_filter,
			object_layer_filter,
			ignore_bodies_filter,
			shape_filter);
}

void Jolt::cast_shape(
		uint32_t p_world_id,
		JPH::ShapeRefC p_shape,
		const JPH::RMat44 &p_transform,
		const JPH::Vec3 &p_move,
		const LocalVector<JPH::ObjectLayer> &p_collide_with_layers,
		JPH::CastShapeCollector &r_result,
		const LocalVector<uint32_t> &p_ignore_bodies,
		const JPH::ShapeCastSettings &p_shape_cast_settings) const {
	const JPH::PhysicsSystem *physics_system = worlds[0];
	const JPH::NarrowPhaseQuery &query = physics_system->GetNarrowPhaseQuery();

	const JPH::RVec3 no_base_offset = JPH::RVec3::sZero(); // Always with no offset.
	JPH::BroadPhaseLayerFilter broad_phase_layer_filter;
	ObjectLayersFilter object_layer_filter;
	object_layer_filter.layers = p_collide_with_layers;
	JPH::IgnoreMultipleBodiesFilter ignore_bodies_filter;
	JPH::ShapeFilter shape_filter;

	ignore_bodies_filter.Reserve(p_ignore_bodies.size());
	for (int i = 0; i < int(p_ignore_bodies.size()); i++) {
		ignore_bodies_filter.IgnoreBody(JPH::BodyID(p_ignore_bodies[i]));
	}

	JPH::RShapeCast shape_cast = JPH::RShapeCast::sFromWorldTransform(
			p_shape,
			JPH::Vec3::sReplicate(1.0f),
			p_transform,
			p_move);

	query.CastShape(
			shape_cast,
			p_shape_cast_settings,
			no_base_offset,
			r_result,
			broad_phase_layer_filter,
			object_layer_filter,
			ignore_bodies_filter,
			shape_filter);
}

void Jolt::collide_shape(
		uint32_t p_world_id,
		JPH::ShapeRefC p_shape,
		const JPH::RMat44 &p_transform,
		const LocalVector<JPH::ObjectLayer> &p_collide_with_layers,
		JPH::CollideShapeCollector &r_result,
		const LocalVector<uint32_t> &p_ignore_bodies,
		const JPH::CollideShapeSettings &p_settings) const {
	const JPH::PhysicsSystem *physics_system = worlds[0];
	const JPH::NarrowPhaseQuery &query = physics_system->GetNarrowPhaseQuery();

	const JPH::RVec3 no_base_offset = JPH::RVec3::sZero(); // Always with no offset.
	JPH::BroadPhaseLayerFilter broad_phase_layer_filter;
	ObjectLayersFilter object_layer_filter;
	object_layer_filter.layers = p_collide_with_layers;
	JPH::IgnoreMultipleBodiesFilter ignore_bodies_filter;
	JPH::ShapeFilter shape_filter;

	ignore_bodies_filter.Reserve(p_ignore_bodies.size());
	for (int i = 0; i < int(p_ignore_bodies.size()); i++) {
		ignore_bodies_filter.IgnoreBody(JPH::BodyID(p_ignore_bodies[i]));
	}

	query.CollideShape(
			p_shape,
			JPH::Vec3::sReplicate(1.0f), // Scale
			p_transform,
			p_settings,
			no_base_offset,
			r_result,
			broad_phase_layer_filter,
			object_layer_filter,
			ignore_bodies_filter,
			shape_filter);
}

void Jolt::collide_sphere_broad_phase(
		const JPH::Vec3 &p_sphere_center,
		float p_radius,
		std::vector<JPH::BodyID> &p_overlaps) const {
	p_overlaps.clear();

	BroadphaseCollector bp_collector(p_overlaps);
	JPH::PhysicsSystem *physics_system = worlds[0];
	physics_system->GetBroadPhaseQuery().CollideSphere(
			p_sphere_center,
			p_radius,
			bp_collector);
}

JPH::Ref<JPH::CharacterVirtual> Jolt::create_character_virtual(
		const JPH::RMat44 &p_transform,
		JPH::CharacterVirtualSettings *p_settings) {
	JPH::PhysicsSystem *physics_system = worlds[0];

	return JPH::Ref<JPH::CharacterVirtual>(
			new JPH::CharacterVirtual(
					p_settings,
					p_transform.GetTranslation(),
					p_transform.GetQuaternion(),
					0,
					physics_system));
}

void Jolt::character_virtual_pre_step(
		float p_delta_time,
		JPH::CharacterVirtual *p_character,
		JPH::ObjectLayer p_layer_id,
		const LocalVector<uint32_t> &p_ignore_bodies) {
	const JPH::DefaultBroadPhaseLayerFilter broad_phase_layer_filter(object_vs_broad_phase_layer_filter, p_layer_id);
	TableObjectLayerFilter object_layer_filter;
	object_layer_filter.layer = p_layer_id;
	object_layer_filter.table = *layers_table;
	JPH::IgnoreMultipleBodiesFilter ignore_bodies_filter;
	JPH::ShapeFilter shape_filter;

	ignore_bodies_filter.Reserve(p_ignore_bodies.size());
	for (int i = 0; i < int(p_ignore_bodies.size()); i++) {
		ignore_bodies_filter.IgnoreBody(JPH::BodyID(p_ignore_bodies[i]));
	}

	p_character->UpdateGroundVelocity();
}

void Jolt::character_virtual_step(
		float p_delta_time,
		JPH::CharacterVirtual &p_character,
		JPH::CharacterVirtual::ExtendedUpdateSettings &p_update_settings,
		JPH::ObjectLayer p_layer_id,
		const LocalVector<uint32_t> &p_ignore_bodies) {
	JPH::PhysicsSystem *physics_system = worlds[0];

	const JPH::DefaultBroadPhaseLayerFilter broad_phase_layer_filter(object_vs_broad_phase_layer_filter, p_layer_id);
	TableObjectLayerFilter object_layer_filter;
	object_layer_filter.layer = p_layer_id;
	object_layer_filter.table = *layers_table;
	JPH::IgnoreMultipleBodiesFilter ignore_bodies_filter;
	JPH::ShapeFilter shape_filter;

	ignore_bodies_filter.Reserve(p_ignore_bodies.size());
	for (int i = 0; i < int(p_ignore_bodies.size()); i++) {
		ignore_bodies_filter.IgnoreBody(JPH::BodyID(p_ignore_bodies[i]));
	}

	p_character.ExtendedUpdate(
			p_delta_time,
			physics_system->GetGravity(),
			p_update_settings,
			broad_phase_layer_filter,
			object_layer_filter,
			ignore_bodies_filter,
			shape_filter,
			character_virtual_tmp_allocator);
}

JPH::ShapeRefC Jolt::get_scaled_shape(JPH::ShapeRefC p_base_shape, JPH::Vec3 p_scale) {
	if (!p_scale.IsClose(JPH::Vec3(1, 1, 1))) {
		// Create the scaled shape.
		JPH::ScaledShapeSettings scaled_shape_settings(p_base_shape.GetPtr(), p_scale);
		JPH::ShapeSettings::ShapeResult scaled_shape_result = scaled_shape_settings.Create();
		CRASH_COND_MSG(scaled_shape_result.HasError(), "Failed to create ScaledShape: `" + String(scaled_shape_result.GetError().c_str()) + "`.");
		CRASH_COND(!scaled_shape_result.IsValid()); // This can't be triggered at this point, because of the above check.
		JPH::ShapeRefC scaled_shape = scaled_shape_result.Get();

		return scaled_shape;
	} else {
		return p_base_shape;
	}
}
