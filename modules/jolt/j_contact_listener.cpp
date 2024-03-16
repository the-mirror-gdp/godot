
#include "j_contact_listener.h"

#include "j_body_3d.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Body/Body.h"

void JContactListener::OnContactAdded(
		const JPH::Body &inBody1,
		const JPH::Body &inBody2,
		const JPH::ContactManifold &inManifold,
		JPH::ContactSettings &ioSettings) {
	for (const auto &listening_func : contact_added_listeners) {
		listening_func(inBody1, inBody2, inManifold, ioSettings);
	}
	ContactDetected(inBody1, inBody2, inManifold, ioSettings);
}

void JContactListener::OnContactPersisted(
		const JPH::Body &inBody1,
		const JPH::Body &inBody2,
		const JPH::ContactManifold &inManifold,
		JPH::ContactSettings &ioSettings) {
	for (const auto &listening_func : contact_persisted_listeners) {
		listening_func(inBody1, inBody2, inManifold, ioSettings);
	}
	ContactDetected(inBody1, inBody2, inManifold, ioSettings);
}

void JContactListener::ContactDetected(
		const JPH::Body &bodyA,
		const JPH::Body &bodyB,
		const JPH::ContactManifold &inManifold,
		JPH::ContactSettings &ioSettings) {
	if (!bodyA.IsSensor() && !bodyB.IsSensor()) {
		return;
	}

	if (inManifold.mPenetrationDepth <= 0.001) {
		return;
	}

	ProcessContactDetected(bodyA, bodyB);
	ProcessContactDetected(bodyB, bodyA);
}

void JContactListener::ProcessContactDetected(
		const JPH::Body &maybeSensor,
		const JPH::Body &body) {
	if (!maybeSensor.IsSensor()) {
		// This is not a sensor, nothing to do.
		return;
	}

	JBody3D *sensor_body_3d = sExtractJBody3D(maybeSensor);
	JBody3D *body_3d = sExtractJBody3D(body);
#ifdef DEBUG_ENABLED
	// These can't be null because bodies are always created by the `Jolt` class
	// that creates the body only for a valid `JBody3D`.
	CRASH_COND(!sensor_body_3d);
	CRASH_COND(!body_3d);
#endif

	sensor_body_3d->sensor_mark_body_as_overlap(body_3d, true);
}

JBody3D *JContactListener::sExtractJBody3D(const JPH::Body &body) {
	return reinterpret_cast<JBody3D *>(body.GetUserData());
}
