#include "tm_character_3d.h"

#include "Jolt/Math/Vec3.h"
#include "core/math/vector3.h"
#include "modules/jolt/godot_expose_utils.h"
#include "modules/jolt/j_query_results.h"
#include "modules/jolt/j_shape.h"
#include "modules/jolt/j_utils.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Math/Real.h"
#include "modules/network_synchronizer/core/core.h"
#include "modules/network_synchronizer/data_buffer.h"
#include "scene/3d/label_3d.h"

#include "tm_scene_sync.h"
#include "tm_space_object_base.h"

#define __CLASS__ TMCharacter3D

void TMCharacter3D::set_character_height(real_t p_feet_heigth) {
	character_height = p_feet_heigth;
	// Reset the virtual character.
	virtual_character = nullptr;
}

real_t TMCharacter3D::get_character_height() const {
	return character_height;
}

void TMCharacter3D::set_character_radius(real_t p_radius) {
	character_radius = p_radius;
	// Reset the virtual character.
	virtual_character = nullptr;
}

real_t TMCharacter3D::get_character_radius() const {
	return character_radius;
}

SETGET_IMPL(real_t, character_supporting_offset);

SETGET_IMPL(real_t, max_walking_slope_deg);

SETGET_IMPL(real_t, walk_speed);
SETGET_IMPL(real_t, run_speed);
SETGET_IMPL(real_t, jump_height);
SETGET_IMPL(real_t, ground_acceleration);
SETGET_IMPL(real_t, ground_deceleration);
SETGET_IMPL(real_t, air_acceleration);
SETGET_IMPL(real_t, air_deceleration);
SETGET_IMPL(real_t, step_height);
SETGET_IMPL(real_t, max_push_strength_newton);
SETGET_IMPL(real_t, predictive_contact_margin);
SETGET_IMPL(real_t, shape_margin);

SETGET_IMPL(bool, frozen);
SETGET_IMPL(real_t, movement_scale);

void TMCharacter3D::_bind_methods() {
	EXPOSE_NO_HINT(FLOAT, character_height);
	EXPOSE_NO_HINT(FLOAT, character_radius);
	EXPOSE_NO_HINT(FLOAT, character_supporting_offset);
	EXPOSE_NO_HINT(FLOAT, max_walking_slope_deg);

	EXPOSE_NO_HINT(FLOAT, walk_speed);
	EXPOSE_NO_HINT(FLOAT, run_speed);
	EXPOSE_NO_HINT(FLOAT, jump_height);
	EXPOSE_NO_HINT(FLOAT, ground_acceleration);
	EXPOSE_NO_HINT(FLOAT, ground_deceleration);
	EXPOSE_NO_HINT(FLOAT, air_acceleration);
	EXPOSE_NO_HINT(FLOAT, air_deceleration);
	EXPOSE_NO_HINT(FLOAT, step_height);
	EXPOSE_NO_HINT(FLOAT, max_push_strength_newton);
	EXPOSE_NO_HINT(FLOAT, predictive_contact_margin);
	EXPOSE_NO_HINT(FLOAT, shape_margin);

	EXPOSE_NO_HINT(BOOL, frozen);
	EXPOSE_NO_HINT(FLOAT, movement_scale);

	ClassDB::bind_method(D_METHOD("jolt_char_set_state", "data"), &TMCharacter3D::jolt_char_set_state);
	ClassDB::bind_method(D_METHOD("jolt_char_get_state"), &TMCharacter3D::jolt_char_get_state);

	ClassDB::bind_method(D_METHOD("get_cached_max_walk_speed"), &TMCharacter3D::get_cached_max_walk_speed);

	ClassDB::bind_method(D_METHOD("process_character", "delta", "input_move", "input_run", "input_jump", "gravity"), &TMCharacter3D::process_character);

	ClassDB::bind_method(D_METHOD("is_on_floor"), &TMCharacter3D::is_on_floor);
	ClassDB::bind_method(D_METHOD("is_on_ceiling"), &TMCharacter3D::is_on_ceiling);

	ClassDB::bind_method(D_METHOD("queue_interaction", "jbody", "interaction_name", "argument"), &TMCharacter3D::queue_interaction);
	ClassDB::bind_method(D_METHOD("write_pending_interactions_to_buffer", "out_buffer"), &TMCharacter3D::write_pending_interactions_to_buffer);
	ClassDB::bind_method(D_METHOD("execute_interactions", "buffer"), &TMCharacter3D::execute_interactions);
	ClassDB::bind_method(D_METHOD("are_interactions_different", "buffer_A", "buffer_B"), &TMCharacter3D::are_interactions_different);
	ClassDB::bind_method(D_METHOD("count_interactions_buffer_size", "buffer"), &TMCharacter3D::count_interactions_buffer_size);

	ClassDB::bind_method(D_METHOD("get_ground_velocity"), &TMCharacter3D::get_ground_velocity);
	ClassDB::bind_method(D_METHOD("get_character_relative_velocity"), &TMCharacter3D::get_character_relative_velocity);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "jolt_char_sync_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "jolt_char_set_state", "jolt_char_get_state");

	ADD_SIGNAL(MethodInfo("jump"));
}

#undef __CLASS__

TMCharacter3D::TMCharacter3D() :
		JBody3D() {
	set_mass(70.0);
}

TMCharacter3D::~TMCharacter3D() {
}

void TMCharacter3D::jolt_char_set_state(const Vector<uint8_t> &p_data) {
	last_char_set_state = p_data;
	notify_received_net_sync_update();

	if (p_data.size() < 1) {
		return;
	}

	jolt_state.set_data(p_data);
	jolt_state.BeginRead();
	jolt_state.SetValidating(false);

	jolt_state.Read(previous_move_velocity);

	if (!body && has_desired_body_id()) {
		create_body();
	}

	update_virtual_character();

	if (virtual_character) {
		virtual_character->RestoreState(jolt_state);
	}
}

Vector<uint8_t> TMCharacter3D::jolt_char_get_state() {
	if (TMSceneSync::singleton()->is_client() && last_char_set_state.size() == 0) {
		// On the server the VC is not yet created, so pretend it's not
		// created here either.
		return last_char_set_state;
	}

	// Just collect the `character_virtual` everything else is being collected
	// by `TMSceneSync::jolt_get_state`.
	if (virtual_character) {
		jolt_state.Clear();
		jolt_state.Write(previous_move_velocity);
		virtual_character->SaveState(jolt_state);
		Vector<uint8_t> state_data = jolt_state.get_data();
		return state_data;
	} else {
		return last_char_set_state;
	}
}

real_t TMCharacter3D::get_cached_max_walk_speed() const {
	return cached_is_running ? run_speed : walk_speed;
}

JPH::Vec3 move_toward(const JPH::Vec3 &p_from, const JPH::Vec3 &p_to, const real_t p_speed) {
	const JPH::Vec3 v = p_from;
	const JPH::Vec3 vd = p_to - v;
	const real_t len = vd.Length();
	if (len <= 0.001 || len < p_speed) {
		return p_to;
	} else {
		return v + ((vd / len) * p_speed);
	}
}

void TMCharacter3D::process_character(
		real_t p_delta,
		const Vector2 &p_input_move,
		bool p_input_run,
		bool p_input_jump,
		real_t p_gravity) {
	// Make sure this character doesn't disappear while simulating, no matter what.
	notify_received_net_sync_update();

	if (frozen) {
		// Nothing to update.
		const JPH::RVec3 Pos = virtual_character->GetPosition();
		const JPH::Quat Rot = virtual_character->GetRotation();
		body->MoveKinematic(
				Pos,
				Rot,
				float(p_delta));
		return;
	}

	update_virtual_character();
	if (!virtual_character) {
		return;
	}

	// Update the GroudVelocity.
	Jolt::singleton()->character_virtual_pre_step(
			float(p_delta),
			virtual_character,
			Jolt::singleton()->get_layer_from_layer_name(layer_name),
			ignore_bodies);

	const bool on_ground = is_on_floor();
	const real_t speed = (p_input_run ? run_speed : walk_speed) * movement_scale;
	const real_t acceleration = (on_ground ? ground_acceleration : air_acceleration) * movement_scale;
	const real_t deceleration = (on_ground ? ground_deceleration : air_deceleration) * movement_scale;

	// 1. Calculate ground motion.
	JPH::Vec3 velocity = JPH::Vec3::sZero();

	if (on_ground) {
		velocity += virtual_character->GetGroundVelocity();
	}

	// 2. Calculate force and impulses.
	{
		const JPH::Vec3 force = body->GetAccumulatedForce();
		velocity += p_delta * force * body->GetMotionProperties()->GetInverseMass();
		body->ResetForce();
		body->ResetTorque();

		velocity += accumulated_impulses * body->GetMotionProperties()->GetInverseMass();
		accumulated_impulses = JPH::Vec3::sZero();
	}

	// 3. Calculate input motion.
	{
		const JPH::Vec3 input = JPH::Vec3(p_input_move.x, 0.0, p_input_move.y).NormalizedOr(JPH::Vec3::sZero());
		const JPH::Vec3 target_velocity = input * speed;

		JPH::Vec3 move_velocity;
		if (p_input_move.is_zero_approx()) {
			move_velocity = move_toward(previous_move_velocity, target_velocity, deceleration * p_delta);
		} else {
			move_velocity = move_toward(previous_move_velocity, target_velocity, acceleration * p_delta);
		}
		previous_move_velocity = move_velocity;
		velocity += move_velocity;
	}

	// 4. Calculate gravity and Jump
	if (!on_ground) {
		// When falling, preserve the velocity.
		velocity.SetY(velocity.GetY() + virtual_character->GetLinearVelocity().GetY());
		// Add gravity.
		velocity.SetY(velocity.GetY() + (p_gravity * p_delta * -1.5));
	} else if (p_input_jump) {
		// Calculate jump velocity
		velocity.SetY(Math::sqrt(2.0 * jump_height * p_gravity * movement_scale));
		emit_signal("jump");
	}

	// 5. Step the motion.
	__step(p_delta, velocity);
}

void TMCharacter3D::__step(real_t p_delta_time, const JPH::Vec3 &p_velocity) {
	// Documentation
	// https://github.com/jrouwe/JoltPhysics/blob/master/Samples/Tests/Character/CharacterVirtualTest.cpp
	// https://github.com/jrouwe/JoltPhysics/blob/master/UnitTests/Physics/CharacterVirtualTests.cpp
	// https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Physics/Character/CharacterVirtual.cpp

	virtual_character->SetLinearVelocity(p_velocity);

	update_settings.mWalkStairsStepUp = JPH::Vec3(0, step_height, 0);
	update_settings.mWalkStairsStepDownExtra = JPH::Vec3(0, -step_height, 0);
	update_settings.mStickToFloorStepDown = JPH::Vec3(0, -step_height, 0);

	Jolt::singleton()->character_virtual_step(
			float(p_delta_time),
			*virtual_character,
			update_settings,
			Jolt::singleton()->get_layer_from_layer_name(layer_name),
			ignore_bodies);

	const JPH::RVec3 Pos = virtual_character->GetPosition();
	const JPH::Quat Rot = virtual_character->GetRotation();

	// Move the KinematicBody
	Jolt::singleton()->body_activate(body->GetID());
	body->MoveKinematic(
			Pos,
			Rot,
			float(p_delta_time));
}

bool TMCharacter3D::is_on_ceiling() const {
	return false;
}

bool TMCharacter3D::is_on_floor() const {
	if (virtual_character) {
		return virtual_character->IsSupported();
	}
	return false;
}

void TMCharacter3D::update_virtual_character() {
	if (virtual_character) {
		// Nothing to do.
		return;
	}

	if (body == nullptr || shape == nullptr || shape->get_shape() == nullptr) {
		// Destroy the virtual character.
		virtual_character = nullptr;
	} else {
		// Create the virtual character.
		character_settings.mShape = shape->get_shape(); // TODO generated the shape based on the character_height + character_radius? I'm not doing this right now, because I'm not sure I want to keep this API.
		//character_settings.mShapeOffset = JPH::Vec3::sAxisY() * (character_height / 2.0);

		// Configure supporting volume so that accepts contacts that touch the
		// lower sphere of the capsule.
		character_settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -character_supporting_offset);
		character_settings.mMaxSlopeAngle = Math::deg_to_rad(max_walking_slope_deg);
		character_settings.mMass = get_mass();
		character_settings.mMaxStrength = max_push_strength_newton;
		character_settings.mPredictiveContactDistance = predictive_contact_margin;
		character_settings.mCharacterPadding = shape_margin;

		virtual_character = Jolt::singleton()->create_character_virtual(
				body->GetWorldTransform(),
				&character_settings);

		ignore_bodies.clear();
		ignore_bodies.push_back(get_body_id());
	}
}

void TMCharacter3D::__update_body() {
	JBody3D::__update_body();
	virtual_character = nullptr;
}

void TMCharacter3D::__create_body() {
	JBody3D::__create_body();
	virtual_character = nullptr;
}

void TMCharacter3D::__update_godot_transform() {
	set_notify_local_transform(false);
	set_notify_transform(false);
	set_global_transform(Jolt::singleton()->body_get_transform(this));
	set_notify_local_transform(true);
	set_notify_transform(true);
}

void TMCharacter3D::__update_jolt_transform() {
	// First set the transform on the kinematic jolt body as usual.
	JBody3D::__update_jolt_transform();

	// Then, set the transform on the virtual_character.
	if (virtual_character) {
		const Transform3D trsf = get_global_transform();
		const JPH::RVec3 o = convert_r(trsf.get_origin());
		const JPH::Quat q = convert(trsf.get_basis().get_rotation_quaternion());
		//const JPH::Vec3 s = convert(trsf.get_basis().get_scale_abs());

		virtual_character->SetPosition(o);
		virtual_character->SetRotation(q);
	}
}

void TMCharacter3D::update_3d_label(uint32_t p_listening_group_index) {
	Object *o = get_node_or_null(NodePath("TMCharacterLabel"));
	Label3D *label = Object::cast_to<Label3D>(o);
	if (!label) {
		label = memnew(Label3D);
		label->set_owner(this);
		add_child(label);
		label->set_name("TMCharacterLabel");
		label->set_font_size(30);
		label->set_outline_size(5);
		label->set_draw_flag(Label3D::FLAG_DISABLE_DEPTH_TEST, true);
		label->set_draw_flag(Label3D::FLAG_FIXED_SIZE, true);
		label->set_billboard_mode(StandardMaterial3D::BillboardMode::BILLBOARD_ENABLED);
	}

	label->set_text("Listening to group: " + uitos(p_listening_group_index));
	label->set_modulate(Color(1, 1, 0.0));
	label->set_pixel_size(0.001);
}

void TMCharacter3D::queue_interaction(Object *p_jbody, const StringName &p_name, const Variant &p_argument) {
	JBody3D *jbody = Object::cast_to<JBody3D>(p_jbody);
	ERR_FAIL_COND(!jbody);
	const int interaction_id = jbody->get_interaction_id(p_name);
	ERR_FAIL_COND(interaction_id < 0);
	ERR_FAIL_COND(interaction_id > 255);
	PendingInteraction pi;
	pi.node_net_id = TMSceneSync::singleton()->scene_synchronizer.get_app_object_net_id(TMSceneSync::singleton()->scene_synchronizer.to_handle(jbody));
	pi.interaction_id = interaction_id;
	ERR_FAIL_COND(pi.node_net_id == NS::ObjectNetId::NONE);
	pi.argument = p_argument.duplicate(true);
	pending_interactions.push_back(std::move(pi));
}

void TMCharacter3D::write_pending_interactions_to_buffer(Object *r_buffer) {
	DataBuffer *buffer = Object::cast_to<DataBuffer>(r_buffer);
	ERR_FAIL_COND(!buffer);

	for (const PendingInteraction &pi : pending_interactions) {
		buffer->add(true);
		buffer->add(pi.node_net_id.id);
		buffer->add(pi.interaction_id);
		buffer->add_variant(pi.argument);
	}

	buffer->add(false);

	pending_interactions.clear();
}

void TMCharacter3D::execute_interactions(Object *p_buffer) {
	DataBuffer *buffer = Object::cast_to<DataBuffer>(p_buffer);
	ERR_FAIL_COND(!buffer);
	while (true) {
		bool has_next;
		buffer->read(has_next);

		if (!has_next) {
			break;
		}

		PendingInteraction pi;

		buffer->read(pi.node_net_id.id);
		buffer->read(pi.interaction_id);
		pi.argument = buffer->read_variant();

		Node *nd = TMSceneSync::singleton()->get_node_from_id(pi.node_net_id.id);
		ERR_FAIL_COND(!nd);
		// Since the interactions are added only if the object passed is a SpaceObject
		// we can safely assume here we always get a `SpaceObjectBase` or `null`.
		JBody3D *jbody = dynamic_cast<JBody3D *>(nd);
		ERR_FAIL_COND(!jbody);
		jbody->call_interaction(pi.interaction_id, pi.argument);
	}
}

bool TMCharacter3D::are_interactions_different(Object *p_buffer_A, Object *p_buffer_B) {
	DataBuffer *buffer_A = Object::cast_to<DataBuffer>(p_buffer_A);
	DataBuffer *buffer_B = Object::cast_to<DataBuffer>(p_buffer_B);
	ERR_FAIL_COND_V(!buffer_A, true);
	ERR_FAIL_COND_V(!buffer_B, true);
	while (true) {
		bool A_has_next;
		bool B_has_next;

		buffer_A->read(A_has_next);
		buffer_B->read(B_has_next);

		if (A_has_next != B_has_next) {
			return true;
		}

		if (!A_has_next) {
			break;
		}

		PendingInteraction A_pi;
		PendingInteraction B_pi;

		buffer_A->read(A_pi.node_net_id.id);
		buffer_B->read(B_pi.node_net_id.id);
		if (A_pi.node_net_id != B_pi.node_net_id) {
			return true;
		}

		buffer_A->read(A_pi.interaction_id);
		buffer_B->read(B_pi.interaction_id);
		if (A_pi.interaction_id != B_pi.interaction_id) {
			return true;
		}

		A_pi.argument = buffer_A->read_variant();
		B_pi.argument = buffer_B->read_variant();
		if (A_pi.argument != B_pi.argument) {
			return true;
		}
	}

	return false;
}

int TMCharacter3D::count_interactions_buffer_size(Object *p_buffer) const {
	DataBuffer *buffer = Object::cast_to<DataBuffer>(p_buffer);
	ERR_FAIL_COND_V(!buffer, 0);

	int size = 0;

	while (true) {
		bool has_next;
		buffer->read(has_next);
		size += buffer->get_bool_size();

		if (!has_next) {
			break;
		}

		size += buffer->read_int_size(DataBuffer::COMPRESSION_LEVEL_1); // NetId
		size += buffer->read_int_size(DataBuffer::COMPRESSION_LEVEL_3); // Interaction Id
		size += buffer->read_variant_size(); // Variant
	}

	return size;
}

void TMCharacter3D::add_impulse(const JPH::Vec3 &p_impulse) {
	accumulated_impulses += p_impulse;
}

void TMCharacter3D::add_impulse_at_position(const JPH::Vec3 &p_impulse, const JPH::RVec3 &p_position) {
	add_impulse(p_impulse);
}

Vector3 TMCharacter3D::get_ground_velocity() const {
	if (virtual_character) {
		return convert(virtual_character->GetGroundVelocity());
	} else {
		return Vector3();
	}
}

Vector3 TMCharacter3D::get_character_relative_velocity() const {
	if (virtual_character) {
		return convert(virtual_character->GetLinearVelocity() - virtual_character->GetGroundVelocity());
	} else {
		return Vector3();
	}
}
