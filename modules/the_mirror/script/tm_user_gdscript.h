
#ifndef TM_USER_GDSCRIPT_H
#define TM_USER_GDSCRIPT_H

#include "modules/gdscript/gdscript.h"
#include "modules/gdscript/gdscript_parser.h"

class TMUserGDScript : public GDScript {
	GDCLASS(TMUserGDScript, GDScript);

	Array _error_messages;
	Object *_target_object = nullptr;
	GDScriptParser::ClassNode *_class_tree = nullptr;
	void _inject_target_object_into_class_tree(GDScriptParser *p_parser);

protected:
	static void _bind_methods();

public:
	Array get_error_messages() const;
	Error load_user_gdscript(const String &p_source_code, Object *p_target_object);
	virtual Error reload(bool p_keep_state = false) override;

	TMUserGDScript();
};

#endif // TM_USER_GDSCRIPT_H
