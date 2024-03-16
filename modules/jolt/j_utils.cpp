#include "j_utils.h"

#include "Jolt/Math/Vec3.h"
#include "core/error/error_macros.h"
#include "core/math/basis.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Math/Mat44.h"

void unit_test_converters() {
	// Test transform conversion.
	{
		const Basis gb = Basis::from_euler(Vector3(1, 1, 1));
		const Transform3D gt(gb, Vector3(200, 0, 0));
		const JPH::RMat44 jt = convert_r(gt);

		CRASH_COND(jt.GetTranslation().GetX() != 200.0);
		CRASH_COND(jt.GetTranslation().GetY() != 0.0);
		CRASH_COND(jt.GetTranslation().GetZ() != 0.0);

		const JPH::Mat44 jm = jt.GetRotation();
		CRASH_COND(jm.GetColumn3(0)[0] != (float)gb.get_column(0)[0]);
		CRASH_COND(jm.GetColumn3(0)[1] != (float)gb.get_column(0)[1]);
		CRASH_COND(jm.GetColumn3(0)[2] != (float)gb.get_column(0)[2]);
		CRASH_COND(jm.GetColumn3(1)[0] != (float)gb.get_column(1)[0]);
		CRASH_COND(jm.GetColumn3(1)[1] != (float)gb.get_column(1)[1]);
		CRASH_COND(jm.GetColumn3(1)[2] != (float)gb.get_column(1)[2]);
		CRASH_COND(jm.GetColumn3(2)[0] != (float)gb.get_column(2)[0]);
		CRASH_COND(jm.GetColumn3(2)[1] != (float)gb.get_column(2)[1]);
		CRASH_COND(jm.GetColumn3(2)[2] != (float)gb.get_column(2)[2]);

		const Transform3D gt2 = convert_r(jt);
		CRASH_COND(!gt.is_equal_approx(gt2));
	}
}

Vector3 convert(const JPH::Vec3 &v) {
	return Vector3(v[0], v[1], v[2]);
}

JPH::Vec3 convert(const Vector3 &v) {
	return JPH::Vec3(v.x, v.y, v.z);
}

JPH::RVec3 convert_r(const Vector3 &v) {
	return JPH::RVec3(v.x, v.y, v.z);
}

Vector3 convert_r(const JPH::RVec3 &v) {
	return Vector3(v[0], v[1], v[2]);
}

JPH::Color convert(const Color &v) {
	return JPH::Color(v.r, v.g, v.b, v.a);
}

Color convert(const JPH::Color &v) {
	return Color(v.r, v.g, v.b, v.a);
}

JPH::Quat convert(const Quaternion &q) {
	return JPH::Quat(q.x, q.y, q.z, q.w);
}

Quaternion convert(const JPH::Quat &q) {
	return Quaternion(q.GetX(), q.GetY(), q.GetZ(), q.GetW());
}

JPH::Mat44 convert(const Transform3D &t) {
	// TODO we need to convert the scale too.
	const JPH::Mat44 res(
			JPH::Vec4(convert(t.get_basis().get_column(0)), 0.0),
			JPH::Vec4(convert(t.get_basis().get_column(1)), 0.0),
			JPH::Vec4(convert(t.get_basis().get_column(2)), 0.0),
			convert(t.get_origin()));
	return res;
}

JPH::RMat44 convert_r(const Transform3D &t) {
	// TODO we need to convert the scale too.
	const JPH::RMat44 res(
			JPH::Vec4(convert(t.get_basis().get_column(0)), 0.0),
			JPH::Vec4(convert(t.get_basis().get_column(1)), 0.0),
			JPH::Vec4(convert(t.get_basis().get_column(2)), 0.0),
			convert_r(t.get_origin()));
	return res;
}

Transform3D convert_r(const JPH::RMat44 &t) {
	Transform3D res;
	res.set_basis(Basis(convert(t.GetColumn3(0)), convert(t.GetColumn3(1)), convert(t.GetColumn3(2))));
	res.set_origin(convert_r(t.GetTranslation()));
	// TODO we need to convert the scale.
	return res;
}

JPH::Mat44 convert(const Basis &b) {
	// TODO we need to convert the scale too.
	const JPH::Mat44 res(
			JPH::Vec4(convert(b.get_column(0)), 0.0),
			JPH::Vec4(convert(b.get_column(1)), 0.0),
			JPH::Vec4(convert(b.get_column(2)), 0.0),
			JPH::Vec3::sZero());
	return res;
}
