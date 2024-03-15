#include "tm_node_util.h"

#include "modules/gdscript/gdscript.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/sprite_3d.h"

// Misc.

void _get_all_descendants_internal(const Node *p_node, TypedArray<Node> &p_descendants) {
	for (int i = 0; i < p_node->get_child_count(); i++) {
		const Node *child = p_node->get_child(i);
		p_descendants.push_back(child);
		_get_all_descendants_internal(child, p_descendants);
	}
}

TypedArray<Node> TMNodeUtil::get_all_descendants(const Node *p_node) {
	TypedArray<Node> descendants;
	ERR_FAIL_NULL_V(p_node, descendants);
	_get_all_descendants_internal(p_node, descendants);
	return descendants;
}

AABB TMNodeUtil::get_local_aabb_of_descendants(const Node3D *p_node) {
	ERR_FAIL_NULL_V(p_node, AABB());
	TypedArray<Node> descendants = get_all_descendants(p_node);
	AABB aabb;
	for (int i = 0; i < descendants.size(); i++) {
		// You would hope that TypedArray<Node> stores Node, but it actually stores Variant.
		Node3D *child = Object::cast_to<Node3D>(descendants[i].operator Object *());
		if (!child) {
			continue; // Not a Node3D, it has no position or volume in 3D space.
		}
		if (child->get_name() == StringName("_PlaceholderMesh")) {
			continue; // Special case for The Mirror: Don't include placeholder meshes.
		}
		const Transform3D rel_transform = get_relative_transform(p_node, child);
		AABB child_aabb;
		if (Object::cast_to<Sprite3D>(child)) {
			const Sprite3D *sprite = Object::cast_to<Sprite3D>(child);
			const Vector2i texture_size = sprite->get_texture()->get_size();
			const real_t size = sprite->get_pixel_size() * MAX(texture_size.x, texture_size.y);
			const Vector3 start = rel_transform.get_origin() - 0.5f * size * Vector3(1, 1, 1);
			child_aabb = AABB(start, Vector3(1, 1, 1) * size);
		} else if (Object::cast_to<GeometryInstance3D>(child)) {
			// Note: Check for GeometryInstance3D instead of the `get_aabb()`
			// method, since we do not want nodes like Light3D to be included.
			child_aabb = rel_transform.xform(Object::cast_to<GeometryInstance3D>(child)->get_aabb());
		} else if (Object::cast_to<CollisionShape3D>(child)) {
			Ref<ArrayMesh> child_shape_mesh = Object::cast_to<CollisionShape3D>(child)->get_shape()->get_debug_mesh();
			child_aabb = rel_transform.xform(child_shape_mesh->get_aabb());
		} else {
			continue;
		}
		if (aabb.size == Vector3()) {
			aabb = child_aabb;
		} else {
			aabb = aabb.merge(child_aabb);
		}
	}
	return aabb;
}

Vector3 TMNodeUtil::get_local_bottom_point(const Node3D *p_node) {
	ERR_FAIL_NULL_V(p_node, Vector3());
	AABB local_aabb = get_local_aabb_of_descendants(p_node);
	Vector3 local_bottom = local_aabb.get_center();
	local_bottom.y -= local_aabb.size.y * 0.5;
	return local_bottom;
}

String TMNodeUtil::get_relative_node_path_string(const Node *p_ancestor, const Node *p_descendant) {
	ERR_FAIL_NULL_V(p_descendant, "");
	String path_string = p_descendant->get_name();
	const Node *parent = p_descendant->get_parent();
	while (parent != p_ancestor) {
		path_string = String(parent->get_name()) + "/" + path_string;
		parent = parent->get_parent();
	}
	return path_string;
}

Transform3D TMNodeUtil::get_relative_transform(const Node3D *p_ancestor, const Node3D *p_descendant) {
	ERR_FAIL_NULL_V(p_descendant, Transform3D());
	Transform3D transform = p_descendant->get_transform();
	const Node *parent = p_descendant->get_parent();
	while (parent != p_ancestor) {
		ERR_FAIL_NULL_V_MSG(parent, transform, "Ancestor and descendant are not in the same node hierarchy.");
		const Node3D *parent_3d = Object::cast_to<Node3D>(parent);
		ERR_FAIL_NULL_V_MSG(parent_3d, transform, "Ancestor and descendant are not in the same 3D transform tree. Node " + parent->get_name() + " is not a Node3D.");
		transform = parent_3d->get_transform() * transform;
		parent = parent->get_parent();
	}
	return transform;
}

String TMNodeUtil::get_unique_child_name(const Node *p_parent, const String &p_child_name) {
	ERR_FAIL_NULL_V(p_parent, "");
	String base_name = p_child_name;
	String new_child_name = p_child_name;
	if (p_child_name.contains("/")) {
		base_name = p_child_name.substr(p_child_name.rfind("/"));
		new_child_name = base_name;
	}
	int i = 2;
	while (p_parent->has_node(new_child_name)) {
		new_child_name = base_name + itos(i);
		i++;
	}
	return new_child_name;
}

// Get multiple nodes.

void _recursive_find_nodes_by_meta_internal(const Node *p_start_node, const StringName &p_meta_key, TypedArray<Node> &p_nodes) {
	for (int i = 0; i < p_start_node->get_child_count(); i++) {
		Node *child = p_start_node->get_child(i);
		if (child->has_meta(p_meta_key)) {
			p_nodes.push_back(child);
		}
		_recursive_find_nodes_by_meta_internal(child, p_meta_key, p_nodes);
	}
}

TypedArray<Node> TMNodeUtil::recursive_find_nodes_by_meta(const Node *p_start_node, const StringName &p_meta_key) {
	TypedArray<Node> nodes;
	ERR_FAIL_NULL_V(p_start_node, nodes);
	_recursive_find_nodes_by_meta_internal(p_start_node, p_meta_key, nodes);
	return nodes;
}

void _recursive_find_nodes_by_type_internal(const Node *p_start_node, const Object *p_type_object, TypedArray<Node> &p_nodes) {
	const GDScriptNativeClass *native_type = Object::cast_to<GDScriptNativeClass>(p_type_object);
	const Script *script_type = Object::cast_to<Script>(p_type_object);
	for (int i = 0; i < p_start_node->get_child_count(); i++) {
		Node *child = p_start_node->get_child(i);
		if (native_type) {
			if (ClassDB::is_parent_class(child->get_class_name(), native_type->get_name())) {
				p_nodes.push_back(child);
			}
		} else if (script_type) {
			if (child->get_script_instance()) {
				Script *script_ptr = child->get_script_instance()->get_script().ptr();
				while (script_ptr) {
					if (script_ptr == script_type) {
						p_nodes.push_back(child);
						break;
					}
					script_ptr = script_ptr->get_base_script().ptr();
				}
			}
		}
		_recursive_find_nodes_by_type_internal(child, p_type_object, p_nodes);
	}
}

TypedArray<Node> TMNodeUtil::recursive_find_nodes_by_type(const Node *p_start_node, const Object *p_type_object) {
	TypedArray<Node> nodes;
	ERR_FAIL_NULL_V(p_start_node, nodes);
	_recursive_find_nodes_by_type_internal(p_start_node, p_type_object, nodes);
	return nodes;
}

void _recursive_find_nodes_by_type_string_internal(const Node *p_start_node, const StringName &p_type_string, TypedArray<Node> &p_nodes) {
	for (int i = 0; i < p_start_node->get_child_count(); i++) {
		Node *child = p_start_node->get_child(i);
		if (ClassDB::is_parent_class(child->get_class_name(), p_type_string)) {
			p_nodes.push_back(child);
		}
		_recursive_find_nodes_by_type_string_internal(child, p_type_string, p_nodes);
	}
}

TypedArray<Node> TMNodeUtil::recursive_find_nodes_by_type_string(const Node *p_start_node, const StringName &p_type_string) {
	TypedArray<Node> nodes;
	ERR_FAIL_NULL_V(p_start_node, nodes);
	_recursive_find_nodes_by_type_string_internal(p_start_node, p_type_string, nodes);
	return nodes;
}

// Get single nodes.

Node *TMNodeUtil::recursive_get_node_by_meta(const Node *p_start_node, const StringName &p_meta_key) {
	ERR_FAIL_NULL_V(p_start_node, nullptr);
	for (int i = 0; i < p_start_node->get_child_count(); i++) {
		Node *child = p_start_node->get_child(i);
		if (child->has_meta(p_meta_key)) {
			return child;
		}
		Node *next_child = recursive_get_node_by_meta(child, p_meta_key);
		if (next_child) {
			return next_child;
		}
	}
	return nullptr;
}

Node *TMNodeUtil::recursive_get_node_by_name(const Node *p_start_node, const StringName &p_sname) {
	ERR_FAIL_NULL_V(p_start_node, nullptr);
	for (int i = 0; i < p_start_node->get_child_count(); i++) {
		Node *child = p_start_node->get_child(i);
		if (child->get_name() == p_sname) {
			return child;
		}
		Node *next_child = recursive_get_node_by_name(child, p_sname);
		if (next_child) {
			return next_child;
		}
	}
	return nullptr;
}

Node *TMNodeUtil::recursive_get_node_by_type(const Node *p_start_node, const Object *p_type_object) {
	ERR_FAIL_NULL_V(p_start_node, nullptr);
	const GDScriptNativeClass *native_type = Object::cast_to<GDScriptNativeClass>(p_type_object);
	const Script *script_type = Object::cast_to<Script>(p_type_object);
	for (int i = 0; i < p_start_node->get_child_count(); i++) {
		Node *child = p_start_node->get_child(i);
		if (native_type) {
			if (ClassDB::is_parent_class(child->get_class_name(), native_type->get_name())) {
				return child;
			}
		} else if (script_type) {
			if (child->get_script_instance()) {
				Script *script_ptr = child->get_script_instance()->get_script().ptr();
				while (script_ptr) {
					if (script_ptr == script_type) {
						return child;
					}
					script_ptr = script_ptr->get_base_script().ptr();
				}
			}
		}
		Node *next_child = recursive_get_node_by_type(child, p_type_object);
		if (next_child) {
			return next_child;
		}
	}
	return nullptr;
}

Node *TMNodeUtil::recursive_get_node_by_type_string(const Node *p_start_node, const String &p_type_string) {
	ERR_FAIL_NULL_V(p_start_node, nullptr);
	for (int i = 0; i < p_start_node->get_child_count(); i++) {
		Node *child = p_start_node->get_child(i);
		if (ClassDB::is_parent_class(child->get_class_name(), p_type_string)) {
			return child;
		}
		Node *next_child = recursive_get_node_by_type_string(child, p_type_string);
		if (next_child) {
			return next_child;
		}
	}
	return nullptr;
}

// Boring boilerplate and bindings.

TMNodeUtil *TMNodeUtil::singleton = nullptr;

TMNodeUtil::TMNodeUtil() {
	singleton = this;
}

TMNodeUtil::~TMNodeUtil() {
	singleton = nullptr;
}

void TMNodeUtil::_bind_methods() {
	// Misc.
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("get_all_descendants", "node"), &TMNodeUtil::get_all_descendants);
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("get_local_aabb_of_descendants", "node"), &TMNodeUtil::get_local_aabb_of_descendants);
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("get_local_bottom_point", "node"), &TMNodeUtil::get_local_bottom_point);
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("get_relative_node_path_string", "ancestor", "descendant"), &TMNodeUtil::get_relative_node_path_string);
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("get_relative_transform", "ancestor", "descendant"), &TMNodeUtil::get_relative_transform);
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("get_unique_child_name", "parent", "child_name"), &TMNodeUtil::get_unique_child_name);

	// Get multiple nodes.
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("recursive_find_nodes_by_meta", "start_node", "meta_key"), &TMNodeUtil::recursive_find_nodes_by_meta);
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("recursive_find_nodes_by_type", "start_node", "type_object"), &TMNodeUtil::recursive_find_nodes_by_type);
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("recursive_find_nodes_by_type_string", "start_node", "type_string"), &TMNodeUtil::recursive_find_nodes_by_type_string);

	// Get single nodes.
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("recursive_get_node_by_name", "start_node", "string_name"), &TMNodeUtil::recursive_get_node_by_name);
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("recursive_get_node_by_type", "start_node", "type_object"), &TMNodeUtil::recursive_get_node_by_type);
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("recursive_get_node_by_type_string", "start_node", "type_string"), &TMNodeUtil::recursive_get_node_by_type_string);
	ClassDB::bind_static_method("TMNodeUtil", D_METHOD("recursive_get_node_by_meta", "start_node", "meta_key"), &TMNodeUtil::recursive_get_node_by_meta);
}
