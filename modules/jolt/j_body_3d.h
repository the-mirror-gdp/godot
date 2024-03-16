
#ifndef J_BODY_3D_H
#define J_BODY_3D_H

#include "jolt.h"

#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "core/math/aabb.h"
#include "core/math/basis.h"
#include "godot_expose_utils.h"
#include "j_shape.h"
#include "modules/jolt/j_query_results.h"
#include "modules/jolt/state_recorder_sync.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Core/Mutex.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/physics_material.h"

class JBody3D : public Node3D {
	GDCLASS(JBody3D, Node3D);

public:
	enum BodyMode {
		STATIC,
		KINEMATIC,
		DYNAMIC,
		SENSOR,
		// The cheapest sensor. This type of sensor will only detect active bodies entering their area.
		SENSOR_ONLY_ACTIVE
	};

	enum SensorOverlapStatus {
		OVERLAP_STATUS_START,
		OVERLAP_STATUS_INSIDE,
		OVERLAP_STATUS_END
	};

	struct SensorOverlap {
		JBody3D *body = nullptr;
		JPH::BodyID id = JPH::BodyID();
		SensorOverlapStatus status = OVERLAP_STATUS_END;

		bool operator==(const SensorOverlap &o) const { return body == o.body; }
	};

private:
	Vector<StringName> interactions;

	JPH::ShapeCastSettings _cast_body_shape_cast_settings;
	LocalVector<uint32_t> _ignore_self_index_and_seq;

protected:
	// Holds the latest received jolt set state, and used as return for the get
	// function, when the state can be generated: making sure to return the
	// proper data.
	Vector<uint8_t> last_jolt_set_state;
	class JPH::Body *body = nullptr;
	JPH::ShapeRefC scaled_shape;
	JPH::Vec3 applied_scale = JPH::Vec3(1, 1, 1);
	JPH::Vec3 linear_velocity = JPH::Vec3::sZero();

	bool ignore_state_sync = false;

	JPH::BodyID desired_body_id = JPH::BodyID();

	BodyMode body_mode = BodyMode::DYNAMIC;
	StringName layer_name;
	Ref<JShape3D> shape;

	// Rigid body
	bool allow_sleeping = true;
	bool report_precise_touch_location = false;
	real_t friction = 0.2;
	real_t bounciness = 0.0;
	real_t mass = 1.0;
	real_t linear_damping = 0.05;
	real_t angular_damping = 0.05;
	real_t max_linear_velocity = 500.0;
	real_t max_angular_velocity_degree = 0.25 * 180.0 * 60.0; // quart a 180 degs per frame (at 60Hz).
	real_t gravity_scale = 1.0;
	bool use_ccd = false;

	bool kinematic_detect_static = false;

	// Shape used for kinematic move
	JPH::ShapeRefC shape_with_margin;

	StateRecorderSync jolt_state;

	JPH::Mutex sensor_overlap_mutex;
	LocalVector<SensorOverlap> overlaps;

	uint64_t last_received_update_timestamp = 0;
	bool net_sync_updating = true;

	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_property);
	bool _get(const StringName &p_name, Variant &r_property) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

private:
	void _move(const Vector3 &p_movement);

public:
	JBody3D();
	virtual ~JBody3D() override;

	class JPH::Body *get_jph_body();

	void set_scaled_shape(JPH::ShapeRefC p_shape, const JPH::Vec3 &p_applied_scale);
	JPH::ShapeRefC get_scaled_shape() const;
	const JPH::Vec3 &get_applied_scale() const;

	void set_ignore_state_sync(bool p_ignore);
	bool get_ignore_state_sync() const;

	void jolt_set_state(const Vector<uint8_t> &p_data);
	Vector<uint8_t> jolt_get_state();

	uint8_t jolt_get_combined_layer_and_body_mode() const;
	void jolt_set_combined_layer_and_body_mode(uint8_t p_layer_and_body_mode);

	JPH::BodyID get_jph_body_id() const;
	uint32_t get_body_id() const;
	bool has_body_id() const;

	void set_desired_body_id(uint32_t p_body_id);
	uint32_t get_desired_body_id() const;
	JPH::BodyID get_jph_desired_body_id() const;
	bool has_desired_body_id() const;

	void set_body_mode(BodyMode p_mode);
	BodyMode get_body_mode() const;
	JPH::EMotionType get_body_motion_type() const;
	bool should_use_ccd() const;

	bool is_dynamic() const;
	bool is_sleeping() const;
	bool is_static() const;
	bool is_kinematic() const;
	bool is_sensor() const;

	void set_layer_name(const StringName &p_layer_name);
	StringName get_layer_name() const;

	void set_shape(Ref<JShape3D> p_shape);
	Ref<JShape3D> get_shape() const;
	void on_shape_changed();

	// ------------------------------------------------------------------- RIGID
	void set_always_static(bool p_a_s);
	bool get_always_static() const;

	void set_allow_sleeping(bool p_allow_sleeping);
	bool get_allow_sleeping() const;

	void set_report_precise_touch_location(bool p_report_precise_touch_location);
	bool get_report_precise_touch_location() const;

	void set_friction(const real_t p_friction);
	real_t get_friction() const;

	void set_bounciness(const real_t p_bounciness);
	real_t get_bounciness() const;

	void set_mass(real_t p_mass);
	real_t get_mass() const;

	void set_linear_damping(real_t p_ld);
	real_t get_linear_damping() const;

	void set_angular_damping(real_t p_ad);
	real_t get_angular_damping() const;

	void set_max_linear_velocity(real_t p_mlv);
	real_t get_max_linear_velocity() const;

	void set_max_angular_velocity_degree(real_t p_mavd);
	real_t get_max_angular_velocity_degree() const;

	void set_gravity_scale(real_t p_factor);
	real_t get_gravity_scale() const;

	void set_use_ccd(bool p_use_ccd);
	bool get_use_ccd() const;

	// ------------------------------------------------------------------ SENSOR
	void set_kinematic_detect_static(bool p_can);
	bool get_kinematic_detect_static() const;

	// -------------------------------------------------------------------- APIs

	void notify_received_net_sync_update();
	bool is_net_sync_updating() const;

	JPH::AABox get_aabb() const;

	void add_force_gd(const Vector3 &p_force);
	void add_force_at_position_gd(const Vector3 &p_force, const Vector3 &p_position);
	void add_force(const JPH::Vec3 &p_force);
	void add_force_at_position(const JPH::Vec3 &p_force, const JPH::RVec3 &p_position);

	void add_torque_gd(const Vector3 &p_torque);
	void add_torque(const JPH::Vec3 &p_torque);

	void add_impulse_gd(const Vector3 &p_impulse);
	void add_impulse_at_position_gd(const Vector3 &p_impulse, const Vector3 &p_position);
	virtual void add_impulse(const JPH::Vec3 &p_impulse);
	virtual void add_impulse_at_position(const JPH::Vec3 &p_impulse, const JPH::RVec3 &p_position);

	void add_impulse_angular_gd(const Vector3 &p_impulse);
	void add_impulse_angular(const JPH::Vec3 &p_impulse);

	void set_linear_velocity_gd(const Vector3 &p_velocity);
	Vector3 get_linear_velocity_gd() const;

	void set_linear_velocity(const JPH::Vec3 &p_velocity);
	JPH::Vec3 get_linear_velocity() const;

	void set_angular_velocity_gd(const Vector3 &p_velocity);
	Vector3 get_angular_velocity_gd() const;

	void set_angular_velocity(const JPH::Vec3 &p_velocity);
	JPH::Vec3 get_angular_velocity() const;

	void activate();

	void move_and_collide(const Vector3 &p_movement, Array p_collide_with_layers);

	void move_kinematic(
			const Vector3 &p_target_position,
			const Basis &p_target_rotation,
			real_t p_time);

	Array cast_body_gd(const Vector3 &p_move, Array p_collide_with_layers);
	void cast_body(
			const JPH::Vec3 &p_move,
			JQueryVectorResult<JPH::CastShapeCollector> &r_result,
			Array p_collide_with_layers);

	Array collide_body_gd(
			Array p_collide_with_layers,
			float p_separation_distance = 0.0);

	Array collide_body_with_position_gd(
			const Transform3D &p_position,
			Array p_collide_with_layers,
			float p_separation_distance);

	void collide_body(
			JQueryVectorResult<JPH::CollideShapeCollector> &r_result,
			Array p_collide_with_layers,
			float p_separation_distance = 0.0);

	void collide_body_with_position(
			const JPH::RMat44 &p_position,
			JQueryVectorResult<JPH::CollideShapeCollector> &r_result,
			Array p_collide_with_layers,
			float p_separation_distance = 0.0);

	void collide_body_with_position_and_shape(
			const JPH::RMat44 &p_position,
			JPH::ShapeRefC p_shape,
			JQueryVectorResult<JPH::CollideShapeCollector> &r_result,
			Array p_collide_with_layers,
			float p_separation_distance = 0.0);

	virtual void debug_draw_point(const JPH::RVec3 &p_location, const JPH::Vec3 &p_color = JPH::Vec3(1, 0, 0));
	virtual void debug_draw_sphere(const JPH::RVec3 &p_location, float p_radius, const JPH::Vec3 &p_color = JPH::Vec3(1, 0, 0));
	virtual void debug_draw_line(const JPH::RVec3 &p_start, const JPH::RVec3 &p_end, float p_thickness, const JPH::Vec3 &p_color = JPH::Vec3(1, 0, 0));

	/// Creates the body, does nothing if the body exists.
	void create_body();

	real_t get_enclosing_radius();

public: // ------------------------------------------------------- INTERNAL APIs
	void sensor_mark_body_as_overlap(JBody3D *p_body, bool p_access_from_thread);
	void sensor_notify_overlap_events();

protected:
	void _notification(int p_what);

public:
	virtual void __update_body();
	virtual void __destroy_body();
	virtual void __create_body();

	// This function is called by Jolt.
	virtual void __update_godot_transform();
	virtual void __update_jolt_transform();

	void register_interaction(const StringName &p_name);
	int get_interaction_id(const StringName &p_name) const;
	void call_interaction(int p_index, const Variant &p_argument);
};

VARIANT_ENUM_CAST(JBody3D::BodyMode);

#endif // J_BODY_3D_H
