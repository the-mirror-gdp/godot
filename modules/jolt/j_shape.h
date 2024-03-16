
#ifndef J_SHAPE_H
#define J_SHAPE_H

#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/templates/local_vector.h"
#include "core/variant/variant.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"

#include "Jolt/Physics/Collision/Shape/Shape.h"

#include "core/io/resource.h"

class JShape3D : public Resource {
	GDCLASS(JShape3D, Resource);

	static void _bind_methods();

protected:
	JPH::ShapeRefC shape;
	LocalVector<class JBody3D *> bodies;

public:
	JShape3D();

	void add_body(class JBody3D &p_body);
	void remove_body(class JBody3D &p_body);

	void _make_shape();
	virtual void make_shape() = 0;
	JPH::ShapeRefC get_shape();

	void notify_shape_changed();

	virtual bool is_convex() const {
		return true;
	}
};

class JBoxShape3D : public JShape3D {
	GDCLASS(JBoxShape3D, JShape3D);

	Vector3 size = Vector3(1, 1, 1);

	static void _bind_methods();

public:
	JBoxShape3D();

	void set_size(const Vector3 &p_size);
	const Vector3 &get_size() const;

	virtual void make_shape() override;
};

class JSphereShape3D : public JShape3D {
	GDCLASS(JSphereShape3D, JShape3D);

	real_t radius = 0.5;

	static void _bind_methods();

public:
	JSphereShape3D();

	void set_radius(real_t p_radius);
	real_t get_radius() const;

	virtual void make_shape() override;
};

class JCapsuleShape3D : public JShape3D {
	GDCLASS(JCapsuleShape3D, JShape3D);

	real_t radius = 0.5;
	real_t height = 2.0;

	static void _bind_methods();

public:
	JCapsuleShape3D();

	void set_radius(real_t p_radius);
	real_t get_radius() const;

	void set_height(real_t p_height);
	real_t get_height() const;

	virtual void make_shape() override;
};

class JCylinderShape3D : public JShape3D {
	GDCLASS(JCylinderShape3D, JShape3D);

	real_t radius = 0.5;
	real_t height = 2.0;

	static void _bind_methods();

public:
	JCylinderShape3D();

	void set_radius(real_t p_radius);
	real_t get_radius() const;

	void set_height(real_t p_height);
	real_t get_height() const;

	virtual void make_shape() override;
};

class JHeightFieldShape3D : public JShape3D {
	GDCLASS(JHeightFieldShape3D, JShape3D);

	Vector<float> field_data;
	Vector3 offset;
	Vector3 scale;
	uint32_t block_size = 2;

	static void _bind_methods();

protected:
	JHeightFieldShape3D();

	void set_field_data(const Vector<float> &p_map_data);
	Vector<float> get_field_data() const;

	void set_offset(const Vector3 &p_offset);
	Vector3 get_offset() const;

	void set_scale(const Vector3 &p_scale);
	Vector3 get_scale() const;

	void set_block_size(int p_bs);
	int get_block_size() const;

	virtual void make_shape() override;

	virtual bool is_convex() const override {
		return false;
	}
};

class JConvexHullShape3D : public JShape3D {
	GDCLASS(JConvexHullShape3D, JShape3D);

	static void _bind_methods();

	Vector<Vector3> points;

public:
	JConvexHullShape3D();

	void set_points(const Vector<Vector3> &p_points);
	Vector<Vector3> get_points() const;

	virtual void make_shape() override;
	virtual bool is_convex() const override;
};

class JMeshShape3D : public JShape3D {
	GDCLASS(JMeshShape3D, JShape3D);

	static void _bind_methods();

	Vector<Vector3> faces;
	bool double_sided = true;

public:
	JMeshShape3D();

	void set_faces(const Vector<Vector3> &p_points);
	Vector<Vector3> get_faces() const;

	void set_double_sided(bool ds);
	bool get_double_sided() const;

	virtual void make_shape() override;
	virtual bool is_convex() const override;
};

class JCompoundShape3D : public JShape3D {
	GDCLASS(JCompoundShape3D, JShape3D);

	static void _bind_methods();

	LocalVector<JPH::ShapeRefC> all_used_shapes;
	Vector<Ref<JShape3D>> shapes;
	Vector<Transform3D> transforms;

public:
	JCompoundShape3D();

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	void set_shapes(const Vector<Variant> &p_shapes);
	Vector<Variant> get_shapes() const;

	void set_transforms(const Vector<Variant> &p_transforms);
	Vector<Variant> get_transforms() const;

	void add_shape(Ref<JShape3D> p_shape, const Transform3D &p_transform);

	virtual void make_shape() override;
	virtual bool is_convex() const override;
};

#endif // J_SHAPE_H
