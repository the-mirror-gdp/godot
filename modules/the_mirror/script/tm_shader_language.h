
#ifndef TM_SHADER_LANGUAGE_H
#define TM_SHADER_LANGUAGE_H

#include "core/object/class_db.h"
#include "core/object/object.h"

class TMShaderLanguage : public Object {
	GDCLASS(TMShaderLanguage, Object);

public:
	TMShaderLanguage();
	~TMShaderLanguage() = default;

	static void _bind_methods();

	String compile_spatial(const String &p_code) const;
};

#endif // TM_SHADER_LANGUAGE_H
