
#ifndef J_UTILS_H
#define J_UTILS_H

#include "Jolt/Core/Core.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"

#include "Jolt/Core/Color.h"
#include "Jolt/Core/Core.h"
#include "Jolt/Math/Math.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Real.h"
#include "Jolt/Math/Trigonometry.h"
#include "Jolt/Math/Vec3.h"
#include "core/math/color.h"
#include "core/math/quaternion.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"

void unit_test_converters();

Vector3 convert(const JPH::Vec3 &v);
JPH::Vec3 convert(const Vector3 &v);
JPH::RVec3 convert_r(const Vector3 &v);
Vector3 convert_r(const JPH::RVec3 &v);

JPH::Color convert(const Color &v);
Color convert(const JPH::Color &v);

JPH::Quat convert(const Quaternion &q);
Quaternion convert(const JPH::Quat &q);

JPH::Mat44 convert(const Transform3D &t);
JPH::RMat44 convert_r(const Transform3D &t);

Transform3D convert_r(const JPH::RMat44 &t);

JPH::Mat44 convert(const Basis &b);

#endif // J_UTILS_H
