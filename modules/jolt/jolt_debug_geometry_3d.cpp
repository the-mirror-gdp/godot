// Adapted version of: https://github.com/godot-jolt/godot-jolt/
// Kudos to Mihe

#include "jolt_debug_geometry_3d.h"

#include "Jolt/Jolt.h"

#include "Jolt/Physics/Collision/Shape/HeightFieldShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "core/object/object.h"
#include "core/variant/variant.h"
#include "jolt.h"
#include "scene/main/node.h"
#include "scene/main/viewport.h"

#ifdef JPH_DEBUG_RENDERER
#include "jolt_debug_renderer_3d.h"
#endif // JPH_DEBUG_RENDERER

void JoltDebugGeometry3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_draw_bodies"), &JoltDebugGeometry3D::get_draw_bodies);
	ClassDB::bind_method(D_METHOD("set_draw_bodies", "enabled"), &JoltDebugGeometry3D::set_draw_bodies);

	ClassDB::bind_method(D_METHOD("get_draw_shapes"), &JoltDebugGeometry3D::get_draw_shapes);
	ClassDB::bind_method(D_METHOD("set_draw_shapes", "enabled"), &JoltDebugGeometry3D::set_draw_shapes);

	ClassDB::bind_method(D_METHOD("get_draw_constraints"), &JoltDebugGeometry3D::get_draw_constraints);
	ClassDB::bind_method(D_METHOD("set_draw_constraints", "enabled"), &JoltDebugGeometry3D::set_draw_constraints);

	ClassDB::bind_method(D_METHOD("get_draw_bounding_boxes"), &JoltDebugGeometry3D::get_draw_bounding_boxes);
	ClassDB::bind_method(D_METHOD("set_draw_bounding_boxes", "enabled"), &JoltDebugGeometry3D::set_draw_bounding_boxes);

	ClassDB::bind_method(D_METHOD("get_draw_centers_of_mass"), &JoltDebugGeometry3D::get_draw_centers_of_mass);
	ClassDB::bind_method(D_METHOD("set_draw_centers_of_mass", "enabled"), &JoltDebugGeometry3D::set_draw_centers_of_mass);

	ClassDB::bind_method(D_METHOD("get_draw_transforms"), &JoltDebugGeometry3D::get_draw_transforms);
	ClassDB::bind_method(D_METHOD("set_draw_transforms", "enabled"), &JoltDebugGeometry3D::set_draw_transforms);

	ClassDB::bind_method(D_METHOD("get_draw_velocities"), &JoltDebugGeometry3D::get_draw_velocities);
	ClassDB::bind_method(D_METHOD("set_draw_velocities", "enabled"), &JoltDebugGeometry3D::set_draw_velocities);

	ClassDB::bind_method(D_METHOD("get_draw_triangle_outlines"), &JoltDebugGeometry3D::get_draw_triangle_outlines);
	ClassDB::bind_method(D_METHOD("set_draw_triangle_outlines", "enabled"), &JoltDebugGeometry3D::set_draw_triangle_outlines);

	ClassDB::bind_method(D_METHOD("get_draw_constraint_reference_frames"), &JoltDebugGeometry3D::get_draw_constraint_reference_frames);
	ClassDB::bind_method(D_METHOD("set_draw_constraint_reference_frames", "enabled"), &JoltDebugGeometry3D::set_draw_constraint_reference_frames);

	ClassDB::bind_method(D_METHOD("get_draw_constraint_limits"), &JoltDebugGeometry3D::get_draw_constraint_limits);
	ClassDB::bind_method(D_METHOD("set_draw_constraint_limits", "enabled"), &JoltDebugGeometry3D::set_draw_constraint_limits);

	ClassDB::bind_method(D_METHOD("get_draw_as_wireframe"), &JoltDebugGeometry3D::get_draw_as_wireframe);
	ClassDB::bind_method(D_METHOD("set_draw_as_wireframe", "enabled"), &JoltDebugGeometry3D::set_draw_as_wireframe);

	ClassDB::bind_method(D_METHOD("get_draw_with_color_scheme"), &JoltDebugGeometry3D::get_draw_with_color_scheme);
	ClassDB::bind_method(D_METHOD("set_draw_with_color_scheme", "color_scheme"), &JoltDebugGeometry3D::set_draw_with_color_scheme);

	ClassDB::bind_method(D_METHOD("get_material_depth_test"), &JoltDebugGeometry3D::get_material_depth_test);
	ClassDB::bind_method(D_METHOD("set_material_depth_test", "enabled"), &JoltDebugGeometry3D::set_material_depth_test);

	ADD_GROUP("Draw", "draw_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_bodies"), "set_draw_bodies", "get_draw_bodies");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_shapes"), "set_draw_shapes", "get_draw_shapes");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_constraints"), "set_draw_constraints", "get_draw_constraints");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_bounding_boxes"), "set_draw_bounding_boxes", "get_draw_bounding_boxes");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_centers_of_mass"), "set_draw_centers_of_mass", "get_draw_centers_of_mass");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_transforms"), "set_draw_transforms", "get_draw_transforms");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_velocities"), "set_draw_velocities", "get_draw_velocities");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_triangle_outlines"), "set_draw_triangle_outlines", "get_draw_triangle_outlines");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_constraint_reference_frames"), "set_draw_constraint_reference_frames", "get_draw_constraint_reference_frames");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_constraint_limits"), "set_draw_constraint_limits", "get_draw_constraint_limits");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_as_wireframe"), "set_draw_as_wireframe", "get_draw_as_wireframe");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "draw_with_color_scheme", PROPERTY_HINT_ENUM_SUGGESTION, "Instance,Shape Type,Motion Type,Sleep State,Island"), "set_draw_with_color_scheme", "get_draw_with_color_scheme");

	ADD_GROUP("Material", "material_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "material_depth_test"), "set_material_depth_test", "get_material_depth_test");

	BIND_ENUM_CONSTANT(COLOR_SCHEME_INSTANCE);
	BIND_ENUM_CONSTANT(COLOR_SCHEME_SHAPE_TYPE);
	BIND_ENUM_CONSTANT(COLOR_SCHEME_MOTION_TYPE);
	BIND_ENUM_CONSTANT(COLOR_SCHEME_SLEEP_STATE);
	BIND_ENUM_CONSTANT(COLOR_SCHEME_ISLAND);
}

#ifdef JPH_DEBUG_RENDERER

JoltDebugGeometry3D::JoltDebugGeometry3D() :
		mesh(RenderingServer::get_singleton()->mesh_create()), debug_renderer(JoltDebugRenderer3D::acquire()) {
	set_base(mesh);

	set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);

	default_material.instantiate();
	default_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	default_material->set_specular_mode(StandardMaterial3D::SPECULAR_DISABLED);
	default_material->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
}

JoltDebugGeometry3D::~JoltDebugGeometry3D() {
	if (mesh.is_valid()) {
		RenderingServer::get_singleton()->free(mesh);
	}

	JoltDebugRenderer3D::release(debug_renderer);
}

#else // JPH_DEBUG_RENDERER

JoltDebugGeometry3D::JoltDebugGeometry3D() = default;

JoltDebugGeometry3D::~JoltDebugGeometry3D() = default;

#endif // JPH_DEBUG_RENDERER

void JoltDebugGeometry3D::_notification(int p_what) {
#ifdef JPH_DEBUG_RENDERER

	if (p_what == NOTIFICATION_READY) {
		set_process_internal(true);
	}

	if (p_what != NOTIFICATION_INTERNAL_PROCESS) {
		return;
	}

	JPH::PhysicsSystem *physics_system = Jolt::singleton()->worlds[0];

	if (physics_system == nullptr) {
		ERR_PRINT_ONCE("JoltDebugGeometry3D was unable to retrieve the Jolt-based physics system.");
		return;
	}

	RenderingServer *rendering_server = RenderingServer::get_singleton();
	ERR_FAIL_NULL(rendering_server);

	const Viewport *viewport = get_viewport();
	ERR_FAIL_NULL(viewport);

	const Camera3D *camera = viewport->get_camera_3d();
	ERR_FAIL_NULL(camera);

	debug_renderer->draw(*physics_system, *camera, draw_settings);
	const int32_t surface_count = debug_renderer->submit(mesh);

	const RID material_rid = default_material->get_rid();

	for (int32_t i = 0; i < surface_count; ++i) {
		rendering_server->mesh_surface_set_material(mesh, i, material_rid);
	}
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_bodies() const {
#ifdef JPH_DEBUG_RENDERER
	return draw_settings.draw_bodies;
#else // JPH_DEBUG_RENDERER
	return true;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_bodies([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.draw_bodies = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_shapes() const {
#ifdef JPH_DEBUG_RENDERER
	return draw_settings.draw_shapes;
#else // JPH_DEBUG_RENDERER
	return true;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_shapes([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.draw_shapes = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_constraints() const {
#ifdef JPH_DEBUG_RENDERER
	return draw_settings.draw_constraints;
#else // JPH_DEBUG_RENDERER
	return true;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_constraints([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.draw_constraints = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_bounding_boxes() const {
#ifdef JPH_DEBUG_RENDERER
	return draw_settings.draw_bounding_boxes;
#else // JPH_DEBUG_RENDERER
	return false;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_bounding_boxes([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.draw_bounding_boxes = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_centers_of_mass() const {
#ifdef JPH_DEBUG_RENDERER
	return draw_settings.draw_centers_of_mass;
#else // JPH_DEBUG_RENDERER
	return false;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_centers_of_mass([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.draw_centers_of_mass = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_transforms() const {
#ifdef JPH_DEBUG_RENDERER
	return draw_settings.draw_transforms;
#else // JPH_DEBUG_RENDERER
	return false;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_transforms([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.draw_transforms = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_velocities() const {
#ifdef JPH_DEBUG_RENDERER
	return draw_settings.draw_velocities;
#else // JPH_DEBUG_RENDERER
	return false;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_velocities([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.draw_velocities = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_triangle_outlines() const {
#ifdef JPH_DEBUG_RENDERER
	return JPH::MeshShape::sDrawTriangleOutlines;
#else // JPH_DEBUG_RENDERER
	return false;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_triangle_outlines([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	JPH::MeshShape::sDrawTriangleOutlines = p_enabled;
	JPH::HeightFieldShape::sDrawTriangleOutlines = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_constraint_reference_frames() const {
#ifdef JPH_DEBUG_RENDERER
	return draw_settings.draw_constraint_reference_frames;
#else // JPH_DEBUG_RENDERER
	return false;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_constraint_reference_frames([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.draw_constraint_reference_frames = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_constraint_limits() const {
#ifdef JPH_DEBUG_RENDERER
	return draw_settings.draw_constraint_limits;
#else // JPH_DEBUG_RENDERER
	return false;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_constraint_limits([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.draw_constraint_limits = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_draw_as_wireframe() const {
#ifdef JPH_DEBUG_RENDERER
	return draw_settings.draw_as_wireframe;
#else // JPH_DEBUG_RENDERER
	return true;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_as_wireframe([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.draw_as_wireframe = p_enabled;
#endif // JPH_DEBUG_RENDERER
}

JoltDebugGeometry3D::ColorScheme JoltDebugGeometry3D::get_draw_with_color_scheme() const {
#ifdef JPH_DEBUG_RENDERER
	return (ColorScheme)draw_settings.color_scheme;
#else // JPH_DEBUG_RENDERER
	return ColorScheme::COLOR_SCHEME_SHAPE_TYPE;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_draw_with_color_scheme([[maybe_unused]] ColorScheme p_color_scheme) {
#ifdef JPH_DEBUG_RENDERER
	draw_settings.color_scheme = (JPH::BodyManager::EShapeColor)p_color_scheme;
#endif // JPH_DEBUG_RENDERER
}

bool JoltDebugGeometry3D::get_material_depth_test() const {
#ifdef JPH_DEBUG_RENDERER
	ERR_FAIL_NULL_V(default_material, true);
	return !default_material->get_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST);
#else // JPH_DEBUG_RENDERER
	return true;
#endif // JPH_DEBUG_RENDERER
}

void JoltDebugGeometry3D::set_material_depth_test([[maybe_unused]] bool p_enabled) {
#ifdef JPH_DEBUG_RENDERER
	ERR_FAIL_NULL(default_material);
	default_material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, !p_enabled);
#endif // JPH_DEBUG_RENDERER
}

static_assert(
		(int32_t)JoltDebugGeometry3D::COLOR_SCHEME_INSTANCE ==
		(int32_t)JPH::BodyManager::EShapeColor::InstanceColor);

static_assert(
		(int32_t)JoltDebugGeometry3D::COLOR_SCHEME_SHAPE_TYPE ==
		(int32_t)JPH::BodyManager::EShapeColor::ShapeTypeColor);

static_assert(
		(int32_t)JoltDebugGeometry3D::COLOR_SCHEME_MOTION_TYPE ==
		(int32_t)JPH::BodyManager::EShapeColor::MotionTypeColor);

static_assert(
		(int32_t)JoltDebugGeometry3D::COLOR_SCHEME_SLEEP_STATE ==
		(int32_t)JPH::BodyManager::EShapeColor::SleepColor);

static_assert(
		(int32_t)JoltDebugGeometry3D::COLOR_SCHEME_ISLAND ==
		(int32_t)JPH::BodyManager::EShapeColor::IslandColor);
