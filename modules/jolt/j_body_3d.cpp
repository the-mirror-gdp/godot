#include "j_body_3d.h"

#include "Jolt/Math/Real.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Body/AllowedDOFs.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/MassProperties.h"
#include "Jolt/Physics/Body/MotionProperties.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "core/error/error_macros.h"
#include "core/math/basis.h"
#include "core/math/transform_3d.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/local_vector.h"
#include "core/variant/dictionary.h"
#include "j_utils.h"
#include "modules/jolt/j_body_3d.h"
#include "modules/jolt/jolt.h"
#include "modules/the_mirror/physics/tm_scene_sync.h"

#define __CLASS__ JBody3D

/// The `MASS_FACTOR` is a (tweakable) magic number that is used to make the Jolt bodies behave like in the real world.
/// Looking at the Jolt documentation, Jolt uses Kg, but for some reason 1000kg feels much lighter than it should in real life.
#define MASS_FACTOR 100.0

void JBody3D::_bind_methods() {
	BIND_ENUM_CONSTANT(STATIC);
	BIND_ENUM_CONSTANT(KINEMATIC);
	BIND_ENUM_CONSTANT(DYNAMIC);
	BIND_ENUM_CONSTANT(SENSOR);
	BIND_ENUM_CONSTANT(SENSOR_ONLY_ACTIVE);

	ClassDB::bind_method(D_METHOD("set_ignore_state_sync", "ignore"), &JBody3D::set_ignore_state_sync);
	ClassDB::bind_method(D_METHOD("get_ignore_state_sync"), &JBody3D::get_ignore_state_sync);

	ClassDB::bind_method(D_METHOD("jolt_set_state", "data"), &JBody3D::jolt_set_state);
	ClassDB::bind_method(D_METHOD("jolt_get_state"), &JBody3D::jolt_get_state);

	ClassDB::bind_method(D_METHOD("jolt_set_combined_layer_and_body_mode", "layer_and_body_mode"), &JBody3D::jolt_set_combined_layer_and_body_mode);
	ClassDB::bind_method(D_METHOD("jolt_get_combined_layer_and_body_mode"), &JBody3D::jolt_get_combined_layer_and_body_mode);

	ClassDB::bind_method(D_METHOD("get_body_id"), &JBody3D::get_body_id);
	ClassDB::bind_method(D_METHOD("has_body_id"), &JBody3D::has_body_id);

	ClassDB::bind_method(D_METHOD("set_desired_body_id", "body_id"), &JBody3D::set_desired_body_id);
	ClassDB::bind_method(D_METHOD("get_desired_body_id"), &JBody3D::get_desired_body_id);
	ClassDB::bind_method(D_METHOD("has_desired_body_id"), &JBody3D::has_desired_body_id);

	ClassDB::bind_method(D_METHOD("set_body_mode", "body_mode"), &JBody3D::set_body_mode);
	ClassDB::bind_method(D_METHOD("get_body_mode"), &JBody3D::get_body_mode);

	ClassDB::bind_method(D_METHOD("is_dynamic"), &JBody3D::is_dynamic);
	ClassDB::bind_method(D_METHOD("is_static"), &JBody3D::is_static);
	ClassDB::bind_method(D_METHOD("is_kinematic"), &JBody3D::is_kinematic);
	ClassDB::bind_method(D_METHOD("is_sensor"), &JBody3D::is_sensor);

	ClassDB::bind_method(D_METHOD("set_layer_name", "layer_name"), &JBody3D::set_layer_name);
	ClassDB::bind_method(D_METHOD("get_layer_name"), &JBody3D::get_layer_name);

	ClassDB::bind_method(D_METHOD("set_shape", "shape"), &JBody3D::set_shape);
	ClassDB::bind_method(D_METHOD("get_shape"), &JBody3D::get_shape);

	ClassDB::bind_method(D_METHOD("set_allow_sleeping", "allow"), &JBody3D::set_allow_sleeping);
	ClassDB::bind_method(D_METHOD("get_allow_sleeping"), &JBody3D::get_allow_sleeping);

	ClassDB::bind_method(D_METHOD("set_report_precise_touch_location", "precise"), &JBody3D::set_report_precise_touch_location);
	ClassDB::bind_method(D_METHOD("get_report_precise_touch_location"), &JBody3D::get_report_precise_touch_location);

	ClassDB::bind_method(D_METHOD("set_kinematic_detect_static", "can"), &JBody3D::set_kinematic_detect_static);
	ClassDB::bind_method(D_METHOD("get_kinematic_detect_static"), &JBody3D::get_kinematic_detect_static);

	ClassDB::bind_method(D_METHOD("notify_received_net_sync_update"), &JBody3D::notify_received_net_sync_update);
	ClassDB::bind_method(D_METHOD("is_net_sync_updating"), &JBody3D::is_net_sync_updating);

	ClassDB::bind_method(D_METHOD("add_force", "force"), &JBody3D::add_force_gd);
	ClassDB::bind_method(D_METHOD("add_force_at_position", "force", "position"), &JBody3D::add_force_at_position_gd);
	ClassDB::bind_method(D_METHOD("add_torque", "torque"), &JBody3D::add_torque_gd);
	ClassDB::bind_method(D_METHOD("add_impulse", "impulse"), &JBody3D::add_impulse_gd);
	ClassDB::bind_method(D_METHOD("add_impulse_at_position", "impulse", "position"), &JBody3D::add_impulse_at_position_gd);
	ClassDB::bind_method(D_METHOD("add_impulse_angular", "impulse"), &JBody3D::add_impulse_angular_gd);

	ClassDB::bind_method(D_METHOD("set_linear_velocity", "velocity"), &JBody3D::set_linear_velocity_gd);
	ClassDB::bind_method(D_METHOD("get_linear_velocity"), &JBody3D::get_linear_velocity_gd);

	ClassDB::bind_method(D_METHOD("set_angular_velocity", "velocity"), &JBody3D::set_angular_velocity_gd);
	ClassDB::bind_method(D_METHOD("get_angular_velocity"), &JBody3D::get_angular_velocity_gd);

	ClassDB::bind_method(D_METHOD("activate"), &JBody3D::activate);

	ClassDB::bind_method(D_METHOD("move_and_collide", "movement", "collide_with"), &JBody3D::move_and_collide);
	ClassDB::bind_method(D_METHOD("move_kinematic", "position", "rotation", "time"), &JBody3D::move_kinematic);

	ClassDB::bind_method(D_METHOD("cast_body", "move", "collide_with"), &JBody3D::cast_body_gd);
	ClassDB::bind_method(D_METHOD("collide_body", "collide_with_layers", "separation_distance"), &JBody3D::collide_body_gd, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("collide_body_with_position", "position", "collide_with_layers", "separation_distance"), &JBody3D::collide_body_with_position_gd, DEFVAL(0.0));

	ClassDB::bind_method(D_METHOD("create_body"), &JBody3D::create_body);

	ClassDB::bind_method(D_METHOD("register_interaction", "func_name"), &JBody3D::register_interaction);
	ClassDB::bind_method(D_METHOD("get_interaction_id", "func_name"), &JBody3D::get_interaction_id);
	ClassDB::bind_method(D_METHOD("call_interaction", "interaction_index", "argument"), &JBody3D::call_interaction);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "jolt_sync_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "jolt_set_state", "jolt_get_state");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "desired_body_id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_desired_body_id", "get_desired_body_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "body_mode", PROPERTY_HINT_ENUM, "Static,Kinematic,Dynamic,Sensor,SensorOnlyActive"), "set_body_mode", "get_body_mode");
	// This now on the project side, instead of the cpp side, so we comment-out, see https://github.com/the-mirror-megaverse/godot-soft-fork/pull/166
	//ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "layer_name", PROPERTY_HINT_ENUM_SUGGESTION, Jolt::singleton()->layer_get_all_comma_separated_layers_name()), "set_layer_name", "get_layer_name");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "shape", PROPERTY_HINT_RESOURCE_TYPE, "JShape3D"), "set_shape", "get_shape");

	// Motion
	ADD_GROUP("Motion", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_sleeping"), "set_allow_sleeping", "get_allow_sleeping");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "report_precise_touch_location"), "set_report_precise_touch_location", "get_report_precise_touch_location");

	EXPOSE_NO_HINT(FLOAT, friction);
	EXPOSE_NO_HINT(FLOAT, bounciness);
	EXPOSE_NO_HINT(FLOAT, mass);
	EXPOSE_NO_HINT(FLOAT, linear_damping);
	EXPOSE_NO_HINT(FLOAT, angular_damping);
	EXPOSE_NO_HINT(FLOAT, max_linear_velocity);
	EXPOSE_NO_HINT(FLOAT, max_angular_velocity_degree);
	EXPOSE_NO_HINT(FLOAT, gravity_scale);
	EXPOSE_NO_HINT(FLOAT, use_ccd);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "linear_velocity"), "set_linear_velocity", "get_linear_velocity");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "angular_velocity"), "set_angular_velocity", "get_angular_velocity");

	// SENSOR
	ADD_GROUP("SENSOR", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "kinematic_detect_static"), "set_kinematic_detect_static", "get_kinematic_detect_static");

	ADD_SIGNAL(MethodInfo("start_updating"));
	ADD_SIGNAL(MethodInfo("stop_updating"));

	ADD_SIGNAL(MethodInfo("body_created"));
	ADD_SIGNAL(MethodInfo("body_destroyed"));
	ADD_SIGNAL(MethodInfo("overlap_start", PropertyInfo(Variant::INT, "other_body_id"), PropertyInfo(Variant::OBJECT, "other_body")));
	ADD_SIGNAL(MethodInfo("overlap_end", PropertyInfo(Variant::INT, "other_body_id"), PropertyInfo(Variant::OBJECT, "other_body")));
	ADD_SIGNAL(MethodInfo("body_entered_trigger", PropertyInfo(Variant::OBJECT, "other_body")));
	ADD_SIGNAL(MethodInfo("body_exited_trigger", PropertyInfo(Variant::OBJECT, "other_body")));
}

bool JBody3D::_set(const StringName &p_name, const Variant &p_property) {
	if ("layer_name" == p_name) {
		const StringName ln = p_property;
		set_layer_name(ln);
		return true;
	} else {
		return false;
	}
}

bool JBody3D::_get(const StringName &p_name, Variant &r_property) const {
	if ("layer_name" == p_name) {
		r_property = get_layer_name();
		return true;
	} else {
		return false;
	}
}

void JBody3D::_get_property_list(List<PropertyInfo> *p_list) const {
	ADD_GROUP("", "");
	p_list->push_back(PropertyInfo(Variant::STRING_NAME, "layer_name", PROPERTY_HINT_ENUM_SUGGESTION, Jolt::singleton()->get_layers_comma_separated()));
}

#undef __CLASS__

JBody3D::JBody3D() :
		Node3D() {
	set_notify_local_transform(true);
	set_notify_transform(true);
	_cast_body_shape_cast_settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideWithAll;
	_cast_body_shape_cast_settings.mReturnDeepestPoint = true;
	_ignore_self_index_and_seq.resize(1);
}

JBody3D::~JBody3D() {
	__destroy_body();

	Jolt::singleton()->world_unregister_sensor(this);
	Jolt::singleton()->notify_body_3d_destroy(this);
}

JPH::BodyID JBody3D::get_jph_body_id() const {
	if (body) {
		return body->GetID();
	}
	return JPH::BodyID();
}

class JPH::Body *JBody3D::get_jph_body() {
	return body;
}

void JBody3D::set_scaled_shape(JPH::ShapeRefC p_shape, const JPH::Vec3 &p_applied_scale) {
	scaled_shape = p_shape;
	applied_scale = p_applied_scale;
}

JPH::ShapeRefC JBody3D::get_scaled_shape() const {
	return scaled_shape;
}

const JPH::Vec3 &JBody3D::get_applied_scale() const {
	return applied_scale;
}

void JBody3D::set_ignore_state_sync(bool p_ignore) {
	ignore_state_sync = p_ignore;
}

bool JBody3D::get_ignore_state_sync() const {
	return ignore_state_sync;
}

void JBody3D::jolt_set_state(const Vector<uint8_t> &p_state_data) {
	last_jolt_set_state = p_state_data;
	notify_received_net_sync_update();

	jolt_state.set_data(p_state_data);
	jolt_state.BeginRead();

	if (p_state_data.size() <= 0) {
		return;
	}

	if (ignore_state_sync) {
		return;
	}

	// Extract the layer and body_mode
	uint8_t layer_and_body_mode;
	jolt_state.Read(layer_and_body_mode);
	jolt_set_combined_layer_and_body_mode(layer_and_body_mode);

	if (!body && has_desired_body_id()) {
		create_body();
	}

	const bool print_and_error = false;
	if (print_and_error) {
		ERR_FAIL_COND_MSG(!body, "This client doesn't have any jolt body. This is not supposed to happen: " + get_path());
	} else {
		if (!body) {
			return;
		}
	}

	CRASH_COND_MSG(body->GetMotionPropertiesUnchecked() == nullptr, "In The Mirror all the bodies are created with the Motion Properties: to simplify the state set/reset.");

	Jolt::singleton()->body_restore_state(*body, jolt_state);

	// Update the Godot transform.
	const Transform3D t(Quaternion(convert(body->GetRotation())), convert_r(body->GetPosition()));
	set_global_transform(t);
}

Vector<uint8_t> JBody3D::jolt_get_state() {
	jolt_state.Clear();

	if (!body) {
		return last_jolt_set_state;
	}

	CRASH_COND_MSG(body->GetMotionPropertiesUnchecked() == nullptr, "In The Mirror all the bodies are created with the Motion Properties: to simplify the state set/reset.");

	const uint8_t layer_and_body_mode = jolt_get_combined_layer_and_body_mode();
	jolt_state.Write(layer_and_body_mode);

	Jolt::singleton()->body_save_state(*body, jolt_state);

	Vector<uint8_t> state_data = jolt_state.get_data();
	return state_data;
}

uint8_t JBody3D::jolt_get_combined_layer_and_body_mode() const {
	uint8_t layer_and_body_mode = Jolt::singleton()->get_layer_from_layer_name(layer_name);
	// Make room for the body mode.
	layer_and_body_mode = layer_and_body_mode << 3;
	// Store the body mode
	layer_and_body_mode |= static_cast<uint8_t>(body_mode);
	return layer_and_body_mode;
}

void JBody3D::jolt_set_combined_layer_and_body_mode(uint8_t p_layer_and_body_mode) {
	set_body_mode(static_cast<BodyMode>(p_layer_and_body_mode & 0x7));
	const int layer_index = p_layer_and_body_mode >> 3;
	// Set the layer name
	set_layer_name(Jolt::singleton()->get_layers_table()->get_layer_name(layer_index));
}

uint32_t JBody3D::get_body_id() const {
	if (body) {
		return body->GetID().GetIndex();
	}
	return JPH::BodyID::cInvalidBodyID;
}

bool JBody3D::has_body_id() const {
	if (body) {
		return !body->GetID().IsInvalid();
	}
	return false;
}

void JBody3D::set_desired_body_id(uint32_t p_body_id) {
	desired_body_id = JPH::BodyID(p_body_id);
	__update_body();
}

uint32_t JBody3D::get_desired_body_id() const {
	return desired_body_id.GetIndex();
}

JPH::BodyID JBody3D::get_jph_desired_body_id() const {
	return desired_body_id;
}

bool JBody3D::has_desired_body_id() const {
	return !desired_body_id.IsInvalid();
}

void JBody3D::set_body_mode(BodyMode p_mode) {
	if (p_mode == body_mode) {
		return;
	}

	body_mode = p_mode;

	// Update the body.
	if (body) {
		if (get_body_motion_type() == JPH::EMotionType::Dynamic) {
			if (!shape.is_null()) {
				if (!shape->is_convex()) {
					// Only convex shapes can be dynamic.
					body_mode = BodyMode::STATIC;
				}
			}
		}

		Jolt::singleton()->body_set_motion_type(
				body->GetID(),
				get_body_motion_type());

		Jolt::singleton()->body_set_use_CCD(
				get_jph_body_id(),
				should_use_ccd());

		if (is_sensor() != body->IsSensor()) {
			body->SetIsSensor(is_sensor());
		}

		// Reset the mass, to make sure it's properly initialized.
		set_mass(mass);

		if (is_dynamic()) {
			activate();
		}

		body->GetMotionPropertiesUnchecked()->ResetForce();
		body->GetMotionPropertiesUnchecked()->ResetTorque();
		body->GetMotionPropertiesUnchecked()->SetAngularVelocity(JPH::Vec3::sZero());
		body->GetMotionPropertiesUnchecked()->SetLinearVelocity(JPH::Vec3::sZero());
	}

	if (is_sensor()) {
		Jolt::singleton()->world_register_sensor(this);
	} else {
		Jolt::singleton()->world_unregister_sensor(this);
	}
}

JBody3D::BodyMode JBody3D::get_body_mode() const {
	return body_mode;
}

JPH::EMotionType JBody3D::get_body_motion_type() const {
	JPH::EMotionType mt = JPH::EMotionType::Static;
	if (body_mode == BodyMode::STATIC) {
		mt = JPH::EMotionType::Static;
	} else if (body_mode == BodyMode::KINEMATIC) {
		mt = JPH::EMotionType::Kinematic;
	} else if (body_mode == BodyMode::DYNAMIC) {
		mt = JPH::EMotionType::Dynamic;
	} else if (body_mode == BodyMode::SENSOR) {
		mt = JPH::EMotionType::Kinematic;
	} else if (body_mode == BodyMode::SENSOR_ONLY_ACTIVE) {
		mt = JPH::EMotionType::Static;
	}
	return mt;
}

bool JBody3D::should_use_ccd() const {
	if (!is_dynamic()) {
		// Only dynamic objects use CCD.
		return false;
	}

	const bool has_valid_shape = shape != nullptr && shape->get_shape() != nullptr;
	if (!has_valid_shape) {
		return false;
	}

	if (use_ccd) {
		return true;
	}

	// If one of the body side is smaller than 1.5m, we force using ccd.
	const JPH::Vec3 s = get_aabb().GetSize();
	const float smallest_axis = MIN(s.GetX(), MIN(s.GetY(), s.GetZ()));
	return smallest_axis <= 1.3;
}

bool JBody3D::is_dynamic() const {
	return get_body_mode() == BodyMode::DYNAMIC;
}

bool JBody3D::is_sleeping() const {
	if (body) {
		return !body->IsActive();
	}
	return true;
}

bool JBody3D::is_static() const {
	return get_body_mode() == BodyMode::STATIC || get_body_mode() == BodyMode::SENSOR_ONLY_ACTIVE;
}

bool JBody3D::is_kinematic() const {
	return get_body_mode() == BodyMode::KINEMATIC;
}

bool JBody3D::is_sensor() const {
	return get_body_mode() == BodyMode::SENSOR || get_body_mode() == BodyMode::SENSOR_ONLY_ACTIVE;
}

void JBody3D::set_layer_name(const StringName &p_layer_name) {
	if (layer_name == p_layer_name) {
		return;
	}

	layer_name = p_layer_name;

	if (body) {
		Jolt::singleton()->body_set_layer(
				body->GetID(),
				Jolt::singleton()->get_layer_from_layer_name(p_layer_name));
	}
}

StringName JBody3D::get_layer_name() const {
	return layer_name;
}

void JBody3D::set_shape(Ref<JShape3D> p_shape) {
	if (shape == p_shape) {
		return;
	}

	if (shape != nullptr) {
		shape->remove_body(*this);
	}

	shape = p_shape;

	if (shape != nullptr) {
		shape->add_body(*this);
	}

	__update_body();
}

Ref<JShape3D> JBody3D::get_shape() const {
	return shape;
}

void JBody3D::on_shape_changed() {
	__update_body();
	shape_with_margin = nullptr;
}

void JBody3D::set_allow_sleeping(bool p_allow_sleeping) {
	allow_sleeping = p_allow_sleeping;
	if (body) {
		body->SetAllowSleeping(p_allow_sleeping);
	}
}

bool JBody3D::get_allow_sleeping() const {
	return allow_sleeping;
}

void JBody3D::set_report_precise_touch_location(bool p_report_precise_touch_location) {
	report_precise_touch_location = p_report_precise_touch_location;
	if (body) {
		body->SetUseManifoldReduction(!p_report_precise_touch_location);
	}
}

bool JBody3D::get_report_precise_touch_location() const {
	return report_precise_touch_location;
}

void JBody3D::set_friction(const real_t p_friction) {
	friction = p_friction;
	if (body) {
		body->SetFriction(p_friction);
	}
}

real_t JBody3D::get_friction() const {
	return friction;
}

void JBody3D::set_bounciness(const real_t p_bounciness) {
	bounciness = p_bounciness;
	if (body) {
		body->SetRestitution(p_bounciness);
	}
}

real_t JBody3D::get_bounciness() const {
	return bounciness;
}

void JBody3D::set_mass(real_t p_mass) {
	mass = p_mass;
	mass = MAX(0.001, mass);
	if (body && shape.is_valid() && shape->get_shape()) {
		// Updates the mass
		const JPH::EAllowedDOFs allowedDOFs = JPH::EAllowedDOFs::All;
		JPH::MassProperties mp = shape->get_shape()->GetMassProperties();
		mp.ScaleToMass(mass * MASS_FACTOR);
		body->GetMotionPropertiesUnchecked()->SetMassProperties(allowedDOFs, mp);
	}
}

real_t JBody3D::get_mass() const {
	return mass;
}

void JBody3D::set_linear_damping(real_t p_ld) {
	linear_damping = p_ld;
	if (body) {
		body->GetMotionPropertiesUnchecked()->SetLinearDamping(linear_damping);
	}
}

real_t JBody3D::get_linear_damping() const {
	return linear_damping;
}

void JBody3D::set_angular_damping(real_t p_ad) {
	angular_damping = p_ad;
	if (body) {
		body->GetMotionPropertiesUnchecked()->SetAngularDamping(angular_damping);
	}
}

real_t JBody3D::get_angular_damping() const {
	return angular_damping;
}

void JBody3D::set_max_linear_velocity(real_t p_mlv) {
	max_linear_velocity = p_mlv;
	if (body) {
		body->GetMotionPropertiesUnchecked()->SetMaxLinearVelocity(max_linear_velocity);
	}
}

real_t JBody3D::get_max_linear_velocity() const {
	return max_linear_velocity;
}

void JBody3D::set_max_angular_velocity_degree(real_t p_mavd) {
	max_angular_velocity_degree = p_mavd;
	if (body) {
		body->GetMotionPropertiesUnchecked()->SetMaxAngularVelocity(Math::deg_to_rad(max_angular_velocity_degree));
	}
}

real_t JBody3D::get_max_angular_velocity_degree() const {
	return max_angular_velocity_degree;
}

void JBody3D::set_gravity_scale(real_t p_gravity_scale) {
	gravity_scale = p_gravity_scale;
	if (body) {
		body->GetMotionPropertiesUnchecked()->SetGravityFactor(gravity_scale);
	}
}

real_t JBody3D::get_gravity_scale() const {
	return gravity_scale;
}

void JBody3D::set_use_ccd(bool p_use_ccd) {
	use_ccd = p_use_ccd;
	if (body) {
		Jolt::singleton()->body_set_use_CCD(get_jph_body_id(), should_use_ccd());
	}
}

bool JBody3D::get_use_ccd() const {
	return use_ccd;
}

void JBody3D::set_kinematic_detect_static(bool p_can) {
	kinematic_detect_static = p_can;
	if (body) {
		body->SetCollideKinematicVsNonDynamic(kinematic_detect_static);
	}
}

bool JBody3D::get_kinematic_detect_static() const {
	return kinematic_detect_static;
}

void JBody3D::notify_received_net_sync_update() {
	last_received_update_timestamp = OS::get_singleton()->get_ticks_msec();
}

bool JBody3D::is_net_sync_updating() const {
	return net_sync_updating;
}

JPH::AABox JBody3D::get_aabb() const {
	if (body) {
		return body->GetWorldSpaceBounds();
	} else {
		JPH::AABox b;
		b.SetEmpty();
		return b;
	}
}

void JBody3D::add_force_gd(const Vector3 &p_force) {
	add_force(convert(p_force));
}

void JBody3D::add_force_at_position_gd(const Vector3 &p_force, const Vector3 &p_position) {
	add_force_at_position(convert(p_force), convert_r(p_position));
}

void JBody3D::add_force(const JPH::Vec3 &p_force) {
	ERR_FAIL_COND(body == nullptr);
	ERR_FAIL_COND_MSG(Jolt::singleton()->world_is_processing(0), "You can't use this API while the physics is processing.");
	if (body->GetMotionPropertiesUnchecked()) {
		body->AddForce(p_force);
		activate();
	}
}

void JBody3D::add_force_at_position(const JPH::Vec3 &p_force, const JPH::RVec3 &p_position) {
	ERR_FAIL_COND(body == nullptr);
	ERR_FAIL_COND_MSG(Jolt::singleton()->world_is_processing(0), "You can't use this API while the physics is processing.");
	if (body->GetMotionPropertiesUnchecked()) {
		body->AddForce(p_force, p_position);
		activate();
	}
}

void JBody3D::add_torque_gd(const Vector3 &p_torque) {
	add_torque(convert(p_torque));
}

void JBody3D::add_torque(const JPH::Vec3 &p_torque) {
	ERR_FAIL_COND(body == nullptr);
	ERR_FAIL_COND_MSG(Jolt::singleton()->world_is_processing(0), "You can't use this API while the physics is processing.");
	if (body->GetMotionPropertiesUnchecked()) {
		body->AddTorque(p_torque);
		activate();
	}
}

void JBody3D::add_impulse_gd(const Vector3 &p_force) {
	add_impulse(convert(p_force));
}

void JBody3D::add_impulse_at_position_gd(const Vector3 &p_force, const Vector3 &p_position) {
	add_impulse_at_position(convert(p_force), convert_r(p_position));
}

void JBody3D::add_impulse(const JPH::Vec3 &p_impulse) {
	ERR_FAIL_COND(body == nullptr);
	ERR_FAIL_COND_MSG(Jolt::singleton()->world_is_processing(0), "You can't use this API while the physics is processing.");
	if (body->GetMotionPropertiesUnchecked()) {
		activate();
		body->AddImpulse(p_impulse);
	}
}

void JBody3D::add_impulse_at_position(const JPH::Vec3 &p_impulse, const JPH::RVec3 &p_position) {
	ERR_FAIL_COND(body == nullptr);
	ERR_FAIL_COND_MSG(Jolt::singleton()->world_is_processing(0), "You can't use this API while the physics is processing.");
	if (body->GetMotionPropertiesUnchecked()) {
		activate();
		body->AddImpulse(p_impulse, p_position);
	}
}

void JBody3D::add_impulse_angular_gd(const Vector3 &p_impulse) {
	add_impulse_angular(convert(p_impulse));
}

void JBody3D::add_impulse_angular(const JPH::Vec3 &p_impulse) {
	ERR_FAIL_COND(body == nullptr);
	ERR_FAIL_COND_MSG(Jolt::singleton()->world_is_processing(0), "You can't use this API while the physics is processing.");
	if (body->GetMotionPropertiesUnchecked()) {
		body->AddAngularImpulse(p_impulse);
		activate();
	}
}

void JBody3D::set_linear_velocity_gd(const Vector3 &p_velocity) {
	set_linear_velocity(convert(p_velocity));
}

Vector3 JBody3D::get_linear_velocity_gd() const {
	return convert(get_linear_velocity());
}

void JBody3D::set_linear_velocity(const JPH::Vec3 &p_velocity) {
	linear_velocity = p_velocity;

	if (body == nullptr) {
		return;
	}

	ERR_FAIL_COND_MSG(Jolt::singleton()->world_is_processing(0), "You can't use this API while the physics is processing.");
	body->SetLinearVelocityClamped(p_velocity);
	if (p_velocity != JPH::Vec3::sZero()) {
		activate();
	}
}

JPH::Vec3 JBody3D::get_linear_velocity() const {
	if (body == nullptr) {
		return linear_velocity;
	}

	ERR_FAIL_COND_V_MSG(Jolt::singleton()->world_is_processing(0), linear_velocity, "You can't use this API while the physics is processing.");
	return body->GetLinearVelocity();
}

void JBody3D::set_angular_velocity_gd(const Vector3 &p_velocity) {
	set_angular_velocity(convert(p_velocity));
}

Vector3 JBody3D::get_angular_velocity_gd() const {
	return convert(get_angular_velocity());
}

void JBody3D::set_angular_velocity(const JPH::Vec3 &p_velocity) {
	if (body == nullptr) {
		return;
	}

	ERR_FAIL_COND_MSG(Jolt::singleton()->world_is_processing(0), "You can't use this API while the physics is processing.");
	body->SetAngularVelocityClamped(p_velocity);
	if (p_velocity != JPH::Vec3::sZero()) {
		activate();
	}
}

JPH::Vec3 JBody3D::get_angular_velocity() const {
	if (body == nullptr) {
		return JPH::Vec3::sZero();
	}
	ERR_FAIL_COND_V_MSG(Jolt::singleton()->world_is_processing(0), JPH::Vec3::sZero(), "You can't use this API while the physics is processing.");
	return body->GetAngularVelocity();
}

void JBody3D::activate() {
	if (body == nullptr) {
		return;
	}

	if ((body->IsDynamic() || body->IsKinematic()) && !body->IsActive()) {
		Jolt::singleton()->body_activate(body->GetID());
	}
}

JPH::ShapeCastResult &_get_max_penetration_hit(std::vector<JPH::ShapeCastResult> &p_hits) {
	JPH::ShapeCastResult &max_penetration_hit = p_hits[0];
	for (size_t i = 1; i < p_hits.size(); i++) {
		if (p_hits[i].mPenetrationDepth > max_penetration_hit.mPenetrationDepth) {
			max_penetration_hit = p_hits[i];
		}
	}
	return max_penetration_hit;
}

JPH::ShapeCastResult &_get_nearest_hit(std::vector<JPH::ShapeCastResult> &p_hits) {
	JPH::ShapeCastResult &nearest_hit = p_hits[0];
	for (size_t i = 1; i < p_hits.size(); i++) {
		if (p_hits[i].mFraction < nearest_hit.mFraction) {
			nearest_hit = p_hits[i];
		}
	}
	return nearest_hit;
}

void JBody3D::_move(const Vector3 &p_movement) {
	set_global_position(get_global_position() + p_movement);
}

void JBody3D::move_and_collide(const Vector3 &p_movement, Array p_collide_with_layers) {
	JPH::ShapeRefC shape_ref = shape->get_shape();
	JQueryVectorResult<JPH::CastShapeCollector> result;

	LocalVector<JPH::ObjectLayer> layers;
	for (int i = 0; i < p_collide_with_layers.size(); i++) {
		const JPH::ObjectLayer layer = Jolt::singleton()->get_layer_from_layer_name(p_collide_with_layers[i]);
		layers.push_back(layer);
	}

	Jolt::singleton()->cast_shape(
			0,
			shape_ref,
			body->GetWorldTransform(),
			convert(p_movement),
			layers,
			result,
			_ignore_self_index_and_seq, _cast_body_shape_cast_settings);
	if (!result.has_hits()) {
		_move(p_movement);
		return;
	}
	JPH::ShapeCastResult &nearest_hit = _get_nearest_hit(result.hits);
	_move(p_movement * nearest_hit.mFraction);
}

void JBody3D::move_kinematic(const Vector3 &p_target_position, const Basis &p_target_rotation, real_t p_time) {
	if (body && body->IsKinematic()) {
		const JPH::RVec3 target_position = convert_r(p_target_position);
		const JPH::Quat target_rotation = convert(p_target_rotation.get_rotation_quaternion());
		activate();
		body->MoveKinematic(
				target_position,
				target_rotation,
				p_time);
	}
}

Array JBody3D::cast_body_gd(const Vector3 &p_move, Array p_collide_with_layers) {
	JQueryVectorResult<JPH::CastShapeCollector> result;
	cast_body(convert(p_move), result, p_collide_with_layers);

	// convert to Dictionary
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

void JBody3D::cast_body(
		const JPH::Vec3 &p_move,
		JQueryVectorResult<JPH::CastShapeCollector> &r_result,
		Array p_collide_with_layers) {
	if (body == nullptr) {
		return;
	}
	if (!shape.is_valid() || shape->get_shape() == nullptr) {
		return;
	}

	LocalVector<JPH::ObjectLayer> layers;
	for (int i = 0; i < p_collide_with_layers.size(); i++) {
		const JPH::ObjectLayer layer = Jolt::singleton()->get_layer_from_layer_name(p_collide_with_layers[i]);
		layers.push_back(layer);
	}

	Jolt::singleton()->cast_shape(
			0,
			shape->get_shape(),
			body->GetWorldTransform(),
			p_move,
			layers,
			r_result,
			_ignore_self_index_and_seq,
			_cast_body_shape_cast_settings);
}

Array JBody3D::collide_body_gd(
		Array p_collide_with_layers,
		float p_separation_distance) {
	return collide_body_with_position_gd(
			get_global_transform(),
			p_collide_with_layers,
			p_separation_distance);
}

Array JBody3D::collide_body_with_position_gd(
		const Transform3D &p_position,
		Array p_collide_with_layers,
		float p_separation_distance) {
	JQueryVectorResult<JPH::CollideShapeCollector> result;

	collide_body_with_position(
			convert_r(p_position),
			result,
			p_collide_with_layers);

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

void JBody3D::collide_body(
		JQueryVectorResult<JPH::CollideShapeCollector> &r_result,
		Array p_collide_with_layers,
		float p_separation_distance) {
	collide_body_with_position(
			body->GetWorldTransform(),
			r_result,
			p_collide_with_layers);
}

void JBody3D::collide_body_with_position(
		const JPH::RMat44 &p_position,
		JQueryVectorResult<JPH::CollideShapeCollector> &r_result,
		Array p_collide_with_layers,
		float p_separation_distance) {
	if (body == nullptr) {
		return;
	}
	if (!shape.is_valid() || shape->get_shape() == nullptr) {
		return;
	}

	collide_body_with_position_and_shape(
			body->GetWorldTransform(),
			shape->get_shape(),
			r_result,
			p_collide_with_layers);
}

void JBody3D::collide_body_with_position_and_shape(
		const JPH::RMat44 &p_position,
		JPH::ShapeRefC p_shape,
		JQueryVectorResult<JPH::CollideShapeCollector> &r_result,
		Array p_collide_with_layers,
		float p_separation_distance) {
	if (body == nullptr) {
		return;
	}

	LocalVector<uint32_t> bodies_to_ignore;
	bodies_to_ignore.push_back(body->GetID().GetIndexAndSequenceNumber());

	JPH::CollideShapeSettings shape_cast_settings;
	shape_cast_settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideWithAll;
	shape_cast_settings.mMaxSeparationDistance = p_separation_distance;

	LocalVector<JPH::ObjectLayer> layers;
	for (int i = 0; i < p_collide_with_layers.size(); i++) {
		const JPH::ObjectLayer layer = Jolt::singleton()->get_layer_from_layer_name(p_collide_with_layers[i]);
		layers.push_back(layer);
	}

	Jolt::singleton()->collide_shape(
			0,
			p_shape,
			p_position,
			layers,
			r_result,
			bodies_to_ignore,
			shape_cast_settings);
}

void JBody3D::debug_draw_point(const JPH::RVec3 &p_location, const JPH::Vec3 &p_color) {
	const Color color(p_color[0], p_color[1], p_color[2], 1.0);
	call("_debug_draw_point", Vector3(convert_r(p_location)), color);
}

void JBody3D::debug_draw_sphere(const JPH::RVec3 &p_location, float p_radius, const JPH::Vec3 &p_color) {
	const Color color(p_color[0], p_color[1], p_color[2], 1.0);
	call("_debug_draw_sphere", Vector3(convert_r(p_location)), p_radius, color);
}

void JBody3D::debug_draw_line(const JPH::RVec3 &p_start, const JPH::RVec3 &p_end, float p_thickness, const JPH::Vec3 &p_color) {
	const Color color(p_color[0], p_color[1], p_color[2], 1.0);
	call("_debug_draw_line", Vector3(convert_r(p_start)), Vector3(convert_r(p_end)), p_thickness, color);
}

void JBody3D::create_body() {
	if (body && body->GetID() == desired_body_id) {
		// Nothing to do.
		return;
	}

	if (shape != nullptr && shape->get_shape() != nullptr) {
		__create_body();
	}
}

real_t JBody3D::get_enclosing_radius() {
	if (body == nullptr) {
		return 0.0;
	}
	return body->GetShape()->GetInnerRadius();
}

void JBody3D::sensor_mark_body_as_overlap(JBody3D *p_body, bool p_access_from_thread) {
	if (p_access_from_thread) {
		sensor_overlap_mutex.lock();
	}

	const int index = overlaps.find({ p_body });
	if (index >= 0) {
		if (overlaps[index].status != OVERLAP_STATUS_START) {
			// The overlap was already known, mark as inside.
			overlaps[index].status = OVERLAP_STATUS_INSIDE;
		} else {
			// The overlap was alrady detected on this frame, so nothing to do.
			// This happens when more than 1 contacts are being registered between
			// the sensor and the body.
		}
	} else {
		// This is a new overlap.
		overlaps.push_back({ p_body, p_body->get_jph_body_id(), OVERLAP_STATUS_START });
	}

	if (p_access_from_thread) {
		sensor_overlap_mutex.unlock();
	}
}

void JBody3D::sensor_notify_overlap_events() {
	for (int i = overlaps.size() - 1; i >= 0; i--) {
		SensorOverlap &o = overlaps[i];
		if (o.status == OVERLAP_STATUS_START) {
			emit_signal("overlap_start", o.id.GetIndex(), o.body);
			emit_signal("body_entered_trigger", o.body);
		} else if (o.status == OVERLAP_STATUS_END) {
			// At this point the "body" may have gone, so it's better to fetch
			// the body using it's ID.
			JBody3D *other_body = Jolt::singleton()->body_fetch(o.id);
			emit_signal("overlap_end", o.id.GetIndex(), other_body);
			emit_signal("body_exited_trigger", other_body);
			// Remove this overlap.
			overlaps.remove_at_unordered(i);
			continue;
		}

		// Set the overlap status as `end` right away, instead to wait for the next frame.
		// This step is needed so we can detect the ended overlaps, on the next frame.
		o.status = OVERLAP_STATUS_END;
	}
}

void JBody3D::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		__update_body();
		if (!Engine::get_singleton()->is_editor_hint()) {
			set_process_internal(true);
		}

	} else if (p_what == NOTIFICATION_INTERNAL_PROCESS) {
		bool updated_on_this_frame = false;
		if (TMSceneSync::singleton()->is_server()) {
			// Never deactivate on the server.
			updated_on_this_frame = true;
		} else if (is_dynamic() || is_kinematic()) {
			const uint64_t now = OS::get_singleton()->get_ticks_msec();
			const uint64_t long_time = 10 * 1000; // 10 seconds.
			if ((now - last_received_update_timestamp) < long_time) {
				updated_on_this_frame = true;
			} else if (net_sync_updating) {
				updated_on_this_frame = TMSceneSync::singleton()->client_is_object_simulating(this);
			}
		} else {
			updated_on_this_frame = true;
		}

		if (updated_on_this_frame != net_sync_updating) {
			// Trigger the event
			net_sync_updating = updated_on_this_frame;
			if (net_sync_updating) {
				emit_signal("start_updating");
			} else {
				emit_signal("stop_updating");
			}
		}

	} else if (p_what == NOTIFICATION_LOCAL_TRANSFORM_CHANGED ||
			p_what == NOTIFICATION_TRANSFORM_CHANGED) {
		if (Engine::get_singleton()->is_editor_hint()) {
			return;
		}
		if (body == nullptr) {
			return;
		}

		__update_jolt_transform();
	}
}

void JBody3D::__update_body() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	if (!is_ready()) {
		return;
	}

	const bool has_valid_shape = shape != nullptr && shape->get_shape() != nullptr;
	if (has_valid_shape) {
		if (body) {
			if (has_desired_body_id() && desired_body_id != body->GetID()) {
				// The desired body id is different, so recreate the body.
				__destroy_body();
				__create_body();
			} else {
				Jolt::singleton()->body_set_shape(this, shape->get_shape(), is_dynamic());
				Jolt::singleton()->body_set_use_CCD(get_jph_body_id(), should_use_ccd());
			}
		} else {
			__create_body();
		}
	} else {
		__destroy_body();
	}
}

void JBody3D::__destroy_body() {
	if (body) {
		if (is_sensor()) {
			Jolt::singleton()->world_unregister_sensor(this);
		}

		Jolt::singleton()->body_destroy(body);
		body = nullptr;
		emit_signal("body_destroyed");
	}
}

void JBody3D::__create_body() {
	CRASH_COND(body != nullptr);
	CRASH_COND(shape == nullptr);
	CRASH_COND(shape->get_shape() == nullptr);
	body = Jolt::singleton()->create_body(
			this,
			0,
			desired_body_id,
			Jolt::singleton()->get_layer_from_layer_name(layer_name),
			shape->get_shape(),
			get_global_transform(),
			friction,
			bounciness,
			mass * MASS_FACTOR,
			is_sensor(),
			kinematic_detect_static,
			get_body_motion_type(),
			allow_sleeping,
			report_precise_touch_location,
			linear_damping,
			angular_damping,
			max_linear_velocity,
			max_angular_velocity_degree,
			gravity_scale,
			should_use_ccd());
	ERR_FAIL_COND_MSG(!body, "Failed to create the body. desired_body_id: " + itos(desired_body_id.GetIndex()));

	if (!desired_body_id.IsInvalid()) {
		CRASH_COND(desired_body_id != body->GetID());
	}

	if (is_sensor()) {
		Jolt::singleton()->world_register_sensor(this);
	} else {
		Jolt::singleton()->world_unregister_sensor(this);
	}

	CRASH_COND_MSG(body->GetMotionPropertiesUnchecked() == nullptr, "In The Mirror all the bodies are created with the Motion Properties: to simplify the state set/reset.");

	_ignore_self_index_and_seq[0] = body->GetID().GetIndexAndSequenceNumber();
	emit_signal("body_created");
}

void JBody3D::__update_godot_transform() {
	if (body->IsStatic()) {
		return;
	}

	set_notify_local_transform(false);
	set_notify_transform(false);
	set_global_transform(Jolt::singleton()->body_get_transform(this));
	set_notify_local_transform(true);
	set_notify_transform(true);
}

void JBody3D::__update_jolt_transform() {
	Jolt::singleton()->body_set_transform(
			this,
			get_global_transform());
}

void JBody3D::register_interaction(const StringName &p_name) {
	if (!interactions.has(p_name)) {
		interactions.push_back(p_name);
	}
}

int JBody3D::get_interaction_id(const StringName &p_name) const {
	return interactions.find(p_name);
}

void JBody3D::call_interaction(int p_index, const Variant &p_argument) {
	ERR_FAIL_COND(p_index < 0);
	ERR_FAIL_COND(interactions.size() <= p_index);
	call(interactions[p_index], p_argument);
}
