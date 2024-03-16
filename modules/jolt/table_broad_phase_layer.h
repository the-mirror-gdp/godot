
#ifndef TABLE_BROAD_PHASE_LAYER_H
#define TABLE_BROAD_PHASE_LAYER_H

#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"

#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "core/io/resource.h"
#include "core/templates/local_vector.h"

#define LAYERS_COUNT 32
#define BROADPHASE_COUNT 4

// NOTE: LAYERS_COUNT is set on the `SCsub`, you want to change it there.
//       I made this like that, to avoid using the C++ template.

struct LayersPair {
	StringName layer_A;
	StringName layer_B;

	LayersPair() = default;
	LayersPair(
			StringName p_layer_A,
			StringName p_layer_B) :
			layer_A(p_layer_A),
			layer_B(p_layer_B) {}

	bool operator==(const LayersPair &p_other) {
		return (layer_A == p_other.layer_A && layer_B == p_other.layer_B) ||
				(layer_A == p_other.layer_B && layer_B == p_other.layer_A);
	}
};

class JLayersTable : public Resource {
	GDCLASS(JLayersTable, Resource);

	friend class Jolt;
	friend class TableObjectLayerPairFilter;
	friend class TableObjectVsBroadPhaseLayerFilter;
	friend class TableBroadPhaseLayer;
	friend class TableObjectLayerFilter;

	LocalVector<StringName> gd_layers_name;
	LocalVector<int> gd_layers_broad_phase;
	LocalVector<LayersPair> gd_layers_collide_with;

	bool layersVSlayers_collisions_table[LAYERS_COUNT][LAYERS_COUNT];
	bool layersVSbroadphase_collisions_table[LAYERS_COUNT][BROADPHASE_COUNT];
	uint8_t layers_broadphase[LAYERS_COUNT];
	LocalVector<StringName> layers_name;

public:
	JLayersTable();

	void reload_layers();

	void add_physics_layer(const StringName &p_layer_name, uint8_t p_broad_pahse_id);
	void collide_with(const StringName &p_layer_name_1, const StringName &p_layer_name_2);
	JPH::ObjectLayer get_layer(const StringName &p_layer_name) const;
	StringName get_layer_name(JPH::ObjectLayer p_layer) const;

	bool _set(const StringName &p_name, const Variant &p_property);
	bool __set(const StringName &p_name, const Variant &p_property);
	bool _get(const StringName &p_name, Variant &r_property) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
};

/// Class that determines if two object layers can collide
class TableObjectLayerPairFilter : public JPH::ObjectLayerPairFilter {
public:
	const JLayersTable *table = nullptr;

	TableObjectLayerPairFilter() {}

	virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
		return table->layersVSlayers_collisions_table[inObject1][inObject2];
	}
};

// Used to test ObjectLayer vs BroadPhaseLayer.
class TableObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
	const JLayersTable *table = nullptr;

	TableObjectVsBroadPhaseLayerFilter() {}

	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inBLayer2) const override {
		return table->layersVSbroadphase_collisions_table[inLayer1][uint8_t(inBLayer2)];
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
class TableBroadPhaseLayer final : public JPH::BroadPhaseLayerInterface {
public:
	JLayersTable *table = nullptr;

	TableBroadPhaseLayer() {}

	virtual JPH::uint GetNumBroadPhaseLayers() const override {
		int m = -1;
		for (int i = 0; i < LAYERS_COUNT; i++) {
			if (table->layers_broadphase[i] != UINT8_MAX) {
				m = MAX(int(table->layers_broadphase[i]), m);
			}
		}
		return m + 1;
	}

	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
		return JPH::BroadPhaseLayer(table->layers_broadphase[inLayer]);
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
		String s;
		for (int i = 0; i < LAYERS_COUNT; i++) {
			if (JPH::BroadPhaseLayer(table->layers_broadphase[i]) == inLayer) {
				s = s + table->layers_name[i] + "-";
			}
		}
		return s.utf8().ptr();
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
};

class TableObjectLayerFilter : public JPH::ObjectLayerFilter {
public:
	JPH::ObjectLayer layer;
	const JLayersTable *table = nullptr;

	virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const {
		return table->layersVSlayers_collisions_table[layer][inLayer];
	}
};

#endif // TABLE_BROAD_PHASE_LAYER_H
