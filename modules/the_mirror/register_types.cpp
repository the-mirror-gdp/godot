#include "register_types.h"

#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"

#include "physics/collision_fx_manager.h"
#include "physics/tm_character_3d.h"
#include "physics/tm_scene_sync.h"
#include "physics/tm_space_object_base.h"
#include "script/tm_shader_language.h"
#include "script/tm_user_gdscript.h"
#include "script/tm_user_gdscript_highlighter.h"
#include "tm_audio_player_3d.h"
#include "util/tm_data_util.h"
#include "util/tm_file_util.h"
#include "util/tm_node_util.h"

void initialize_the_mirror_module(ModuleInitializationLevel p_level) {
	// This function double check that the SIMD macros with CMake are the same one used by Scons.
	CRASH_COND(!JPH::VerifyJoltVersionID());

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
		GDREGISTER_CLASS(TMCharacter3D);
		GDREGISTER_CLASS(TMSceneSync);
		GDREGISTER_CLASS(TMShaderLanguage);
		GDREGISTER_CLASS(TMSpaceObjectBase);
		GDREGISTER_CLASS(TMUserGDScript);
		GDREGISTER_CLASS(TMUserGDScriptSyntaxHighlighter);
		GDREGISTER_CLASS(CollisionFxManager);

		memnew(TMSceneSync);
		memnew(CollisionFxManager);

		Engine::get_singleton()->add_singleton(
				Engine::Singleton("TMSceneSync", TMSceneSync::singleton()));

		Engine::get_singleton()->add_singleton(
				Engine::Singleton("CollisionFxManager", CollisionFxManager::get_singleton()));

		TMSceneSync::singleton()->add_child(CollisionFxManager::get_singleton());
	}
}

void uninitialize_the_mirror_module(ModuleInitializationLevel p_level) {
	if (p_level == ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_SERVERS) {
		TMSceneSync::singleton()->remove_child(CollisionFxManager::get_singleton());
		memdelete(CollisionFxManager::get_singleton());
		memdelete(TMSceneSync::singleton());
	}
}
