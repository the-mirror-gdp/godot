#include "tm_body_id_manager.h"

#include "Jolt/Physics/Body/BodyID.h"
#include "core/error/error_macros.h"
#include "modules/jolt/j_body_3d.h"

TMBodyIdManager::TMBodyIdManager(int p_max_unsync_bodies) :
		max_unsync_bodies(p_max_unsync_bodies) {
}

JPH::BodyID TMBodyIdManager::fetch_free_body_id() {
	JPH::BodyID id = fetch_free_unsync_body_id();
	untracked_body_ids.push_back(id.GetIndex());
	return id;
}

void TMBodyIdManager::on_body_created(const JPH::BodyID &p_body_id) {
	// Nothing to do.
}

void TMBodyIdManager::on_body_destroyed(const JPH::BodyID &p_body_id) {
	const int index = untracked_body_ids.find(p_body_id.GetIndex());
	if (index >= 0) {
		untracked_body_ids.remove_at_unordered(index);
		release_body_id(p_body_id);
	} else {
		// Nothing to do.
	}
}

void TMBodyIdManager::on_body_3d_destroyed(class JBody3D &p_body) {
	if (!p_body.get_jph_desired_body_id().IsInvalid()) {
		release_body_id(p_body.get_jph_desired_body_id());
	}
}

void TMBodyIdManager::reset() {
	generated_unsync_body_id_count = 0;
	free_unsync_body_ids.clear();

	generated_sync_body_id_count = 0;
	free_sync_body_ids.clear();
}

bool TMBodyIdManager::is_unsync_body_id(const JPH::BodyID &p_body_id) const {
	return p_body_id.GetIndex() < uint32_t(max_unsync_bodies);
}

JPH::BodyID TMBodyIdManager::fetch_free_unsync_body_id() {
	if (free_unsync_body_ids.size() > 0) {
		const int id = free_unsync_body_ids[free_unsync_body_ids.size() - 1];
		free_unsync_body_ids.resize(free_unsync_body_ids.size() - 1);
		return JPH::BodyID(id);
	} else {
		const int id = generated_unsync_body_id_count;
		CRASH_COND(id >= max_unsync_bodies);
		generated_unsync_body_id_count++;
		return JPH::BodyID(id);
	}
}

JPH::BodyID TMBodyIdManager::fetch_free_sync_body_id() {
	if (free_sync_body_ids.size() > 0) {
		const int id = free_sync_body_ids[free_sync_body_ids.size() - 1];
		free_sync_body_ids.resize(free_sync_body_ids.size() - 1);
		return JPH::BodyID(id);
	} else {
		const int id = generated_sync_body_id_count;
		generated_sync_body_id_count++;
		return JPH::BodyID(id + max_unsync_bodies);
	}
}

void TMBodyIdManager::release_body_id(const JPH::BodyID &p_id) {
	if (is_unsync_body_id(p_id)) {
		free_unsync_body_ids.push_back(p_id.GetIndex());
	} else {
		free_sync_body_ids.push_back(p_id.GetIndex());
	}
}
