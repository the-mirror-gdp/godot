
#ifndef JOLT_H
#define JOLT_H

#include "modules/jolt/j_shape.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Math/Real.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Character/CharacterVirtual.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Collision/RayCast.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Collision/ShapeCast.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/StateRecorder.h"

#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "table_broad_phase_layer.h"

namespace JPH {
class PhysicsSystem;
class TempAllocator;
class JobSystem;
}; //namespace JPH

class TMCharacterVirtual;

class Jolt : public Object {
	GDCLASS(Jolt, Object);

public:
private:
	static Jolt *the_singleton;

public:
	static Jolt *singleton();

	static void _bind_methods();

public:
	class JBodyIdManager *body_id_manager = nullptr;
	int max_bodies = 0;

private:
	Ref<JLayersTable> layers_table;
	TableObjectLayerPairFilter object_layer_pair_filter;
	TableObjectVsBroadPhaseLayerFilter object_vs_broad_phase_layer_filter;
	TableBroadPhaseLayer broad_phase_layer;

public:
	std::vector<bool> worlds_need_broadphase_opt;
	LocalVector<JPH::PhysicsSystem *> worlds;
	LocalVector<class JContactListener *> contact_listeners;
	LocalVector<JPH::TempAllocator *> worlds_temp_allocators;
	LocalVector<JPH::JobSystem *> worlds_job_systems;

	JPH::TempAllocatorMalloc character_virtual_tmp_allocator;

	LocalVector<class JBody3D *> sensors_list;

public:
	Jolt();
	~Jolt();

	void __init();

	void set_body_id_manager(class JBodyIdManager *p_manager);
	class JBodyIdManager *get_body_id_manager() const;

	void set_layers_table(const Ref<JLayersTable> &p_layers_table);
	Ref<JLayersTable> get_layers_table() const;

	String get_layers_comma_separated() const;

	/// Create a world.
	/// @param MaxBodies ............. Maximum number of bodies to support.
	/// @param NumBodyMutexes ........ Number of body mutexes to use. Should be a power of 2 in the range [1, 64], use 0 to auto detect.
	/// @param MaxBodyPairs .......... Maximum amount of body pairs to process (anything else will fall through the world),
	///                                this number should generally be much higher than the max amount of contact points as
	///                                there will be lots of bodies close that are not actually touching.
	/// @param MaxContactConstraints . Maximum amount of contact constraints to process (anything else will fall through the world).
	uint32_t create_world(uint32_t p_max_bodies, uint32_t p_num_body_mutexes, uint32_t p_max_body_pairs, uint32_t p_max_contact_constraints);
	void world_destroy(uint32_t p_index);

	JContactListener *get_contact_listener(uint32_t p_world_id);
	const JContactListener *get_contact_listener(uint32_t p_world_id) const;

private:
	void __destroy_world(uint32_t p_index);

public:
	JPH::ObjectLayer get_layer_from_layer_name(const StringName &p_layer) const;

	void world_register_sensor(JBody3D *p_sensor);
	void world_unregister_sensor(JBody3D *p_sensor);

	void world_process(uint32_t p_world_id, double p_delta);
	void world_sync(uint32_t p_world_id, bool p_force_all = false);
	void world_sync_overlap_events();

	bool world_is_processing(uint32_t p_world_id) const;

	void world_get_state(uint32_t p_world_id, class StateRecorderSync &p_state, JPH::EStateRecorderState p_recorder_state, const JPH::StateRecorderFilter *p_ilter);
	void world_set_state(uint32_t p_world_id, class StateRecorderSync &p_state, JPH::EStateRecorderState p_recorder_state);

	class JPH::Body *create_body(
			class JBody3D *p_owner,
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
			bool p_allow_sleep,
			bool p_report_precise_touch_location,
			real_t p_linear_damping,
			real_t p_angular_damping,
			real_t p_max_linear_velocity,
			real_t p_max_angular_velocity_deg,
			real_t p_gravity_scale,
			bool p_use_ccd);

	void body_destroy(JPH::Body *p_body, bool p_wakesup_near_objects = true);
	void notify_body_3d_destroy(class JBody3D *p_body);

	void body_save_state(JPH::Body &p_body, StateRecorderSync &inStream);
	void body_restore_state(JPH::Body &p_body, StateRecorderSync &inStream);

	class JBody3D *body_fetch(JPH::BodyID p_body_id);
	class JPH::Body *body_jolt_fetch(JPH::BodyID p_body_id);

	void body_set_use_CCD(JPH::BodyID p_body_id, bool p_use_ccd);

	void body_activate(JPH::BodyID p_body_id);
	void body_deactivate(JPH::BodyID p_body_id);

	void body_set_shape(JBody3D *p_body, JPH::ShapeRefC p_shape, bool p_activate);

	void body_set_transform(JBody3D *p_body, const Transform3D &p_transform);
	// TODO this is a temporary function, please use proper method to updated the position efficiently.
	Transform3D body_get_transform(JBody3D *p_body) const;

	void body_set_motion_type(JPH::BodyID p_body_id, JPH::EMotionType type);

	void body_set_layer(JPH::BodyID p_body_id, JPH::ObjectLayer p_layer_id);

	Dictionary cast_ray_gd(
			uint32_t p_world_id,
			const Vector3 &p_from,
			const Vector3 &p_direction,
			real_t p_distance,
			const Array &p_collide_with_layers,
			const Array &p_ignore_bodies = Array()) const;

	Array cast_shape_gd(
			uint32_t p_world_id,
			Ref<JShape3D> p_shape,
			const Transform3D &p_from,
			const Vector3 &p_move,
			const Array &p_collide_with_layers,
			const Array &p_ignore_bodies = Array()) const;

	Array collide_shape_gd(
			uint32_t p_world_id,
			Ref<JShape3D> p_shape,
			const Transform3D &p_from,
			const Array &p_collide_with_layers,
			const Array &p_ignore_bodies = Array()) const;

	TypedArray<Dictionary> high_level_ray_or_shape_cast(
			const Vector3 &p_from,
			const Vector3 &p_direction,
			real_t p_length,
			real_t p_sphere_radius,
			const Array &p_collide_with_layers,
			const Array &p_ignore_bodies) const;

	void cast_ray(
			uint32_t p_world_id,
			const JPH::RVec3 &p_from,
			const JPH::Vec3 &p_direction,
			real_t p_distance,
			const LocalVector<JPH::ObjectLayer> &p_collide_with_layers,
			JPH::CastRayCollector &r_result,
			const LocalVector<uint32_t> &p_ignore_bodies = LocalVector<uint32_t>(),
			const JPH::ShapeCastSettings &p_shape_cast_settings = JPH::ShapeCastSettings()) const;

	void cast_shape(
			uint32_t p_world_id,
			JPH::ShapeRefC p_shape,
			const JPH::RMat44 &p_transform,
			const JPH::Vec3 &p_move,
			const LocalVector<JPH::ObjectLayer> &p_collide_with_layers,
			JPH::CastShapeCollector &r_result,
			const LocalVector<uint32_t> &p_ignore_bodies = LocalVector<uint32_t>(),
			const JPH::ShapeCastSettings &p_shape_cast_settings = JPH::ShapeCastSettings()) const;

	void collide_shape(
			uint32_t p_world_id,
			JPH::ShapeRefC p_shape,
			const JPH::RMat44 &p_transform,
			const LocalVector<JPH::ObjectLayer> &p_collide_with_layers,
			JPH::CollideShapeCollector &r_result,
			const LocalVector<uint32_t> &p_ignore_bodies = LocalVector<uint32_t>(),
			const JPH::CollideShapeSettings &p_settings = JPH::CollideShapeSettings()) const;

	void collide_sphere_broad_phase(
			const JPH::Vec3 &p_sphere_center,
			float p_radius,
			std::vector<JPH::BodyID> &p_overlaps) const;

	JPH::Ref<JPH::CharacterVirtual> create_character_virtual(
			const JPH::RMat44 &p_transform,
			JPH::CharacterVirtualSettings *p_settings);

	void character_virtual_pre_step(
			float p_delta_time,
			JPH::CharacterVirtual *p_character,
			JPH::ObjectLayer p_layer_id,
			const LocalVector<uint32_t> &p_ignore_bodies = LocalVector<uint32_t>());

	void character_virtual_step(
			float p_delta_time,
			JPH::CharacterVirtual &p_character,
			JPH::CharacterVirtual::ExtendedUpdateSettings &p_update_settings,
			JPH::ObjectLayer p_layer_id,
			const LocalVector<uint32_t> &p_ignore_bodies = LocalVector<uint32_t>());

private:
	JPH::ShapeRefC get_scaled_shape(JPH::ShapeRefC p_base_shape, JPH::Vec3 p_scale);
};

#endif // JOLT_H
