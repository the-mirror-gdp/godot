#include "register_types.h"

#include "script/tm_shader_language.h"
#include "script/tm_user_gdscript.h"
#include "script/tm_user_gdscript_highlighter.h"
#include "tm_audio_player_3d.h"
#include "util/tm_data_util.h"
#include "util/tm_file_util.h"
#include "util/tm_node_util.h"

void initialize_the_mirror_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
		GDREGISTER_CLASS(TMDataUtil);
		GDREGISTER_CLASS(TMFileUtil);
		GDREGISTER_CLASS(TMNodeUtil);
		Engine *engine = Engine::get_singleton();
		engine->add_singleton(Engine::Singleton("TMDataUtil", memnew(TMDataUtil)));
		engine->add_singleton(Engine::Singleton("TMFileUtil", memnew(TMFileUtil)));
		engine->add_singleton(Engine::Singleton("TMNodeUtil", memnew(TMNodeUtil)));
	} else if (p_level == ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_SERVERS) {
		GDREGISTER_CLASS(TMAudioPlayer3D);
		GDREGISTER_CLASS(TMShaderLanguage);
		GDREGISTER_CLASS(TMUserGDScript);
		GDREGISTER_CLASS(TMUserGDScriptSyntaxHighlighter);
	}
}

void uninitialize_the_mirror_module(ModuleInitializationLevel p_level) {
}
