
#ifndef TM_NODE_UTIL_H
#define TM_NODE_UTIL_H

#include "core/object/class_db.h"

class Node;
class Node3D;

class TMNodeUtil : public Object {
	GDCLASS(TMNodeUtil, Object);

protected:
	static TMNodeUtil *singleton;
	static void _bind_methods();

public:
	// Misc.
	static TypedArray<Node> get_all_descendants(const Node *p_node);
	static AABB get_local_aabb_of_descendants(const Node3D *p_node);
	static Vector3 get_local_bottom_point(const Node3D *p_node);
	static String get_relative_node_path_string(const Node *p_ancestor, const Node *p_descendant);
	static Transform3D get_relative_transform(const Node3D *p_ancestor, const Node3D *p_descendant);
	static String get_unique_child_name(const Node *p_parent, const String &p_child_name);

	// Get multiple nodes.
	static TypedArray<Node> recursive_find_nodes_by_meta(const Node *p_start_node, const StringName &p_meta_key);
	static TypedArray<Node> recursive_find_nodes_by_type(const Node *p_start_node, const Object *p_type_object);
	static TypedArray<Node> recursive_find_nodes_by_type_string(const Node *p_start_node, const StringName &p_type_string);

	// Get single nodes.
	static Node *recursive_get_node_by_meta(const Node *p_start_node, const StringName &p_meta_key);
	static Node *recursive_get_node_by_name(const Node *p_start_node, const StringName &p_sname);
	static Node *recursive_get_node_by_type(const Node *p_start_node, const Object *p_type_object);
	static Node *recursive_get_node_by_type_string(const Node *p_start_node, const String &p_type_string);

	TMNodeUtil();
	~TMNodeUtil();
};

#endif // TM_NODE_UTIL_H
