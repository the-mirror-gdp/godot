
#ifndef OBJECT_LAYERS_FILTER_H
#define OBJECT_LAYERS_FILTER_H

#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Jolt.h"

#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "core/templates/local_vector.h"

class ObjectLayersFilter : public JPH::ObjectLayerFilter {
public:
	LocalVector<JPH::ObjectLayer> layers;

	virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const {
		if (layers.size() == 0) {
			return true;
		}
		return layers.find(inLayer) >= 0;
	}
};

#endif // OBJECT_LAYERS_FILTER_H
