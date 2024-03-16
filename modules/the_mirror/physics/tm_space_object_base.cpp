#include "tm_space_object_base.h"

#include "core/object/object.h"
#include "scene/3d/label_3d.h"
#include "scene/main/node.h"

#include "tm_scene_sync.h"

TMSpaceObjectBase::TMSpaceObjectBase() :
		JBody3D() {
}

void TMSpaceObjectBase::_bind_methods() {
	ClassDB::bind_method(D_METHOD("mark_as_changed"), &TMSpaceObjectBase::mark_as_changed);

	ClassDB::bind_method(D_METHOD("set_selected_by_peers", "peers"), &TMSpaceObjectBase::set_selected_by_peers);
	ClassDB::bind_method(D_METHOD("get_selected_by_peers"), &TMSpaceObjectBase::get_selected_by_peers);

	ClassDB::bind_method(D_METHOD("notify_peer_selection_start", "peer"), &TMSpaceObjectBase::notify_peer_selection_start);
	ClassDB::bind_method(D_METHOD("notify_peer_selection_end", "peer"), &TMSpaceObjectBase::notify_peer_selection_end);

	ClassDB::bind_method(D_METHOD("server_npc_move_velocity", "delta", "direction", "acceleration", "deceleration", "max_speed", "step_height", "max_push_force", "supporting_height", "gravity"), &TMSpaceObjectBase::server_npc_move_velocity);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "selected_by_peers"), "set_selected_by_peers", "get_selected_by_peers");
}

void TMSpaceObjectBase::_notification(int p_what) {
	if (p_what == NOTIFICATION_TRANSFORM_CHANGED) {
		if (!TMSceneSync::singleton()->is_server()) {
			return;
		}

		if (is_static() || is_sensor()) {
			mark_as_changed();
		}
	}
}

void TMSpaceObjectBase::mark_as_changed() {
	static_body_updated_sync_groups.clear();
}

bool TMSpaceObjectBase::static_space_object_sync_group_needs_update(const TMSyncGroupMetadata &p_group) {
	const bool need_update = static_body_updated_sync_groups.find(static_cast<const void *>(&p_group)) == -1;
	if (need_update) {
		static_body_updated_sync_groups.push_back(static_cast<const void *>(&p_group));
	}
	return need_update;
}

void TMSpaceObjectBase::set_selected_by_peers(const Vector<int> &p_selected_by_peers) {
	selected_by_peers = p_selected_by_peers;
}

const Vector<int> &TMSpaceObjectBase::get_selected_by_peers() const {
	return selected_by_peers;
}

void TMSpaceObjectBase::update_3d_label(
		bool p_is_simulated,
		real_t p_size_over_distance,
		real_t p_distance,
		real_t p_update_rate) {
	Object *o = get_node_or_null(NodePath("SpaceObjectLabel"));
	Label3D *label = Object::cast_to<Label3D>(o);
	if (!label) {
		label = memnew(Label3D);
		label->set_owner(this);
		add_child(label);
		label->set_name("SpaceObjectLabel");
		label->set_font_size(30);
		label->set_outline_size(5);
		label->set_draw_flag(Label3D::FLAG_DISABLE_DEPTH_TEST, true);
		label->set_draw_flag(Label3D::FLAG_FIXED_SIZE, true);
		label->set_billboard_mode(StandardMaterial3D::BillboardMode::BILLBOARD_ENABLED);
	}

	if (p_is_simulated) {
		label->set_text("SIMULATED");
		label->set_modulate(Color(1, 0, 0));
		label->set_pixel_size((real_t)0.001);
	} else {
		label->set_text("Size over distance: " + rtos(p_size_over_distance) + "\nDistance: " + rtos(p_distance) + "\nUpdate rate: " + rtos(p_update_rate));
		label->set_modulate(Color(0.3, 0.3, 0.3).lerp(Color(1, 1, 1), p_update_rate));
		label->set_pixel_size(Math::lerp((real_t)0.0005, (real_t)0.001, p_update_rate));
	}
}

void TMSpaceObjectBase::notify_peer_selection_start(int p_peer) {
	if (selected_by_peers.find(p_peer) == -1) {
		selected_by_peers.push_back(p_peer);
	}
}

void TMSpaceObjectBase::notify_peer_selection_end(int p_peer) {
	selected_by_peers.erase(p_peer);
}

JPH::Vec3 npc_move_toward(const JPH::Vec3 &p_from, const JPH::Vec3 &p_to, const real_t p_speed) {
	const JPH::Vec3 v = p_from;
	const JPH::Vec3 vd = p_to - v;
	const real_t len = vd.Length();
	if (len <= 0.001 || len < p_speed) {
		return p_to;
	} else {
		return v + ((vd / len) * p_speed);
	}
}

void TMSpaceObjectBase::server_npc_move_velocity(
		real_t p_delta,
		const Vector2 &p_direction,
		real_t p_acceleration,
		real_t p_deceleration,
		real_t p_max_speed,
		real_t p_step_height,
		real_t p_max_push_force,
		real_t p_supporting_height,
		real_t p_gravity) {
	ERR_FAIL_COND(is_kinematic() == false);
	ERR_FAIL_COND(!TMSceneSync::singleton()->is_server());
	ERR_FAIL_COND(body == nullptr);
	ERR_FAIL_COND(shape == nullptr);
	ERR_FAIL_COND(shape->get_shape() == nullptr);

	if (
			npc.virtual_character == nullptr ||
			!Math::is_equal_approx((real_t)npc.character_settings.mSupportingVolume.GetConstant(), -p_supporting_height, (real_t)0.01)) {
		npc.character_settings.mShape = shape->get_shape();
		npc.character_settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -p_supporting_height);
		npc.character_settings.mMaxSlopeAngle = Math::deg_to_rad(80.0);
		npc.character_settings.mMass = get_mass();
		npc.character_settings.mMaxStrength = p_max_push_force;
		npc.character_settings.mPredictiveContactDistance = 0.3;
		npc.character_settings.mCharacterPadding = 0.02;
		npc.character_settings.mCollisionTolerance = -0.01;

		npc.virtual_character = Jolt::singleton()->create_character_virtual(
				body->GetWorldTransform(),
				&npc.character_settings);

		npc.virtual_character->SetListener(&npc.listener);
		print_line("NOTICE the space object spawned a new VirtualCharacter.");
	}

	CRASH_COND(!npc.virtual_character);

	npc.virtual_character->SetMaxStrength(p_max_push_force);

	npc.ignore_bodies.clear();
	npc.ignore_bodies.push_back(get_body_id());

	// 0. Set the virtual character transform and velocity.
	npc.virtual_character->SetLinearVelocity(body->GetLinearVelocity());
	npc.virtual_character->SetPosition(body->GetPosition());
	npc.virtual_character->SetRotation(body->GetRotation());

	// Update the GroudVelocity.
	Jolt::singleton()->character_virtual_pre_step(
			float(p_delta),
			npc.virtual_character,
			Jolt::singleton()->get_layer_from_layer_name(layer_name),
			npc.ignore_bodies);

	const bool on_ground = npc.virtual_character->IsSupported();
	const JPH::Vec3 input = JPH::Vec3(p_direction.x, 0.0, p_direction.y).NormalizedOr(JPH::Vec3::sZero());
	const JPH::Vec3 target_velocity = input * p_max_speed;

	// 1. Calculate ground motion.
	JPH::Vec3 velocity = JPH::Vec3::sZero();
	//velocity += npc.virtual_character->GetGroundVelocity();

	// 2. Calculate input motion.
	{
		JPH::Vec3 move_velocity;
		if (p_direction.is_zero_approx()) {
			move_velocity = npc_move_toward(npc.previous_move_velocity, target_velocity, p_deceleration * p_delta);
		} else {
			move_velocity = npc_move_toward(npc.previous_move_velocity, target_velocity, p_acceleration * p_delta);
		}
		npc.previous_move_velocity = move_velocity;
		velocity += move_velocity;
	}

	// 3. Calculate gravity.
	if (!on_ground) {
		// When falling, preserve the velocity.
		velocity.SetY(velocity.GetY() + npc.virtual_character->GetLinearVelocity().GetY());
		// Add gravity.
		velocity.SetY(velocity.GetY() + (p_gravity * p_delta * -1.5));
	}

	// 4. Prepare the stepping.
	npc.virtual_character->SetLinearVelocity(velocity);
	npc.update_settings.mWalkStairsStepUp = JPH::Vec3(0, p_step_height, 0);
	npc.update_settings.mWalkStairsStepDownExtra = JPH::Vec3(0, -p_step_height, 0);
	npc.update_settings.mStickToFloorStepDown = JPH::Vec3(0, -p_step_height, 0);

	// 5. Step.
	Jolt::singleton()->character_virtual_step(
			float(p_delta),
			*npc.virtual_character,
			npc.update_settings,
			Jolt::singleton()->get_layer_from_layer_name(layer_name),
			npc.ignore_bodies);

	const JPH::RVec3 Pos = npc.virtual_character->GetPosition();
	const JPH::Quat Rot = npc.virtual_character->GetRotation();

	// Move the KinematicBody
	Jolt::singleton()->body_activate(body->GetID());
	body->MoveKinematic(
			Pos,
			Rot,
			float(p_delta));
}

bool TMSpaceObjectBase::is_npc() const {
	return get_body_mode() == BodyMode::KINEMATIC;
}

void TMSpaceObjectBase::__destroy_body() {
	// This is executed when the shape changes.
	npc.virtual_character = nullptr;
}
