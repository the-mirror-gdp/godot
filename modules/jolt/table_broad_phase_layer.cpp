#include "table_broad_phase_layer.h"
#include "core/string/string_name.h"

JLayersTable::JLayersTable() {
	// Set defaults.

	gd_layers_name.push_back("");
	gd_layers_name.push_back("NO_COLLIDE");
	gd_layers_name.push_back("STATIC");
	gd_layers_name.push_back("KINEMATIC");
	gd_layers_name.push_back("CHARACTER");
	gd_layers_name.push_back("DYNAMIC");
	gd_layers_name.push_back("TRIGGER");

	gd_layers_broad_phase.push_back(0);
	gd_layers_broad_phase.push_back(0);
	gd_layers_broad_phase.push_back(0);
	gd_layers_broad_phase.push_back(1);
	gd_layers_broad_phase.push_back(1);
	gd_layers_broad_phase.push_back(1);
	gd_layers_broad_phase.push_back(1);

	gd_layers_collide_with.push_back(LayersPair(SNAME("KINEMATIC"), SNAME("STATIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("KINEMATIC"), SNAME("KINEMATIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("KINEMATIC"), SNAME("CHARACTER")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("KINEMATIC"), SNAME("DYNAMIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("KINEMATIC"), SNAME("TRIGGER")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("CHARACTER"), SNAME("STATIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("CHARACTER"), SNAME("KINEMATIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("CHARACTER"), SNAME("CHARACTER")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("CHARACTER"), SNAME("DYNAMIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("CHARACTER"), SNAME("TRIGGER")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("DYNAMIC"), SNAME("STATIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("DYNAMIC"), SNAME("KINEMATIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("DYNAMIC"), SNAME("CHARACTER")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("DYNAMIC"), SNAME("DYNAMIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("DYNAMIC"), SNAME("TRIGGER")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("TRIGGER"), SNAME("STATIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("TRIGGER"), SNAME("KINEMATIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("TRIGGER"), SNAME("CHARACTER")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("TRIGGER"), SNAME("DYNAMIC")));
	gd_layers_collide_with.push_back(LayersPair(SNAME("TRIGGER"), SNAME("TRIGGER")));

	reload_layers();
}

void JLayersTable::reload_layers() {
	// Reset the table.
	for (int i = 0; i < LAYERS_COUNT; i++) {
		for (int y = 0; y < LAYERS_COUNT; y++) {
			layersVSlayers_collisions_table[i][y] = false;
		}

		for (int y = 0; y < BROADPHASE_COUNT; y++) {
			layersVSbroadphase_collisions_table[i][y] = false;
		}

		layers_broadphase[i] = UINT8_MAX;
	}

	layers_name.clear();

	for (uint32_t i = 0; i < gd_layers_name.size(); i++) {
		const StringName &ln = gd_layers_name[i];
		const int bp_index = MAX(gd_layers_broad_phase.size() > i ? gd_layers_broad_phase[i] : 0, 0);

		add_physics_layer(ln, bp_index);
	}

	for (uint32_t i = 0; i < gd_layers_collide_with.size(); i++) {
		collide_with(gd_layers_collide_with[i].layer_A, gd_layers_collide_with[i].layer_B);
	}
}

void JLayersTable::add_physics_layer(const StringName &p_layer_name, uint8_t p_broad_pahse_id) {
	int64_t index = layers_name.find(p_layer_name);
	if (index <= -1) {
		CRASH_COND_MSG(index >= LAYERS_COUNT, "Please increase the LAYERS_COUNT if you need more. Do it via SCsub.");
		CRASH_COND_MSG(p_broad_pahse_id >= BROADPHASE_COUNT, "Please increase the BROADPHASE_COUNT if you need more. Do it via SCsub.");

		index = layers_name.size();
		layers_name.push_back(p_layer_name);
		layers_broadphase[index] = p_broad_pahse_id;
	}
}

void JLayersTable::collide_with(const StringName &p_layer_name_1, const StringName &p_layer_name_2) {
	const int64_t index_1 = layers_name.find(p_layer_name_1);
	const int64_t index_2 = layers_name.find(p_layer_name_2);

	if (index_1 >= 0 && index_2 >= 0) {
		layersVSlayers_collisions_table[index_1][index_2] = true;
		layersVSlayers_collisions_table[index_2][index_1] = true;

		layersVSbroadphase_collisions_table[index_1][layers_broadphase[index_2]] = true;
		layersVSbroadphase_collisions_table[index_2][layers_broadphase[index_1]] = true;
	}
}

JPH::ObjectLayer JLayersTable::get_layer(const StringName &p_layer_name) const {
	const int64_t index = layers_name.find(p_layer_name);
	ERR_FAIL_COND_V_MSG(index <= -1, 0, "Jolt: The physics layer " + p_layer_name + " does not exist.");
	return index;
}

StringName JLayersTable::get_layer_name(JPH::ObjectLayer p_layer) const {
	ERR_FAIL_COND_V(p_layer >= layers_name.size(), StringName());
	return layers_name[p_layer];
}

bool JLayersTable::_set(const StringName &p_name, const Variant &p_property) {
	if (__set(p_name, p_property)) {
		reload_layers();
		return true;
	}
	return false;
}

bool JLayersTable::__set(const StringName &p_name, const Variant &p_property) {
	const String name_str = p_name;
	if (name_str == "layers_count") {
		const int old_size = gd_layers_name.size();
		gd_layers_name.resize(p_property);
		gd_layers_broad_phase.resize(p_property);

		if (old_size < int(gd_layers_name.size())) {
			for (int i = old_size; i < int(gd_layers_name.size()); i++) {
				gd_layers_name[i] = StringName();
				gd_layers_broad_phase[i] = 0;
			}
		}

		return true;
	}

	const Vector<String> split_name = name_str.split("/");
	if (split_name[0] == "layers") {
		const int layer_index = split_name[1].to_int();
		ERR_FAIL_COND_V_MSG(layer_index >= int(gd_layers_name.size()), false, "The layer index `" + itos(layer_index) + "` is not defined.");

		if (split_name[2] == "name") {
			gd_layers_name[layer_index] = p_property;
			return true;

		} else if (split_name[2] == "broad_phase") {
			gd_layers_broad_phase[layer_index] = p_property;
			return true;

		} else if (split_name[2] == "collides_with") {
			const String other_layer_name = split_name[3];
			LayersPair lp(gd_layers_name[layer_index], other_layer_name);

			if (p_property.operator bool()) {
				// Should collide.
				if (gd_layers_collide_with.find(lp) == -1) {
					gd_layers_collide_with.push_back(lp);
				}
			} else {
				// Should NOT collide.
				gd_layers_collide_with.erase(lp);
				CRASH_COND(gd_layers_collide_with.find(lp) != -1);
			}
			return true;
		}
	}

	return false;
}

bool JLayersTable::_get(const StringName &p_name, Variant &r_property) const {
	const String name_str = p_name;
	if (name_str == "layers_count") {
		r_property = gd_layers_name.size();
		return true;
	}

	const Vector<String> sname = name_str.split("/");
	if (sname[0] == "layers") {
		const int layer_index = sname[1].to_int();
		ERR_FAIL_COND_V_MSG(layer_index >= int(gd_layers_name.size()), false, "The layer index `" + itos(layer_index) + "` is not defined.");

		if (sname[2] == "name") {
			r_property = gd_layers_name[layer_index];
			return true;

		} else if (sname[2] == "broad_phase") {
			r_property = gd_layers_broad_phase[layer_index];
			return true;

		} else if (sname[2] == "collides_with") {
			const String other_layer_name = sname[3];
			LayersPair lp(gd_layers_name[layer_index], other_layer_name);

			const bool should_collide = gd_layers_collide_with.find(lp) != -1;
			r_property = should_collide;
			return true;
		}
	}

	return false;
}

void JLayersTable::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::INT, "layers_count"));
	for (uint32_t i = 0; i < gd_layers_name.size(); i++) {
		p_list->push_back(PropertyInfo(Variant::STRING_NAME, "layers/" + itos(i) + "/name"));
		p_list->push_back(PropertyInfo(Variant::INT, "layers/" + itos(i) + "/broad_phase"));
		for (uint32_t u = 0; u < gd_layers_name.size(); u++) {
			p_list->push_back(PropertyInfo(Variant::BOOL, "layers/" + itos(i) + "/collides_with/" + gd_layers_name[u]));
		}
	}
}
