#pragma once

#include <vector>
#include "math/Vector3.h"
#include "math/Axis.h"
#include "shape/Box.h"
#include "utility/ICloneable.h"

template<typename TRayHitInfo>
class IShapeList : public ICloneable<IShapeList<TRayHitInfo>>
{
public:
	using size_type = std::vector<char>::size_type;

private:
	virtual std::pair<IShapeList<TRayHitInfo>*, IShapeList<TRayHitInfo>*> splitImpl(size_type leftSideElemCount) const = 0;

public:
	virtual ~IShapeList() = default;

	virtual AABB getAABB(size_type index) const = 0;
	virtual Point getCentroid(size_type index) const = 0;
	virtual size_type count() const = 0;

	std::pair<std::unique_ptr<IShapeList<TRayHitInfo>>, std::unique_ptr<IShapeList<TRayHitInfo>>> split(size_type leftSideElemCount) const
	{
		auto[leftPtr, rightPtr] = this->splitImpl(leftSideElemCount);
		return std::make_pair(std::unique_ptr<IShapeList<TRayHitInfo>>(leftPtr), std::unique_ptr<IShapeList<TRayHitInfo>>(rightPtr));
	}

	virtual void sortByCentroid(Axis axis, bool allowParallelization) = 0;
	
	virtual std::optional<TRayHitInfo> traceRay(const Ray& ray) const = 0;
    virtual std::optional<TRayHitInfo> testVisibility(const Ray& ray, float maxT) const
    {
        auto hit = traceRay(ray);
        if(hit.has_value() && hit->t <= maxT)
        {
            return hit;
        }
        return std::nullopt;
    }
};
