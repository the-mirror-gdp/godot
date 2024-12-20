#include "tm_state_recorder_filter.h"

#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Body/Body.h"
#include "modules/network_synchronizer/core/net_utilities.h"

void TMStateRecorderFilter::Clear() {
	bodies_to_sync_buffer.clear();
}

void TMStateRecorderFilter::MarkAsSync(const JPH::BodyID &inBodyID) {
	NS::VecFunc::insert_at_position_expand(bodies_to_sync_buffer, inBodyID.GetIndex(), true, false);
}

bool TMStateRecorderFilter::ShouldSaveBody(const JPH::BodyID &inBodyID) const {
	return NS::VecFunc::at(bodies_to_sync_buffer, inBodyID.GetIndex(), false);
}

bool TMStateRecorderFilter::ShouldSaveBody(const JPH::Body &inBody) const {
	return ShouldSaveBody(inBody.GetID());
}

bool TMStateRecorderFilter::ShouldSaveConstraint(const JPH::Constraint &inConstraint) const {
	// Constraint sync is not yet implemented.
	return false;
}

bool TMStateRecorderFilter::ShouldSaveContact(const JPH::BodyID &inBody1, const JPH::BodyID &inBody2) const {
	return ShouldSaveBody(inBody1) || ShouldSaveBody(inBody2);
}
