
#ifndef TM_SPACE_OBJECT_BASE_H
#define TM_SPACE_OBJECT_BASE_H

#include "core/templates/local_vector.h"
#include "modules/jolt/j_body_3d.h"

#include "tm_spatial_sync_groups.h"

class NPCCharacterContactListener : public JPH::CharacterContactListener {
	virtual void OnContactAdded(
			const JPH::CharacterVirtual *inCharacter,
			const JPH::BodyID &inBodyID2,
			const JPH::SubShapeID &inSubShapeID2,
			JPH::RVec3Arg inContactPosition,
			JPH::Vec3Arg inContactNormal,
			JPH::CharacterContactSettings &ioSettings) override {
		ioSettings.mCanPushCharacter = false;
		ioSettings.mCanReceiveImpulses = false;
	}
};

struct NPC {
	NPCCharacterContactListener listener;
	/// The character settings
	JPH::CharacterVirtualSettings character_settings;
	/// The character update settings
	JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
	// The CharacterVirtual is responsible to simulate the NPC motion.
	JPH::Ref<JPH::CharacterVirtual> virtual_character;
	LocalVector<uint32_t> ignore_bodies;
	JPH::Vec3 previous_move_velocity = JPH::Vec3::sZero();
};

class TMSpaceObjectBase : public JBody3D {
	GDCLASS(TMSpaceObjectBase, JBody3D);

	LocalVector<const void *> static_body_updated_sync_groups;
	Vector<int> selected_by_peers;

	NPC npc;

public:
	TMSpaceObjectBase();
	virtual ~TMSpaceObjectBase() = default;

	static void _bind_methods();
	void _notification(int p_what);

	void mark_as_changed();
	bool static_space_object_sync_group_needs_update(const TMSyncGroupMetadata &p_group);

	void set_selected_by_peers(const Vector<int> &p_selected_by_peers);
	const Vector<int> &get_selected_by_peers() const;

	void update_3d_label(
			bool p_is_simulated,
			real_t p_size_over_distance,
			real_t p_distance,
			real_t p_update_rate);

	void notify_peer_selection_start(int p_peer);
	void notify_peer_selection_end(int p_peer);

	void server_npc_move_velocity(
			real_t p_delta,
			const Vector2 &p_direction,
			real_t p_acceleration,
			real_t p_deceleration,
			real_t p_max_speed,
			real_t p_step_height,
			real_t p_max_push_force,
			real_t p_supporting_height,
			real_t p_gravity);

	bool is_npc() const;

	virtual void __destroy_body() override;
};

#endif // TM_SPACE_OBJECT_BASE_H
