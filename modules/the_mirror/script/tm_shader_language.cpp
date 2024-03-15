
#include "tm_shader_language.h"
#include "core/core_string_names.h"
#include "core/object/object.h"

#include "servers/rendering/shader_language.h"
#include "servers/rendering/shader_preprocessor.h"
#include "servers/rendering/shader_types.h"

#include "servers/rendering_server.h"

TMShaderLanguage::TMShaderLanguage() :
		Object() {
}

void TMShaderLanguage::_bind_methods() {
	ClassDB::bind_method(D_METHOD("compile_spatial", "code"), &TMShaderLanguage::compile_spatial);
}

static ShaderLanguage::DataType _get_global_shader_uniform_type(const StringName &p_variable) {
	RS::GlobalShaderParameterType gvt = RS::get_singleton()->global_shader_parameter_get_type(p_variable);
	return (ShaderLanguage::DataType)RS::global_shader_uniform_type_get_shader_datatype(gvt);
}

static void _complete_include_paths(List<ScriptLanguage::CodeCompletionOption> *r_options) {
}

String TMShaderLanguage::compile_spatial(const String &p_code) const {
	ShaderLanguage::ShaderCompileInfo comp_info;
	RS::ShaderMode shader_mode = RS::SHADER_SPATIAL;
	Error last_compile_result = Error::OK;

	List<ScriptLanguage::CodeCompletionOption> pp_options;
	List<ScriptLanguage::CodeCompletionOption> pp_defines;
	ShaderPreprocessor preprocessor;
	String code;
	preprocessor.preprocess(p_code, "", code, nullptr, nullptr, nullptr, nullptr, &pp_options, &pp_defines, _complete_include_paths);

	ShaderLanguage sl;
	comp_info.global_shader_uniform_type_func = _get_global_shader_uniform_type;
	comp_info.functions = ShaderTypes::get_singleton()->get_functions(shader_mode);
	comp_info.render_modes = ShaderTypes::get_singleton()->get_modes(shader_mode);
	comp_info.shader_types = ShaderTypes::get_singleton()->get_types();

	last_compile_result = sl.compile(code, comp_info);

	if (last_compile_result != OK) {
		String err_text = "";
		int err_line;
		Vector<ShaderLanguage::FilePosition> include_positions = sl.get_include_positions();
		if (include_positions.size() > 1) {
			//error is in an include
			err_line = include_positions[0].line;
			err_text += "error(" + itos(err_line) + ") in include " + include_positions[include_positions.size() - 1].file + ":" + itos(include_positions[include_positions.size() - 1].line) + ": " + sl.get_error_text() + "\n";
		} else {
			err_line = sl.get_error_line();
			err_text += "error(" + itos(err_line) + "): " + sl.get_error_text() + "\n";
		}
		return err_text;
	}
	return "";
}
