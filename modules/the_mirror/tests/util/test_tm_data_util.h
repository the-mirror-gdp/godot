
#ifndef TEST_TM_DATA_UTIL_H
#define TEST_TM_DATA_UTIL_H

#include "../../util/tm_data_util.h"
#include "tests/test_macros.h"

namespace TestTMDataUtil {

TEST_CASE("[TMDataUtil] JSON Path Splitting") {
	PackedStringArray abc_expected = { "a", "b", "c" };
	PackedStringArray abc1 = TMDataUtil::split_json_path_string("a/b/c");
	PackedStringArray abc2 = TMDataUtil::split_json_path_string("//a//b//c///");
	PackedStringArray abc3 = TMDataUtil::split_json_path_string("a.b.c");
	PackedStringArray abc4 = TMDataUtil::split_json_path_string(".a.b.c.");
	CHECK(abc1 == abc_expected);
	CHECK(abc2 == abc_expected);
	CHECK(abc3 == abc_expected);
	CHECK(abc4 == abc_expected);

	PackedStringArray empty_expected = {};
	PackedStringArray empty1 = TMDataUtil::split_json_path_string("");
	PackedStringArray empty2 = TMDataUtil::split_json_path_string(".");
	PackedStringArray empty3 = TMDataUtil::split_json_path_string("//");
	CHECK(empty1 == empty_expected);
	CHECK(empty2 == empty_expected);
	CHECK(empty3 == empty_expected);
}

TEST_CASE("[TMDataUtil] JSON Path Matching") {
	PackedStringArray ab = { "a", "b" };
	PackedStringArray abcd = { "a", "b", "c", "d" };
	PackedStringArray abe = { "a", "b", "e" };
	CHECK(TMDataUtil::match_depth_json_path(ab, ab) == 0);
	CHECK(TMDataUtil::match_depth_json_path(ab, abcd) == 2);
	CHECK(TMDataUtil::match_depth_json_path(ab, abe) == 1);
	CHECK(TMDataUtil::match_depth_json_path(abcd, ab) == -1);
	CHECK(TMDataUtil::match_depth_json_path(abcd, abcd) == 0);
	CHECK(TMDataUtil::match_depth_json_path(abcd, abe) == -1);
	CHECK(TMDataUtil::match_depth_json_path(abe, ab) == -1);
	CHECK(TMDataUtil::match_depth_json_path(abe, abcd) == -1);
	CHECK(TMDataUtil::match_depth_json_path(abe, abe) == 0);
}

TEST_CASE("[TMDataUtil] JSON Path Get Parent Data Structure") {
	Dictionary dict;
	Dictionary a_dict;
	Dictionary b_dict;
	b_dict["c"] = 2;
	a_dict["b"] = b_dict;
	dict["a"] = a_dict;

	CHECK(TMDataUtil::get_parent_data_structure_for_json_path(dict, { "a", "b" }, false) == a_dict);
	CHECK(TMDataUtil::get_parent_data_structure_for_json_path(dict, { "a", "b", "c" }, false) == b_dict);
	CHECK(TMDataUtil::get_parent_data_structure_for_json_path(dict, { "a", "d", "e" }, false) == Variant());
	CHECK_MESSAGE(a_dict.size() == 1, "TMDataUtil::get_parent_data_structure_for_json_path should not create missing data structures when create_missing is false.");
	TMDataUtil::get_parent_data_structure_for_json_path(dict, { "a", "d", "e" }, true);
	CHECK_MESSAGE(a_dict.size() == 2, "TMDataUtil::get_parent_data_structure_for_json_path should create missing data structures when create_missing is true.");

	TMDataUtil::get_parent_data_structure_for_json_path(dict, { "a", "i", "2" }, true);
	Variant ai = TMDataUtil::get_variable_by_json_path_string(dict, "a/i");
	CHECK_MESSAGE(ai.get_type() == Variant::ARRAY, "TMDataUtil::get_parent_data_structure_for_json_path should create arrays when the next name is an integer.");
	CHECK_MESSAGE(Array(ai).size() == 3, "TMDataUtil::get_parent_data_structure_for_json_path should create arrays with the correct size when the next name is an integer.");
}

TEST_CASE("[TMDataUtil] JSON Path Get") {
	Dictionary dict;
	dict["a"] = 1;
	Dictionary b_dict;
	b_dict["c"] = 2;
	dict["b"] = b_dict;
	b_dict["mynull"] = Variant();

	CHECK(TMDataUtil::get_variable_by_json_path_string(dict, "/a") == Variant(1));
	CHECK(TMDataUtil::get_variable_by_json_path_string(dict, "b.c.") == Variant(2));
	Variant null_result = TMDataUtil::get_variable_by_json_path_string(dict, "b/mynull");
	CHECK(null_result.get_type() == Variant::NIL);
	CHECK(!null_result.is_signaling_null());
	Variant undefined_result = TMDataUtil::get_variable_by_json_path_string(dict, "myundefined");
	CHECK(undefined_result.is_signaling_null());

	PackedStringArray a = { "a" };
	PackedStringArray bc = { "b", "c" };
	PackedStringArray bcd = { "b", "c", "d" };
	CHECK(TMDataUtil::get_variable_by_json_path(dict, a) == Variant(1));
	CHECK(TMDataUtil::get_variable_by_json_path(dict, bc) == Variant(2));
	Variant undefined_bcd_result = TMDataUtil::get_variable_by_json_path(dict, bcd);
	CHECK(undefined_bcd_result.is_signaling_null());

	CHECK_MESSAGE(dict.size() == 2, "TMDataUtil::get_variable_by_json_path* should not create missing keys by default.");
	TMDataUtil::get_variable_by_json_path_string(dict, "newkey", true);
	CHECK_MESSAGE(dict.size() == 3, "TMDataUtil::get_variable_by_json_path* should create missing keys when asked.");
	TMDataUtil::get_variable_by_json_path_string(dict, "newparent/newkey", true);
	CHECK_MESSAGE(dict.size() == 4, "TMDataUtil::get_variable_by_json_path* should create missing data structures when asked.");
}

TEST_CASE("[TMDataUtil] JSON Path Set") {
	Dictionary dict;
	TMDataUtil::set_variable_by_json_path_string(dict, "a/b/c/0/d/e", 123);
	Variant c = TMDataUtil::get_variable_by_json_path_string(dict, "a/b/c");
	CHECK_MESSAGE(c.get_type() == Variant::ARRAY, "TMDataUtil::set_variable_by_json_path should create arrays when the next name is an integer.");
	CHECK_MESSAGE(Array(c).size() == 1, "TMDataUtil::set_variable_by_json_path should create arrays with the correct size when the next name is an integer.");
	Variant d = TMDataUtil::get_variable_by_json_path_string(dict, "a/b/c/0/d");
	CHECK_MESSAGE(d.get_type() == Variant::DICTIONARY, "TMDataUtil::set_variable_by_json_path should create dictionaries when the next name is not an integer.");
	Variant e = TMDataUtil::get_variable_by_json_path_string(dict, "a/b/c/0/d/e");
	CHECK_MESSAGE(e == Variant(123), "TMDataUtil::set_variable_by_json_path should set the correct value.");

	TMDataUtil::set_variable_by_json_path_string(dict, "a/b/f/g/4", 567, false);
	Variant f = TMDataUtil::get_variable_by_json_path_string(dict, "a/b/f");
	CHECK_MESSAGE(f.is_signaling_null(), "TMDataUtil::set_variable_by_json_path should not create missing data structures when create_missing is false.");

	ERR_PRINT_OFF;
	// Will print: TMDataUtil::get_parent_data_structure_for_json_path: Index out of bounds, must be between 0 and 999.
	TMDataUtil::set_variable_by_json_path_string(dict, "arr/-1", -2, true);
	CHECK_MESSAGE(!dict.has("arr"), "TMDataUtil::set_variable_by_json_path should not create arrays when the next name is a negative integer.");
	ERR_PRINT_ON;
}

} // namespace TestTMDataUtil

#endif // TEST_TM_DATA_UTIL_H
