
#ifndef TM_CHARACTER_3D_H
#define TM_CHARACTER_3D_H

#include "core/templates/local_vector.h"
#include "modules/jolt/j_body_3d.h"
#include "modules/jolt/state_recorder_sync.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Character/CharacterVirtual.h"
#include "modules/network_synchronizer/core/core.h"
#include "modules/network_synchronizer/core/net_utilities.h"
#include "modules/network_synchronizer/data_buffer.h"

#include "godot_expose_utils.h"

struct PendingInteraction {
	NS::ObjectNetId node_net_id;
	uint8_t interaction_id;
	Variant argument;
};

class TMCharacter3D : public JBody3D {
	GDCLASS(TMCharacter3D, JBody3D);

	SETGET(real_t, character_height, 1.75);
	SETGET(real_t, character_radius, 0.3);
	SETGET(real_t, character_supporting_offset, 0.1);
	SETGET(real_t, max_walking_slope_deg, 50.0);

	SETGET(real_t, walk_speed, 5.0);
	SETGET(real_t, run_speed, 9.0);
	SETGET(real_t, jump_height, 2.0);
	SETGET(real_t, ground_acceleration, 40.0);
	SETGET(real_t, ground_deceleration, 25.0);
	SETGET(real_t, air_acceleration, 15.0);
	SETGET(real_t, air_deceleration, 5.0);
	SETGET(real_t, step_height, 0.2);
	SETGET(real_t, max_push_strength_newton, 50000.0);
	SETGET(real_t, predictive_contact_margin, 0.3);
	SETGET(real_t, shape_margin, 0.02);

	SETGET(bool, frozen, false);
	SETGET(real_t, movement_scale, 1.0);

	static void _bind_methods();

	// TODO expose this.
	LocalVector<uint32_t> ignore_bodies;

	/// The character settings
	JPH::CharacterVirtualSettings character_settings;
	/// The character update settings
	JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
	// The CharacterVirtual is responsible to simulate the character motion.
	JPH::Ref<JPH::CharacterVirtual> virtual_character;

	bool cached_is_running = false;

	LocalVector<PendingInteraction> pending_interactions;
	JPH::Vec3 accumulated_impulses;

public:
	NS::ObjectData *object_data = nullptr;
	LocalVector<NS::ObjectData *> objects_data;

private:
	// Holds the last state set for this character.
	// Used in case the get state can't fetch the data from the VirtualCharacter.
	Vector<uint8_t> last_char_set_state;

	JPH::Vec3 previous_move_velocity = JPH::Vec3::sZero();

public:
	TMCharacter3D();
	~TMCharacter3D();

	void jolt_char_set_state(const Vector<uint8_t> &p_data);
	Vector<uint8_t> jolt_char_get_state();

	real_t get_cached_max_walk_speed() const;

	void process_character(real_t p_delta, const Vector2 &p_input_move, bool p_input_run, bool p_input_jump, real_t p_gravity);

	void __step(
			real_t p_delta_time,
			const JPH::Vec3 &p_velocity);

	bool is_on_ceiling() const;
	bool is_on_floor() const;

	void update_virtual_character();
	virtual void __update_body() override;
	virtual void __create_body() override;

	virtual void __update_godot_transform() override;
	virtual void __update_jolt_transform() override;

	void update_3d_label(uint32_t p_listening_group_index);

	void queue_interaction(Object *p_space_object, const StringName &p_name, const Variant &p_argument);
	void write_pending_interactions_to_buffer(Object *r_buffer);
	void execute_interactions(Object *p_buffer);
	bool are_interactions_different(Object *p_buffer_A, Object *p_buffer_B);
	int count_interactions_buffer_size(Object *p_buffer) const;

	virtual void add_impulse(const JPH::Vec3 &p_impulse) override;
	virtual void add_impulse_at_position(const JPH::Vec3 &p_impulse, const JPH::RVec3 &p_position) override;

public: // ----------------------------------------------------------------- API
	Vector3 get_ground_velocity() const;
	Vector3 get_character_relative_velocity() const;
};

#endif // TM_CHARACTER_3D_H
