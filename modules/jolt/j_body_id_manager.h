
#ifndef J_BODY_ID_MANAGER_H
#define J_BODY_ID_MANAGER_H

#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Body/BodyID.h"

/// Override this class to control the body ID creation.
class JBodyIdManager {
public:
	virtual ~JBodyIdManager() = default;

	virtual JPH::BodyID fetch_free_body_id() = 0;

	virtual void on_body_created(const JPH::BodyID &p_body_id) = 0;
	virtual void on_body_destroyed(const JPH::BodyID &p_body_id) = 0;
	virtual void on_body_3d_destroyed(class JBody3D &p_body) = 0;
};

#endif // J_BODY_ID_MANAGER_H
