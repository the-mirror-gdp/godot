
#ifndef TEST_TM_FILE_UTIL_H
#define TEST_TM_FILE_UTIL_H

#include "../../util/tm_file_util.h"
#include "tests/test_macros.h"

namespace TestTMFileUtil {

TEST_CASE("[TMFileUtil] Clean String For File Path") {
	String question = "test?test";
	String at_sign = "test@test";
	CHECK(TMFileUtil::clean_string_for_file_path(question) == String("testtest"));
	CHECK(TMFileUtil::clean_string_for_file_path(at_sign) == String("test_test"));
}

TEST_CASE("[TMFileUtil] Parse JSON From String") {
	String test_json_str = "  { \"numTest\": 000484884,  \"stringTest\": \"hello world\" } ";
	Variant json_variant = TMFileUtil::parse_json_from_string(test_json_str);
	CHECK(json_variant.get_type() == Variant::DICTIONARY);
	Dictionary json_dict = json_variant;
	CHECK(json_dict["numTest"].get_type() == Variant::FLOAT);
	CHECK(double(json_dict["numTest"]) == 484884.0);
	CHECK(json_dict["stringTest"] == "hello world");
}

} // namespace TestTMFileUtil

#endif // TEST_TM_FILE_UTIL_H
