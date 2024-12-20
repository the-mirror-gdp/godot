
#ifndef TM_SCENE_SYNC_H
#define TM_SCENE_SYNC_H

#include "core/object/ref_counted.h"
#include "core/templates/local_vector.h"
#include "core/variant/variant.h"
#include "godot_expose_utils.h"
#include "modules/jolt/state_recorder_sync.h"
#include "modules/network_synchronizer/core/core.h"
#include "modules/network_synchronizer/core/net_utilities.h"
#include "modules/network_synchronizer/core/processor.h"
#include "modules/network_synchronizer/core/var_data.h"
#include "modules/network_synchronizer/godot4/gd_scene_synchronizer.h"

#include "tm_body_id_manager.h"
#include "tm_spatial_sync_groups.h"
#include "tm_state_recorder_filter.h"

class TMSceneSync final : public GdSceneSynchronizer {
	GDCLASS(TMSceneSync, GdSceneSynchronizer);
	friend class TMSpatialSyncGroups;

private:
	static TMSceneSync *the_singleton;

	NS::ObjectLocalId tm_scene_sync_object_local_id = NS::ObjectLocalId::NONE;
	TMBodyIdManager body_id_manager;

	StateRecorderSync jolt_state;
	/// This filter is being used only on the client side, it allows to collects
	/// all the info related to the sync bodies.
	/// The list of sync bodies is updated when the server state is being applied.
	TMStateRecorderFilter client_jolt_recorder_filter;

	TMSpatialSyncGroups spatial_sync_groups;

	real_t physics_simulation_radius = 40.0;
	real_t trickled_size_over_distance_max = 0.50; // 50cm size
	real_t trickled_size_over_distance_min = 0.025; // 2.5cm size
	real_t trickled_size_over_distance_stop_sync = 0.009; // 9mm size
	real_t deferred_relevancy_low_rate = 0.15; // 15%
	real_t deferred_relevancy_min_rate = 0.30; // 30%
	real_t deferred_relevancy_max_rate = 0.90; // 90%
	real_t character_group_threshold_distance = 8.0;

	uint64_t simulated_timeout = 3.0 * 1000;
	uint64_t simulated_character_timeout = 10.0 * 1000;
	uint64_t trickled_high_precision_timeout = 1 * 1000;
	real_t trickled_high_precision_rate = 1.0;

	NS::PHandler handler_jolt_process = NS::NullPHandler;

public:
	static TMSceneSync *singleton();

	static void _bind_methods();

public:
	TMSceneSync();
	~TMSceneSync();

	virtual bool snapshot_get_custom_data(const NS::SyncGroup *p_group, NS::VarData &r_custom_data) override;
	virtual void snapshot_set_custom_data(const NS::VarData &p_custom_data) override;

	/// Returns a body ID that is at the very end of the possible IDs that is possible
	/// to use: Which is really unlikely that the server is using for a SpaceObject.
	int fetch_free_unsync_body_id();
	int fetch_free_sync_body_id();

	void release_body_id(int p_id);

	void start_sync(Object *p_world_context);
	void stop_sync();

	void process_jolt_physics(double p_delta);

	void register_controller(int p_peer, Vector<Variant> p_controller_nodes);

	virtual void on_init_synchronizer(bool p_was_generating_ids) override;
	virtual void on_uninit_synchronizer() override;

	virtual void on_add_object_data(NS::ObjectData &p_node_data) override;
	virtual void on_drop_object_data(NS::ObjectData &p_node_data) override;

	virtual void update_objects_relevancy() override;
	virtual void on_sync_group_created(NS::SyncGroupId p_group_id) override;
};

#endif // TM_SCENE_SYNC_H
