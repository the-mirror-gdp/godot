
#ifndef J_QUERY_RESULTS_H
#define J_QUERY_RESULTS_H

#include "Jolt/Jolt.h"

#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseQuery.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"
#include "core/typedefs.h"
#include "modules/jolt/jolt.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Math/Vec3.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Body/Body.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Collision/CastResult.h"
#include "modules/jolt/thirdparty/JoltPhysics/Jolt/Physics/Collision/RayCast.h"
#include <vector>

#define CMP_EPSILON 0.00001

class JRayQuerySingleResult : public JPH::CastRayCollector {
public:
	using HitType = typename JPH::CastRayCollector::ResultType;

	JPH::RRayCast ray = JPH::RRayCast();
	HitType hit;
	JPH::Vec3 normal;
	JPH::RVec3 location;

	explicit JRayQuerySingleResult() :
			JPH::CastRayCollector() {}

public: // ----------------------------------------------------------- INTERFACE
	bool has_hit() const {
		return hit.mFraction <= 1.0;
	}

	virtual void Reset() override {
		JPH::CastRayCollector::Reset();
		hit = HitType();
	}

	virtual void AddHit(const HitType &p_hit) override {
		if (p_hit.mFraction >= hit.mFraction) {
			// Skip this, not depth enough to be considered an hit.
			return;
		}

		hit = p_hit;
		location = ray.GetPointOnRay(hit.mFraction);

		JPH::Body *body = Jolt::singleton()->body_jolt_fetch(hit.mBodyID);
		if (body) {
			normal = body->GetWorldSpaceSurfaceNormal(
					hit.mSubShapeID2,
					location);
		}
	}
};

template <typename BaseType>
class JQueryVectorResult : public BaseType {
public:
	using HitType = typename BaseType::ResultType;

	std::vector<HitType> hits;
	uint32_t max_hits;
	float min_penetration_depth = -CMP_EPSILON;

	explicit JQueryVectorResult(int32_t p_reserve = 10, uint32_t p_max_hits = 0) :
			max_hits(p_max_hits) {
		hits.reserve(p_reserve);
	}

	bool has_hits() const { return hits.size() > 0; }

	int32_t hits_count() const { return hits.size(); }

	const HitType &get_hit(int32_t p_index) const { return hits[p_index]; }

public: // ----------------------------------------------------------- INTERFACE
	virtual void Reset() override {
		BaseType::Reset();
		hits.clear();
	}

	virtual void AddHit(const HitType &p_hit) override {
		if (p_hit.mPenetrationDepth <= min_penetration_depth) {
			// Skip this, not depth enough to be considered an hit.
			return;
		}

		if (max_hits > 0) {
			if (hits.size() < max_hits) {
				hits.push_back(p_hit);
			} else {
				BaseType::ForceEarlyOut();
			}
		} else {
			hits.push_back(p_hit);
		}
	}
};

class BroadphaseCollector : public JPH::CollideShapeBodyCollector {
public:
	std::vector<JPH::BodyID> &overlapping_bodies;

	BroadphaseCollector(std::vector<JPH::BodyID> &r_overlapping_bodies) :
			overlapping_bodies(r_overlapping_bodies) {}

	virtual void AddHit(const JPH::BodyID &inBodyID) override {
		overlapping_bodies.push_back(inBodyID);
	}
};

#endif // J_QUERY_RESULTS_H
