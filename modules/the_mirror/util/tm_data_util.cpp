#include "tm_data_util.h"

const int _MAX_ARRAY_SIZE = 1000;

Variant TMDataUtil::get_variable_by_json_path_string(const Variant &p_data_structure, const String &p_json_path_string, bool p_create_missing) {
	PackedStringArray split_path = split_json_path_string(p_json_path_string);
	return get_variable_by_json_path(p_data_structure, split_path, p_create_missing);
}

Variant TMDataUtil::get_variable_by_json_path(const Variant &p_data_structure, const PackedStringArray &p_json_path, bool p_create_missing) {
	ERR_FAIL_COND_V_MSG(p_json_path.size() == 0, Variant(), "TMDataUtil::get_variable_by_json_path: json_path must not be empty.");
	Variant data_structure = get_parent_data_structure_for_json_path(p_data_structure, p_json_path, p_create_missing);
	const String &final_name = p_json_path[p_json_path.size() - 1];
	const Variant::Type data_structure_type = data_structure.get_type();
	if (likely(data_structure_type == Variant::DICTIONARY)) {
		Dictionary dict = data_structure;
		if (dict.has(final_name)) {
			return dict[final_name];
		}
		if (p_create_missing) {
			dict[final_name] = Variant();
			return dict[final_name];
		}
	} else if (data_structure_type == Variant::ARRAY) {
		Array arr = data_structure;
		const int index = final_name.to_int();
		if (index < arr.size()) {
			return arr[index];
		}
		if (p_create_missing) {
			ERR_FAIL_INDEX_V_MSG(index, _MAX_ARRAY_SIZE, Variant::create_signaling_null(), "TMDataUtil::get_variable_by_json_path: Index out of bounds, must be between 0 and 999.");
			arr.resize(index + 1);
			return arr[index];
		}
	}
	// A signaling null indicates no variable existed (as opposed to a null Variant at this path).
	return Variant::create_signaling_null();
}

bool TMDataUtil::has_variable_by_json_path_string(const Variant &p_data_structure, const String &p_json_path_string) {
	PackedStringArray split_path = split_json_path_string(p_json_path_string);
	return has_variable_by_json_path(p_data_structure, split_path);
}

bool TMDataUtil::has_variable_by_json_path(const Variant &p_data_structure, const PackedStringArray &p_json_path) {
	ERR_FAIL_COND_V_MSG(p_json_path.size() == 0, Variant(), "TMDataUtil::get_variable_by_json_path: json_path must not be empty.");
	Variant data_structure = get_parent_data_structure_for_json_path(p_data_structure, p_json_path, false);
	if (data_structure.get_type() == Variant::NIL) {
		return false;
	}
	const String &final_name = p_json_path[p_json_path.size() - 1];
	const Variant::Type data_structure_type = data_structure.get_type();
	if (likely(data_structure_type == Variant::DICTIONARY)) {
		Dictionary dict = data_structure;
		return dict.has(final_name);
	} else if (data_structure_type == Variant::ARRAY) {
		Array arr = data_structure;
		const int index = final_name.to_int();
		return index < arr.size();
	}
	return false;
}

void TMDataUtil::set_variable_by_json_path_string(Variant p_data_structure, const String &p_json_path_string, const Variant &p_value, bool p_create_missing) {
	PackedStringArray split_path = split_json_path_string(p_json_path_string);
	set_variable_by_json_path(p_data_structure, split_path, p_value, p_create_missing);
}

void TMDataUtil::set_variable_by_json_path(Variant p_data_structure, const PackedStringArray &p_json_path, const Variant &p_value, bool p_create_missing) {
	ERR_FAIL_COND_MSG(p_json_path.size() == 0, "TMDataUtil::get_variable_by_json_path: json_path must not be empty.");
	Variant data_structure = get_parent_data_structure_for_json_path(p_data_structure, p_json_path, p_create_missing);
	if (!p_create_missing && data_structure.is_signaling_null()) {
		return; // A signaling null indicates no data structure (as opposed to a null Variant where a data structure was expected), and we were told not to create missing variables, so exit silently.
	}
	const String &final_name = p_json_path[p_json_path.size() - 1];
	const Variant::Type data_structure_type = data_structure.get_type();
	if (likely(data_structure_type == Variant::DICTIONARY)) {
		Dictionary dict = data_structure;
		dict[final_name] = p_value;
	} else if (data_structure_type == Variant::ARRAY) {
		Array arr = data_structure;
		const int index = final_name.to_int();
		if (index >= arr.size()) {
			if (p_create_missing && index >= arr.size()) {
				ERR_FAIL_INDEX_MSG(index, _MAX_ARRAY_SIZE, "TMDataUtil::set_variable_by_json_path: Index out of bounds, must be between 0 and 999.");
				arr.resize(index + 1);
			} else {
				ERR_FAIL_MSG("TMDataUtil::set_variable_by_json_path: Index is larger than array size and create_missing is false.");
			}
		}
		arr[index] = p_value;
	} else {
		ERR_PRINT("TMDataUtil::set_variable_by_json_path: Failed to set value, data_structure is not a Dictionary or Array.");
	}
}

Variant TMDataUtil::get_parent_data_structure_for_json_path(const Variant &p_start, const PackedStringArray &p_json_path, bool p_create_missing) {
	Variant data_structure = p_start;
	const int end = p_json_path.size() - 1;
	for (int i = 0; i < end; i++) { // Look through everything, but skip the last element.
		const String &item_name = p_json_path[i];
		const Variant::Type data_structure_type = data_structure.get_type();
		if (likely(data_structure_type == Variant::DICTIONARY)) {
			Dictionary dict = data_structure;
			if (dict.has(item_name) && dict[item_name] != Variant()) {
				data_structure = dict[item_name];
			} else if (p_create_missing) {
				// We need to insert a new data structure, but array or dict?
				const String &next_name = p_json_path[i + 1];
				if (next_name.is_valid_int()) {
					Array new_array;
					const int next_index = next_name.to_int();
					ERR_FAIL_INDEX_V_MSG(next_index, _MAX_ARRAY_SIZE, Variant(), "TMDataUtil::get_parent_data_structure_for_json_path: Index out of bounds, must be between 0 and 999.");
					new_array.resize(next_index + 1);
					dict[item_name] = new_array;
				} else {
					dict[item_name] = Dictionary();
				}
				data_structure = dict[item_name];
			} else {
				return Variant::create_signaling_null(); // null
			}
		} else if (data_structure_type == Variant::ARRAY) {
			Array arr = data_structure;
			const int item_index = item_name.to_int();
			if (item_index < arr.size() && arr[item_index] != Variant()) {
				data_structure = arr[item_index];
			} else if (p_create_missing) {
				arr.resize(item_index + 1);
				// We need to insert a new data structure, but array or dict?
				// Ex: If we have a/b and are asked for a/b/c/0, we need to
				// create a/b/c as an array to make it have numbered elements.
				const String &next_name = p_json_path[i + 1];
				if (next_name.is_valid_int()) {
					Array new_array;
					const int next_index = next_name.to_int();
					ERR_FAIL_INDEX_V_MSG(next_index, _MAX_ARRAY_SIZE, Variant(), "TMDataUtil::get_parent_data_structure_for_json_path: Index out of bounds, must be between 0 and 999.");
					new_array.resize(next_index + 1);
					arr[item_index] = new_array;
				} else {
					arr[item_index] = Dictionary();
				}
				data_structure = arr[item_index];
			} else {
				return Variant::create_signaling_null(); // null
			}
		} else {
			ERR_PRINT("TMDataUtil::get_parent_data_structure_for_json_path: Failed to get parent data structure, data_structure is not a Dictionary or Array.");
			return Variant::create_signaling_null(); // null
		}
	}
	return data_structure;
}

PackedStringArray TMDataUtil::split_json_path_string(const String &p_json_path_string) {
	// Allow referencing nested items using either . or / characters.
	String json_path_string = p_json_path_string;
	if (json_path_string.contains(".")) {
		json_path_string = json_path_string.replace(".", "/");
	}
	return json_path_string.split("/", false);
}

int TMDataUtil::match_depth_json_path(const PackedStringArray &p_signal_path, const PackedStringArray &p_listen_path) {
	const int signal_path_size = p_signal_path.size();
	const int listen_path_size = p_listen_path.size();
	// Example: if a/b/c vs a/b, not a match so return -1.
	if (signal_path_size > listen_path_size) {
		return -1;
	}
	// Example: if a/b/c vs a/b/c, we return 0 (no depth).
	// Example: if a/b vs a/b/c, we return 1 (signal for a/b, must get /c)
	int depth = listen_path_size;
	for (int i = 0; i < signal_path_size; i++) {
		if (p_signal_path[i] == p_listen_path[i]) {
			depth--;
		} else {
			// Example: if a/b vs a/c, not a match so return -1.
			return -1;
		}
	}
	return depth;
}

// Boring boilerplate and bindings.

TMDataUtil *TMDataUtil::singleton = nullptr;

TMDataUtil::TMDataUtil() {
	singleton = this;
}

TMDataUtil::~TMDataUtil() {
	singleton = nullptr;
}

void TMDataUtil::_bind_methods() {
	ClassDB::bind_static_method("TMDataUtil", D_METHOD("get_variable_by_json_path_string", "data_structure", "json_path_string", "create_missing"), &TMDataUtil::get_variable_by_json_path_string, DEFVAL(false));
	ClassDB::bind_static_method("TMDataUtil", D_METHOD("get_variable_by_json_path", "data_structure", "json_path", "create_missing"), &TMDataUtil::get_variable_by_json_path, DEFVAL(false));
	ClassDB::bind_static_method("TMDataUtil", D_METHOD("has_variable_by_json_path_string", "data_structure", "json_path_string"), &TMDataUtil::has_variable_by_json_path_string);
	ClassDB::bind_static_method("TMDataUtil", D_METHOD("has_variable_by_json_path", "data_structure", "json_path"), &TMDataUtil::has_variable_by_json_path);
	ClassDB::bind_static_method("TMDataUtil", D_METHOD("set_variable_by_json_path_string", "data_structure", "json_path_string", "value", "create_missing"), &TMDataUtil::set_variable_by_json_path_string, DEFVAL(true));
	ClassDB::bind_static_method("TMDataUtil", D_METHOD("set_variable_by_json_path", "data_structure", "json_path", "value", "create_missing"), &TMDataUtil::set_variable_by_json_path, DEFVAL(true));
	ClassDB::bind_static_method("TMDataUtil", D_METHOD("get_parent_data_structure_for_json_path", "data_structure", "json_path", "create_missing"), &TMDataUtil::get_parent_data_structure_for_json_path);
	ClassDB::bind_static_method("TMDataUtil", D_METHOD("split_json_path_string", "json_path_string"), &TMDataUtil::split_json_path_string);
	ClassDB::bind_static_method("TMDataUtil", D_METHOD("match_depth_json_path", "signal_path", "listen_path"), &TMDataUtil::match_depth_json_path);
}
