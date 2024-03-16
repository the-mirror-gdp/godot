
#ifndef TM_BODY_ID_MANAGER_H
#define TM_BODY_ID_MANAGER_H

#include "core/templates/local_vector.h"
#include "modules/jolt/j_body_id_manager.h"

class TMBodyIdManager : public JBodyIdManager {
	const int max_unsync_bodies;

	int generated_unsync_body_id_count = 0;
	LocalVector<int> free_unsync_body_ids;

	int generated_sync_body_id_count = 0;
	LocalVector<int> free_sync_body_ids;

	LocalVector<int> untracked_body_ids;

public:
	TMBodyIdManager(int p_max_unsync_bodies);
	virtual ~TMBodyIdManager() = default;

	virtual JPH::BodyID fetch_free_body_id() override;
	virtual void on_body_created(const JPH::BodyID &p_body_id) override;
	virtual void on_body_destroyed(const JPH::BodyID &p_body_id) override;
	virtual void on_body_3d_destroyed(class JBody3D &p_body) override;

	void reset();
	bool is_unsync_body_id(const JPH::BodyID &p_body_id) const;
	JPH::BodyID fetch_free_unsync_body_id();
	JPH::BodyID fetch_free_sync_body_id();

	void release_body_id(const JPH::BodyID &p_id);
};

#endif // TM_BODY_ID_MANAGER_H
