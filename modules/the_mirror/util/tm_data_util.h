
#ifndef TM_DATA_UTIL_H
#define TM_DATA_UTIL_H

#include "core/object/class_db.h"

// This class is stateless.
class TMDataUtil : public Object {
	GDCLASS(TMDataUtil, Object);

protected:
	static TMDataUtil *singleton;
	static void _bind_methods();

public:
	// JSON path functions, for working with paths like "a/b/c" or "a.b.c" or "/items/0/name" (numbers for arrays).
	static Variant get_variable_by_json_path_string(const Variant &p_data_structure, const String &p_json_path_string, bool p_create_missing = false);
	static Variant get_variable_by_json_path(const Variant &p_data_structure, const PackedStringArray &p_json_path, bool p_create_missing = false);
	static bool has_variable_by_json_path_string(const Variant &p_data_structure, const String &p_json_path_string);
	static bool has_variable_by_json_path(const Variant &p_data_structure, const PackedStringArray &p_json_path);
	static void set_variable_by_json_path_string(Variant p_data_structure, const String &p_json_path_string, const Variant &p_value, bool p_create_missing = true);
	static void set_variable_by_json_path(Variant p_data_structure, const PackedStringArray &p_json_path, const Variant &p_value, bool p_create_missing = true);
	static Variant get_parent_data_structure_for_json_path(const Variant &p_start, const PackedStringArray &p_json_path, bool p_create_missing);
	static PackedStringArray split_json_path_string(const String &p_json_path_string);
	static int match_depth_json_path(const PackedStringArray &p_signal_path, const PackedStringArray &p_listen_path);

	TMDataUtil();
	~TMDataUtil();
};

#endif // TM_DATA_UTIL_H
