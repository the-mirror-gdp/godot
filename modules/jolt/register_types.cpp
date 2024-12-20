#include "register_types.h"

#include "Jolt/Core/Core.h"

#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"

#include "modules/jolt/j_shape.h"
#include "modules/jolt/table_broad_phase_layer.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/RegisterTypes.h"

#include "Jolt/Core/Factory.h"
#include "Jolt/Core/Memory.h"
#include "Jolt/RegisterTypes.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "j_body_3d.h"
#include "j_custom_double_sided_shape.h"
#include "jolt.h"
#include "jolt_debug_geometry_3d.h"
#include "mimalloc.h"
#include "modules/jolt/j_utils.h"

void jolt_trace(const char *p_format, ...) {
}

bool jolt_assert(const char *p_expr, const char *p_msg, const char *p_file, uint32_t p_line) {
	// Taken from Godot-Jolt.
	CRASH_NOW_MSG(vformat(
			"Assertion '%s' failed with message '%s' at '%s:%d'",
			p_expr,
			p_msg != nullptr ? p_msg : "",
			p_file,
			p_line));

	return false;
}

void initialize_jolt_module(ModuleInitializationLevel p_level) {
	if (p_level == ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_CORE) {
		// This function double check that the SIMD macros with CMake are the same one used by Scons.
		CRASH_COND(!JPH::VerifyJoltVersionID());

		unit_test_converters();

		// Init the Jolt singleton so that we can setup the layers ASAP.
		memnew(Jolt);

		GLOBAL_DEF_BASIC(PropertyInfo(Variant::STRING, "jolt/config/layers_table"), String());
		GLOBAL_DEF_BASIC("jolt/main_world_config/max_bodies", 10'000);
		GLOBAL_DEF_BASIC("jolt/main_world_config/num_body_mutex", 0);
		GLOBAL_DEF_BASIC("jolt/main_world_config/max_body_pairs", 10'000);
		GLOBAL_DEF_BASIC("jolt/main_world_config/max_contact_constraints", 10'000);

	} else if (p_level == ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_SERVERS) {
		// SETUP Jolt
		// Setup custom allocator, using mimalloc
		JPH::Allocate = &mi_malloc;
		JPH::Free = &mi_free;
		JPH::AlignedAllocate = &mi_malloc_aligned;
		JPH::AlignedFree = &mi_free;

#ifdef JPH_ENABLE_ASSERTS
		JPH::Trace = &jolt_trace;
		JPH::AssertFailed = jolt_assert;
#endif // JPH_ENABLE_ASSERTS

		// Create the Jolt factory used to handle RTTI
		JPH::Factory::sInstance = new JPH::Factory;

		// Register all Jolt physics types
		JPH::RegisterTypes();
		JoltCustomDoubleSidedShape::register_type();

		// SETUP GODOT-Jolt
		ClassDB::register_class<Jolt>();
		ClassDB::register_class<JLayersTable>();
		ClassDB::register_class<JBody3D>();
		ClassDB::register_abstract_class<JShape3D>();
		ClassDB::register_class<JBoxShape3D>();
		ClassDB::register_class<JSphereShape3D>();
		ClassDB::register_class<JCapsuleShape3D>();
		ClassDB::register_class<JCylinderShape3D>();
		ClassDB::register_class<JHeightFieldShape3D>();
		ClassDB::register_class<JMeshShape3D>();
		ClassDB::register_class<JConvexHullShape3D>();
		ClassDB::register_class<JCompoundShape3D>();

		Jolt::singleton()->__init();

		Engine::get_singleton()->add_singleton(
				Engine::Singleton("Jolt", Jolt::singleton()));

	} else if (p_level == ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_SCENE) {
		ClassDB::register_class<JoltDebugGeometry3D>();
		// Load the JLayersTable.
		const String layers_table_res_path = ProjectSettings::get_singleton()->get_setting("jolt/config/layers_table", String());
		if (layers_table_res_path != String()) {
			print_line("Loading layers_table from path: `" + layers_table_res_path + "`");
			Ref<JLayersTable> res = ResourceLoader::load(layers_table_res_path);
			if (!res.is_null()) {
				Jolt::singleton()->set_layers_table(res);
			}
		}
	}
}

void uninitialize_jolt_module(ModuleInitializationLevel p_level) {
	if (p_level == ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_SERVERS) {
		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	} else if (p_level == ModuleInitializationLevel::MODULE_INITIALIZATION_LEVEL_CORE) {
		memdelete(Jolt::singleton());
	}
}
