#include "j_shape.h"

#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"

#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/HeightFieldShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/Physics/Collision/Shape/ScaledShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/Shape/StaticCompoundShape.h"
#include "core/error/error_macros.h"
#include "core/math/quaternion.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/object/object.h"
#include "core/templates/local_vector.h"
#include "core/variant/variant.h"
#include "j_body_3d.h"
#include "j_custom_double_sided_shape.h"
#include "j_utils.h"
#include "jolt.h"
#include "modules/network_synchronizer/core/core.h"
#include <cfloat>
#include <cstdint>

void JShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("make_shape"), &JBoxShape3D::_make_shape);
	ClassDB::bind_method(D_METHOD("is_convex"), &JBoxShape3D::is_convex);
}

JShape3D::JShape3D() :
		Resource() {
}

void JShape3D::add_body(class JBody3D &p_body) {
	bodies.push_back(&p_body);
}

void JShape3D::remove_body(class JBody3D &p_body) {
	const int64_t idx = bodies.find(&p_body);
	if (idx >= 0) {
		bodies.remove_at_unordered(idx);
	}
}

void JShape3D::_make_shape() {
	make_shape();
}

JPH::ShapeRefC JShape3D::get_shape() {
	if (shape == nullptr) {
		make_shape();
	}
	return shape;
}

void JShape3D::notify_shape_changed() {
	shape = nullptr;
	for (JBody3D *body : bodies) {
		body->on_shape_changed();
	}
}

void JBoxShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_size", "size"), &JBoxShape3D::set_size);
	ClassDB::bind_method(D_METHOD("get_size"), &JBoxShape3D::get_size);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "size"), "set_size", "get_size");
}

JBoxShape3D::JBoxShape3D() :
		JShape3D() {
}

void JBoxShape3D::set_size(const Vector3 &p_size) {
	size = p_size;
	notify_shape_changed();
}

const Vector3 &JBoxShape3D::get_size() const {
	return size;
}

void JBoxShape3D::make_shape() {
	shape = nullptr;

	size = size.abs();

	JPH::Vec3 half_extent = convert(size * 0.5);
	real_t convex_radius = half_extent.ReduceMin() - 0.001;
	if (convex_radius < 0.0) {
		// One of the sides is 0.0, so I need to inflate it a bit, or the shape
		// creation fails.
		convex_radius = 0.0;
		half_extent += JPH::Vec3(0.001, 0.001, 0.001);
	}

	JPH::BoxShapeSettings settings(half_extent, convex_radius);
	JPH::ShapeSettings::ShapeResult shape_result = settings.Create();
	ERR_FAIL_COND_MSG(shape_result.HasError(), "Failed to create BoxShape : `" + String(shape_result.GetError().c_str()) + "` HalfExtent(X: " + rtos(half_extent.GetX()) + " Y: " + rtos(half_extent.GetY()) + " Z: " + rtos(half_extent.GetZ()) + ") ConvexRadius(" + rtos(convex_radius) + ") .");
	shape = shape_result.Get();
}

void JSphereShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &JSphereShape3D::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &JSphereShape3D::get_radius);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius"), "set_radius", "get_radius");
}

JSphereShape3D::JSphereShape3D() :
		JShape3D() {
}

void JSphereShape3D::set_radius(real_t p_radius) {
	radius = p_radius;
	notify_shape_changed();
}

real_t JSphereShape3D::get_radius() const {
	return radius;
}

void JSphereShape3D::make_shape() {
	shape = nullptr;

	JPH::SphereShapeSettings settings(radius);
	JPH::ShapeSettings::ShapeResult shape_result = settings.Create();
	ERR_FAIL_COND_MSG(shape_result.HasError(), "Failed to create SphereShape : `" + String(shape_result.GetError().c_str()) + "`.");
	shape = shape_result.Get();
}

void JCapsuleShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &JCapsuleShape3D::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &JCapsuleShape3D::get_radius);

	ClassDB::bind_method(D_METHOD("set_height", "size"), &JCapsuleShape3D::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &JCapsuleShape3D::get_height);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius"), "set_radius", "get_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height"), "set_height", "get_height");
}

JCapsuleShape3D::JCapsuleShape3D() :
		JShape3D() {
}

void JCapsuleShape3D::set_radius(real_t p_radius) {
	radius = p_radius;
	notify_shape_changed();
}

real_t JCapsuleShape3D::get_radius() const {
	return radius;
}

void JCapsuleShape3D::set_height(real_t p_height) {
	height = p_height;
	notify_shape_changed();
}

real_t JCapsuleShape3D::get_height() const {
	return height;
}

void JCapsuleShape3D::make_shape() {
	shape = nullptr;

	JPH::CapsuleShapeSettings settings(MAX(height - (radius * 2.0), real_t(0.0)) / 2.0, radius);
	JPH::ShapeSettings::ShapeResult shape_result = settings.Create();
	ERR_FAIL_COND_MSG(shape_result.HasError(), "Failed to create CapsuleShape : `" + String(shape_result.GetError().c_str()) + "`.");
	shape = shape_result.Get();
}

void JCylinderShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &JCylinderShape3D::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &JCylinderShape3D::get_radius);

	ClassDB::bind_method(D_METHOD("set_height", "size"), &JCylinderShape3D::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &JCylinderShape3D::get_height);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius"), "set_radius", "get_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height"), "set_height", "get_height");
}

JCylinderShape3D::JCylinderShape3D() :
		JShape3D() {
}

void JCylinderShape3D::set_radius(real_t p_radius) {
	radius = p_radius;
	notify_shape_changed();
}

real_t JCylinderShape3D::get_radius() const {
	return radius;
}

void JCylinderShape3D::set_height(real_t p_height) {
	height = p_height;
	notify_shape_changed();
}

real_t JCylinderShape3D::get_height() const {
	return height;
}

void JCylinderShape3D::make_shape() {
	shape = nullptr;

	JPH::CylinderShapeSettings settings(MAX(height, real_t(0.0)) / 2.0, radius);
	JPH::ShapeSettings::ShapeResult shape_result = settings.Create();
	ERR_FAIL_COND_MSG(shape_result.HasError(), "Failed to create CylinderShape : `" + String(shape_result.GetError().c_str()) + "`.");
	shape = shape_result.Get();
}

void JHeightFieldShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_field_data", "data"), &JHeightFieldShape3D::set_field_data);
	ClassDB::bind_method(D_METHOD("get_field_data"), &JHeightFieldShape3D::get_field_data);

	ClassDB::bind_method(D_METHOD("set_offset", "offset"), &JHeightFieldShape3D::set_offset);
	ClassDB::bind_method(D_METHOD("get_offset"), &JHeightFieldShape3D::get_offset);

	ClassDB::bind_method(D_METHOD("set_scale", "scale"), &JHeightFieldShape3D::set_scale);
	ClassDB::bind_method(D_METHOD("get_scale"), &JHeightFieldShape3D::get_scale);

	ClassDB::bind_method(D_METHOD("set_block_size", "block_size"), &JHeightFieldShape3D::set_block_size);
	ClassDB::bind_method(D_METHOD("get_block_size"), &JHeightFieldShape3D::get_block_size);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "field_data"), "set_field_data", "get_field_data");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "offset"), "set_offset", "get_offset");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "scale"), "set_scale", "get_scale");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "block_size"), "set_block_size", "get_block_size");
}

JHeightFieldShape3D::JHeightFieldShape3D() :
		JShape3D() {
}

void JHeightFieldShape3D::set_field_data(const Vector<float> &p_map_data) {
	field_data = p_map_data;
	notify_shape_changed();
}

Vector<float> JHeightFieldShape3D::get_field_data() const {
	return field_data;
}

void JHeightFieldShape3D::set_offset(const Vector3 &p_offset) {
	offset = p_offset;
	notify_shape_changed();
}

Vector3 JHeightFieldShape3D::get_offset() const {
	return offset;
}

void JHeightFieldShape3D::set_scale(const Vector3 &p_scale) {
	scale = p_scale;
	notify_shape_changed();
}

Vector3 JHeightFieldShape3D::get_scale() const {
	return scale;
}

void JHeightFieldShape3D::set_block_size(int p_bs) {
	block_size = uint32_t(MAX(0, p_bs));
	notify_shape_changed();
}

int JHeightFieldShape3D::get_block_size() const {
	return int(block_size);
}

void JHeightFieldShape3D::make_shape() {
	shape = nullptr;

	const uint32_t sample_count = uint32_t(Math::pow(field_data.size(), 0.5) + FLT_EPSILON);
	ERR_FAIL_COND_MSG(sample_count < 2, "The HeightField can't be created from map of size less than 2.");
	ERR_FAIL_COND_MSG((sample_count * sample_count) != uint32_t(field_data.size()), "The heightmap can't be created from a data that is not squared.");
	ERR_FAIL_COND_MSG(sample_count % block_size != 0, "The heightfield should have a sample_count `" + itos(sample_count) + "` a multiple of block_size `" + itos(block_size) + "`");

	JPH::HeightFieldShapeSettings settings(
			field_data.ptr(),
			convert(offset),
			convert(scale),
			sample_count);
	settings.mBlockSize = block_size;

	JPH::ShapeSettings::ShapeResult shape_result = settings.Create();
	ERR_FAIL_COND_MSG(shape_result.HasError(), "Failed to create heightfield shape: `" + String(shape_result.GetError().c_str()) + "`. Data count: `" + itos(field_data.size()) + "` calculated sample_count: `" + itos(sample_count) + "` Block size: `" + itos(block_size) + "`.");
	CRASH_COND(!shape_result.IsValid()); // This can't be triggered at this point, because of the above check.
	shape = shape_result.Get();
}

void JConvexHullShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_points", "points"), &JConvexHullShape3D::set_points);
	ClassDB::bind_method(D_METHOD("get_points"), &JConvexHullShape3D::get_points);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "points"), "set_points", "get_points");
}

JConvexHullShape3D::JConvexHullShape3D() :
		JShape3D() {
}

void JConvexHullShape3D::set_points(const Vector<Vector3> &p_points) {
	points = p_points;
}

Vector<Vector3> JConvexHullShape3D::get_points() const {
	return points;
}

void JConvexHullShape3D::make_shape() {
	shape = nullptr;

	LocalVector<JPH::Vec3> jpoints;
	jpoints.resize(points.size());
	for (int i = 0; i < int(points.size()); i++) {
		jpoints[i] = convert(points[i]);
	}

	JPH::ConvexHullShapeSettings settings(jpoints.ptr(), jpoints.size());

	JPH::ShapeSettings::ShapeResult shape_result = settings.Create();
	ERR_FAIL_COND_MSG(shape_result.HasError(), "Failed to create Mesh shape: `" + String(shape_result.GetError().c_str()) + "`.");
	CRASH_COND(!shape_result.IsValid()); // This can't be triggered at this point, because of the above check.
	shape = shape_result.Get();
}

bool JConvexHullShape3D::is_convex() const {
	return true;
}

void JMeshShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_faces", "faces"), &JMeshShape3D::set_faces);
	ClassDB::bind_method(D_METHOD("get_faces"), &JMeshShape3D::get_faces);

	ClassDB::bind_method(D_METHOD("set_double_sided", "ds"), &JMeshShape3D::set_double_sided);
	ClassDB::bind_method(D_METHOD("get_double_sided"), &JMeshShape3D::get_double_sided);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "faces"), "set_faces", "get_faces");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "double_sided"), "set_double_sided", "get_double_sided");
}

JMeshShape3D::JMeshShape3D() :
		JShape3D() {
}

void JMeshShape3D::set_faces(const Vector<Vector3> &p_faces) {
	faces = p_faces;
}

Vector<Vector3> JMeshShape3D::get_faces() const {
	return faces;
}

void JMeshShape3D::set_double_sided(bool ds) {
	double_sided = ds;
}

bool JMeshShape3D::get_double_sided() const {
	return double_sided;
}

void JMeshShape3D::make_shape() {
	shape = nullptr;

	JPH::TriangleList triangles;
	triangles.reserve(faces.size() / 3);

	for (int i = 0; i < int(faces.size() / 3); i++) {
		triangles.push_back(
				JPH::Triangle(
						convert(faces[(i * 3) + 0]),
						convert(faces[(i * 3) + 1]),
						convert(faces[(i * 3) + 2])));
	}

	JPH::MeshShapeSettings mesh_shape_settings(triangles);
	JPH::ShapeSettings::ShapeResult mesh_shape_result = mesh_shape_settings.Create();

	ERR_FAIL_COND_MSG(mesh_shape_result.HasError(), "Failed to create Mesh shape: `" + String(mesh_shape_result.GetError().c_str()) + "`.");
	CRASH_COND(!mesh_shape_result.IsValid()); // This can't be triggered at this point, because of the above check.

	if (double_sided) {
		JoltCustomDoubleSidedShapeSettings double_sided_shape_settings;
		double_sided_shape_settings.mInnerShapePtr = mesh_shape_result.Get();
		JPH::ShapeSettings::ShapeResult double_sided_shape_result = double_sided_shape_settings.Create();

		ERR_FAIL_COND_MSG(double_sided_shape_result.HasError(), "Failed to create Mesh shape: `" + String(double_sided_shape_result.GetError().c_str()) + "`.");
		CRASH_COND(!double_sided_shape_result.IsValid()); // This can't be triggered at this point, because of the above check.

		shape = double_sided_shape_result.Get();
	} else {
		shape = mesh_shape_result.Get();
	}
}

bool JMeshShape3D::is_convex() const {
	return false;
}

void JCompoundShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_shapes", "shapes"), &JCompoundShape3D::set_shapes);
	ClassDB::bind_method(D_METHOD("get_shapes"), &JCompoundShape3D::get_shapes);

	ClassDB::bind_method(D_METHOD("set_transforms", "transforms"), &JCompoundShape3D::set_transforms);
	ClassDB::bind_method(D_METHOD("get_transforms"), &JCompoundShape3D::get_transforms);

	ClassDB::bind_method(D_METHOD("add_shape", "shape", "transform"), &JCompoundShape3D::add_shape);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "shapes"), "set_shapes", "get_shapes");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "transforms"), "set_transforms", "get_transforms");
}

JCompoundShape3D::JCompoundShape3D() :
		JShape3D() {
}

bool JCompoundShape3D::_set(const StringName &p_name, const Variant &p_value) {
	String the_name = p_name;
	if (the_name == "count") {
		const int count = p_value;
		shapes.resize(count);
		transforms.resize(count);
		return true;
	}

	Vector<String> path = the_name.split("/");
	ERR_FAIL_COND_V(path.size() != 2, false);

	const int index = path[0].to_int();
	const String prop_name = path[1];

	ERR_FAIL_COND_V(transforms.size() <= index, false);

	if (prop_name == "translation") {
		transforms.write[index].origin = p_value;
		return true;

	} else if (prop_name == "rotation") {
		transforms.write[index].basis.set_euler((Vector3(p_value) / 180.0) * Math_PI);
		return true;

	} else if (prop_name == "scale") {
		transforms.write[index].basis = Basis(transforms[index].basis.get_rotation_quaternion());
		transforms.write[index].basis.scale(p_value);
		return true;

	} else if (prop_name == "shape") {
		shapes.write[index] = p_value;
		return true;
	}

	return false;
}

bool JCompoundShape3D::_get(const StringName &p_name, Variant &r_ret) const {
	String the_name = p_name;
	if (the_name == "count") {
		r_ret = shapes.size();
		return true;
	}

	Vector<String> path = the_name.split("/");
	ERR_FAIL_COND_V(path.size() != 2, false);

	const int index = path[0].to_int();
	const String prop_name = path[1];

	ERR_FAIL_COND_V(transforms.size() <= index, false);

	if (prop_name == "translation") {
		r_ret = transforms[index].origin;
		return true;

	} else if (prop_name == "rotation") {
		r_ret = (transforms[index].basis.get_euler() / Math_PI) * 180.0;
		return true;

	} else if (prop_name == "scale") {
		r_ret = transforms[index].basis.get_scale();
		return true;

	} else if (prop_name == "shape") {
		r_ret = shapes[index];
		return true;
	}

	return false;
}

void JCompoundShape3D::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::INT, PNAME("count")));
	for (int i = 0; i < shapes.size(); i++) {
		p_list->push_back(PropertyInfo(Variant::VECTOR3, PNAME(itos(i) + "/translation")));
		p_list->push_back(PropertyInfo(Variant::VECTOR3, PNAME(itos(i) + "/rotation")));
		p_list->push_back(PropertyInfo(Variant::VECTOR3, PNAME(itos(i) + "/scale")));
		p_list->push_back(PropertyInfo(Variant::OBJECT, PNAME(itos(i) + "/shape"), PROPERTY_HINT_RESOURCE_TYPE, "JShape3D"));
	}
}

void JCompoundShape3D::set_shapes(const Vector<Variant> &p_shapes) {
	shapes.clear();
	for (const Variant &v : p_shapes) {
		Ref<JShape3D> o = v;
		if (o.is_valid()) {
			shapes.push_back(o);
		} else {
			shapes.push_back(Ref<JShape3D>());
		}
	}
}

Vector<Variant> JCompoundShape3D::get_shapes() const {
	Vector<Variant> r_shapes;
	for (auto &o : shapes) {
		r_shapes.push_back(o);
	}
	return r_shapes;
}

void JCompoundShape3D::set_transforms(const Vector<Variant> &p_transforms) {
	transforms.clear();
	for (const Variant &v : p_transforms) {
		transforms.push_back(v);
	}
}

Vector<Variant> JCompoundShape3D::get_transforms() const {
	Vector<Variant> r_transforms;
	for (auto &o : transforms) {
		r_transforms.push_back(o);
	}
	return r_transforms;
}

void JCompoundShape3D::add_shape(Ref<JShape3D> p_shape, const Transform3D &p_transform) {
	shapes.push_back(p_shape);
	transforms.push_back(p_transform);
}

void JCompoundShape3D::make_shape() {
	shape = nullptr;
	all_used_shapes.clear();

	JPH::StaticCompoundShapeSettings settings;

	for (int i = 0; i < int(shapes.size()); i++) {
		if (shapes[i].is_null()) {
			continue;
		}

		const JPH::Shape *inner_shape = shapes.write[i]->get_shape();
		if (inner_shape == nullptr) {
			continue;
		}
		all_used_shapes.push_back(inner_shape);

		JPH::Vec3 translation = JPH::Vec3::sZero();
		JPH::Quat rotation = JPH::Quat::sIdentity();

		if (transforms.size() > i) {
			translation = convert(transforms[i].origin);
			rotation = convert(transforms[i].basis.get_rotation_quaternion());

			const Vector3 scale = transforms[i].basis.get_scale();
			if (!scale.is_equal_approx(Vector3(1, 1, 1))) {
				// We have to scale this shape.
				JPH::ScaledShapeSettings scaled_shape_settings(inner_shape, convert(scale));
				JPH::ShapeSettings::ShapeResult scaled_shape_result = scaled_shape_settings.Create();
				ERR_FAIL_COND_MSG(scaled_shape_result.HasError(), "Failed to create ScaledShape for StaticCompoundShape : `" + String(scaled_shape_result.GetError().c_str()) + "`.");
				CRASH_COND(!scaled_shape_result.IsValid()); // This can't be triggered at this point, because of the above check.
				JPH::ShapeRefC is_ref = inner_shape = scaled_shape_result.Get();
				CRASH_COND(is_ref == nullptr);
				inner_shape = is_ref;
				all_used_shapes.push_back(is_ref);
			}
		}

		settings.AddShape(
				translation,
				rotation,
				inner_shape);
	}

	JPH::ShapeSettings::ShapeResult shape_result = settings.Create();
	ERR_FAIL_COND_MSG(shape_result.HasError(), "Failed to create StaticCompoundShape : `" + String(shape_result.GetError().c_str()) + "`.");
	CRASH_COND(!shape_result.IsValid()); // This can't be triggered at this point, because of the above check.
	shape = shape_result.Get();
}

bool JCompoundShape3D::is_convex() const {
	for (auto &s : shapes) {
		if (s.is_valid()) {
			if (!s->is_convex()) {
				return false;
			}
		}
	}

	return true;
}
