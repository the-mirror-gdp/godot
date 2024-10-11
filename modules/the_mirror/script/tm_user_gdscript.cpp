#include "tm_user_gdscript.h"

#include "modules/gdscript/gdscript_analyzer.h"
#include "modules/gdscript/gdscript_compiler.h"

Array TMUserGDScript::get_error_messages() const {
	return _error_messages;
}

void TMUserGDScript::_inject_target_object_into_class_tree(GDScriptParser *p_parser) {
	GDScriptParser::VariableNode *target_object_var = p_parser->allocate_node<GDScriptParser::VariableNode>();
	target_object_var->identifier = p_parser->allocate_node<GDScriptParser::IdentifierNode>();
	target_object_var->identifier->name = SNAME("target_object");
	GDScriptParser::LiteralNode *lit = p_parser->allocate_node<GDScriptParser::LiteralNode>();
	lit->value = _target_object;
	lit->reduced_value = _target_object;
	lit->reduced = true;
	lit->is_constant = true;
	lit->datatype.kind = GDScriptParser::DataType::Kind::VARIANT;
	lit->datatype.builtin_type = Variant::OBJECT;
	target_object_var->initializer = lit;
	_class_tree->add_member<GDScriptParser::VariableNode>(target_object_var);
}

Error TMUserGDScript::load_user_gdscript(const String &p_source_code, Object *p_target_object) {
	_target_object = p_target_object;
	set_source_code(p_source_code);
	Error err = reload();
	return err;
}

void TMUserGDScript::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_error_messages"), &TMUserGDScript::get_error_messages);
	ClassDB::bind_method(D_METHOD("load_user_gdscript", "source_code", "target_object"), &TMUserGDScript::load_user_gdscript);
}

Array _get_gdscript_parser_error_messages(const GDScriptParser *p_parser) {
	Array error_messages;
	for (const GDScriptParser::ParserError &error : p_parser->get_errors()) {
		Dictionary error_message;
		error_message[String("line")] = error.line;
		error_message[String("column")] = error.column;
		error_message[String("message")] = error.message;
		error_messages.push_back(error_message);
	}
	return error_messages;
}

// This method is duplicated from GDScript::reload, but with a few changes
// to redirect the error messages to save them for later to display them,
// and to inject `target_object` into the class tree.
// Leave the comments there for comparison with the original Godot method.
Error TMUserGDScript::reload(bool p_keep_state) {
	if (reloading) {
		return OK;
	}
	reloading = true;
	_error_messages.clear();

	//	bool has_instances;
	//	{
	//		MutexLock lock(GDScriptLanguage::get_singleton()->mutex);
	//
	//		has_instances = instances.size();
	//	}
	//
	//	ERR_FAIL_COND_V(!p_keep_state && has_instances, ERR_ALREADY_IN_USE);

	//	String basedir = path;
	//
	//	if (basedir.is_empty()) {
	//		basedir = get_path();
	//	}
	//
	//	if (!basedir.is_empty()) {
	//		basedir = basedir.get_base_dir();
	//	}

	//{
	//	String source_path = path;
	//	if (source_path.is_empty()) {
	//		source_path = get_path();
	//	}
	//	Ref<GDScript> cached_script = GDScriptCache::get_cached_script(source_path);
	//	if (!source_path.is_empty() && cached_script.is_null()) {
	//		MutexLock lock(GDScriptCache::get_singleton()->mutex);
	//		GDScriptCache::get_singleton()->shallow_gdscript_cache[source_path] = Ref<GDScript>(this);
	//	}
	//}

	//bool can_run = ScriptServer::is_scripting_enabled() || is_tool();
	//#ifdef TOOLS_ENABLED
	//	if (p_keep_state && can_run && is_valid()) {
	//		_save_old_static_data();
	//	}
	//#endif

	valid = false;
	GDScriptParser parser;
	Error err = parser.parse(get_source_code(), "", false);
	if (err) {
		//if (EngineDebugger::is_active()) {
		//	GDScriptLanguage::get_singleton()->debug_break_parse(_get_debug_path(), parser.get_errors().front()->get().line, "Parser Error: " + parser.get_errors().front()->get().message);
		//}
		//_err_print_error("GDScript::reload", path.is_empty() ? "built-in" : (const char *)path.utf8().get_data(), parser.get_errors().front()->get().line, ("Parse Error: " + parser.get_errors().front()->get().message).utf8().get_data(), false, ERR_HANDLER_SCRIPT);
		_error_messages = _get_gdscript_parser_error_messages(&parser);
		reloading = false;
		return ERR_PARSE_ERROR;
	}

	// The Mirror: Inject `target_object` into the class.
	_class_tree = parser.get_tree();
	_inject_target_object_into_class_tree(&parser);

	GDScriptAnalyzer analyzer(&parser);
	err = analyzer.analyze();

	if (err) {
		//if (EngineDebugger::is_active()) {
		//	GDScriptLanguage::get_singleton()->debug_break_parse(_get_debug_path(), parser.get_errors().front()->get().line, "Parser Error: " + parser.get_errors().front()->get().message);
		//}

		//const List<GDScriptParser::ParserError>::Element *e = parser.get_errors().front();
		//while (e != nullptr) {
		//	//_err_print_error("GDScript::reload", path.is_empty() ? "built-in" : (const char *)path.utf8().get_data(), e->get().line, ("Parse Error: " + e->get().message).utf8().get_data(), false, ERR_HANDLER_SCRIPT);
		//	e = e->next();
		//}
		_error_messages = _get_gdscript_parser_error_messages(&parser);
		reloading = false;
		return ERR_PARSE_ERROR;
	}

	//can_run = ScriptServer::is_scripting_enabled() || parser.is_tool();

	GDScriptCompiler compiler;
	err = compiler.compile(&parser, this, p_keep_state);

	if (err) {
		//if (can_run) {
		//	if (EngineDebugger::is_active()) {
		//		GDScriptLanguage::get_singleton()->debug_break_parse(_get_debug_path(), compiler.get_error_line(), "Parser Error: " + compiler.get_error());
		//	}
		//	_err_print_error("GDScript::reload", path.is_empty() ? "built-in" : (const char *)path.utf8().get_data(), compiler.get_error_line(), ("Compile Error: " + compiler.get_error()).utf8().get_data(), false, ERR_HANDLER_SCRIPT);
		_error_messages = _get_gdscript_parser_error_messages(&parser);
		reloading = false;
		return ERR_COMPILATION_FAILED;
		//} else {
		//	reloading = false;
		//	return err;
		//}
	}

	//#ifdef DEBUG_ENABLED
	//	for (const GDScriptWarning &warning : parser.get_warnings()) {
	//		if (EngineDebugger::is_active()) {
	//			Vector<ScriptLanguage::StackInfo> si;
	//			EngineDebugger::get_script_debugger()->send_error("", get_script_path(), warning.start_line, warning.get_name(), warning.get_message(), false, ERR_HANDLER_WARNING, si);
	//		}
	//	}
	//#endif

	//if (can_run) {
	err = _static_init();
	if (err) {
		return err;
	}
	//}

	//#ifdef TOOLS_ENABLED
	//	if (can_run && p_keep_state) {
	//		_restore_old_static_data();
	//	}
	//#endif

	reloading = false;
	return OK;
}

TMUserGDScript::TMUserGDScript() :
		GDScript() {
	set_script_path(vformat("tmusergdscript://%d.gd", get_instance_id()));
}
