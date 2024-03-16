
#ifndef TM_STATE_RECORDER_FILTER_H
#define TM_STATE_RECORDER_FILTER_H

#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/StateRecorder.h"

namespace JPH {
class Constraint;
}

class TMStateRecorderFilter : public JPH::StateRecorderFilter {
	std::vector<bool> bodies_to_sync_buffer;

public:
	void Clear();
	void MarkAsSync(const JPH::BodyID &p_id);

	bool ShouldSaveBody(const JPH::BodyID &inBodyID) const;
	virtual bool ShouldSaveBody(const JPH::Body &inBody) const override;
	virtual bool ShouldSaveConstraint(const JPH::Constraint &inConstraint) const override;
	virtual bool ShouldSaveContact(const JPH::BodyID &inBody1, const JPH::BodyID &inBody2) const override;
};

#endif // TM_STATE_RECORDER_FILTER_H
