#pragma once

#include "core/math/vector3.h"
#include "core/object/ref_counted.h"
#include "core/templates/local_vector.h"
#include "modules/network_synchronizer/core/net_utilities.h"

#include "tm_state_recorder_filter.h"

struct TMListenerMetadata;

struct TMSyncGroupMetadata : public RefCounted {
	GDCLASS(TMSyncGroupMetadata, RefCounted);

public:
	uint32_t sync_group_id;
	TMStateRecorderFilter recorder_filter;

	LocalVector<Ref<TMListenerMetadata>> listeners;
	std::vector<uint64_t> into_simulated_radius_timestamp;

	Vector3 origin;
	real_t realtime_relevancy_radius = 1.0;
};

struct TMListenerMetadata : public RefCounted {
	int peer = 0;
	Ref<TMSyncGroupMetadata> sync_group;

	class TMCharacter3D *character = nullptr;
	NS::ObjectData *character_nd = nullptr;

	Vector<NS::ObjectData *> other_controlled_nodes;
};

class TMSpatialSyncGroups {
	class TMSceneSync &scene_sync;

public:
	LocalVector<Ref<TMListenerMetadata>> world_listeners;

	/// All the Sync groups.
	LocalVector<Ref<TMSyncGroupMetadata>> sync_groups_metadata;

public:
	TMSpatialSyncGroups(class TMSceneSync &p_scene_sync) :
			scene_sync(p_scene_sync) {}

	void register_controller(
			int p_peer,
			class TMCharacter3D &p_character,
			NS::ObjectData &p_character_nd,
			Vector<NS::ObjectData *> p_other_controlled_nodes);

	void add_new_node_data(NS::ObjectData *p_nd);
	void remove_node_data(NS::ObjectData *p_node_data);
	void remove_controller(Ref<TMListenerMetadata> p_controller_meta);

	void on_new_sync_group_created(uint32_t id);

	/// This function updates the SyncGroups so that:
	/// - Near characters and space objects are put as physics sync nodes.
	/// - Far characters and space objects are put as deferred sync nodes.
	/// - Really far characters and space objects are completely ignored.
	void update();

	const TMStateRecorderFilter *get_sync_group_updated_recorder_filter(const NS::SyncGroup &p_sg);

	void clear();

private:
	Ref<TMSyncGroupMetadata> get_empty_sync_group();
	Ref<TMSyncGroupMetadata> create_empty_sync_group();
	void assign_controller_to_sync_group(Ref<TMSyncGroupMetadata> p_sg, Ref<TMListenerMetadata> p_controller);

private: // --------------------------------------------------- Update functions
	void update_sync_groups_listeners();
	void update_sync_groups_location_and_tracked_objects();
	void update_sync_group_fetch_group_origin(int p_group_index);
	void update_sync_group_tracked_objects(int p_group_index);
	bool is_any_player_editing_the_space_object(TMSyncGroupMetadata &sg, class JBody3D &p_body) const;
	void compute_update_rate(
			class JBody3D &p_node,
			TMSyncGroupMetadata &p_group,
			uint64_t p_last_simulation_time,
			bool p_is_character,
			bool &r_skip_trickled,
			real_t &r_update_rate,
			real_t &r_size_over_distance,
			real_t &r_distance) const;
};
