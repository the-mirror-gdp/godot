#include "tm_state_recorder_filter.h"

#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Body/Body.h"
#include "modules/network_synchronizer/core/net_utilities.h"

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_GODOT_TRACY_ENABLED
#include "modules/godot_tracy/profiler.h"
#else
// Dummy defines to allow compiling without tracy.
#define ZoneScoped
#endif // MODULE_GODOT_TRACY_ENABLED

void TMStateRecorderFilter::Clear() {
	ZoneScoped;
	bodies_to_sync_buffer.clear();
}

void TMStateRecorderFilter::MarkAsSync(const JPH::BodyID &inBodyID) {
	ZoneScoped;
	NS::VecFunc::insert_at_position_expand(bodies_to_sync_buffer, inBodyID.GetIndex(), true, false);
}

bool TMStateRecorderFilter::ShouldSaveBody(const JPH::BodyID &inBodyID) const {
	ZoneScoped;
	return NS::VecFunc::at(bodies_to_sync_buffer, inBodyID.GetIndex(), false);
}

bool TMStateRecorderFilter::ShouldSaveBody(const JPH::Body &inBody) const {
	ZoneScoped;
	return ShouldSaveBody(inBody.GetID());
}

bool TMStateRecorderFilter::ShouldSaveConstraint(const JPH::Constraint &inConstraint) const {
	ZoneScoped;
	// Constraint sync is not yet implemented.
	return false;
}

bool TMStateRecorderFilter::ShouldSaveContact(const JPH::BodyID &inBody1, const JPH::BodyID &inBody2) const {
	ZoneScoped;
	return ShouldSaveBody(inBody1) || ShouldSaveBody(inBody2);
}
