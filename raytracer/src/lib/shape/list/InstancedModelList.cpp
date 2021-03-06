#include "InstancedModelList.h"
#include "shape/bvh/BVHBuilder.h"
#include "scene/renderable/SceneNode.h"
#include "model/Model.h"
#include <utility>
#include <iostream>

InstancedModelListData::InstancedModelListData(std::vector<SceneNode<Model>>&& shapes)
	: shapes(std::move(shapes)), shapeBVHs()
{
}

InstancedModelList::InstancedModelList(ModelVector&& shapes)
	: data(std::make_shared<InstancedModelListData>(std::move(shapes))), begin(data->shapes.begin()), end(data->shapes.end())
{ }

InstancedModelList::InstancedModelList(std::shared_ptr<InstancedModelListData> data, ModelVector::iterator begin, ModelVector::iterator end)
	: data(std::move(data)), begin(std::move(begin)), end(std::move(end))
{ }

AABB InstancedModelList::getAABB(size_type index) const
{
	const auto& node = this->begin[index];
	auto aabb = node.getData().getShape().getAABB();
	return aabb.getAABBOfTransformed(node.getTransform());
}

Point InstancedModelList::getCentroid(size_type index) const
{
	const auto& node = this->begin[index];
	const Point centroid = node.getData().getShape().getCentroid();
	return node.getTransform().transform(centroid);
}

InstancedModelList::size_type InstancedModelList::count() const
{
	return std::distance(this->begin, this->end);
}

std::pair<IShapeList<SceneRayHitInfo>*, IShapeList<SceneRayHitInfo>*> InstancedModelList::splitImpl(size_type leftSideElemCount) const
{
	auto l1 = new InstancedModelList(data, this->begin, this->begin + leftSideElemCount);
	auto l2 = new InstancedModelList(data, this->begin + leftSideElemCount, this->end);

	return std::make_pair(l1, l2);
}

void InstancedModelList::sortByCentroid(Axis axis, bool allowParallelization)
{
	std::sort(this->begin, this->end, [axis](const auto& a, const auto& b)
	{
		const auto& node1 = a;
		const auto& node2 = b;
		auto p1 = node1.getTransform().transform(node1.getData().getShape().getCentroid());
		auto p2 = node2.getTransform().transform(node2.getData().getShape().getCentroid());
		return p1[static_cast<int>(axis)] < p2[static_cast<int>(axis)];
	});
}

void InstancedModelList::buildShapeBVHCache(Statistics::Collector* stats) const
{
	// Calculate shape BVHs
	for (auto& modelNode : this->data->shapes)
	{
		auto* list = dynamic_cast<IShapeList<RayHitInfo>*>(&modelNode.getData().getShape());
		if (list != nullptr)
		{
			auto shape = modelNode.getData().getShapePtr();
            auto it = this->data->shapeBVHs.find(shape);
            if(it == this->data->shapeBVHs.end())
            {
                auto bvh = BVHBuilder<RayHitInfo>::buildBVH(*list, stats);
#ifdef ENABLE_L2_BVH_PACK
                bvh.pack();
#endif
                LOGSTAT(stats, "SecondLevelBVHNodeCount", bvh.getSize());
                this->data->shapeBVHs.insert({ shape, std::move(bvh) });
            }
		}
	}
}


std::optional<std::reference_wrapper<InstancedModelListData::ShapeBVH>> InstancedModelListData::findShapeBVH(IShape& shape)
{
	// Dummy shared ptr with same internal pointer, but doesn't actually increase/decrease ref count (performance)
	std::shared_ptr<IShape> key(std::shared_ptr<IShape>(), &shape);

	auto a = this->shapeBVHs.find(key);
	if(a != this->shapeBVHs.end())
	{
		return a->second;
	}else
	{
		return std::nullopt;
	}
}

std::optional<SceneRayHitInfo> InstancedModelList::traceRay(const Ray & ray) const
{
	std::optional<RayHitInfo> closestHit;
	SceneNode<Model>* node;
	for(ModelVector::iterator it = begin; it < end; ++it)
	{
	    auto& modelNode = *it;

		const auto transformedRay = modelNode.getTransform().transformInverse(ray);
		
		auto bvh = this->data->findShapeBVH(modelNode.getData().getShape());

		std::optional<RayHitInfo> hit;
		if(bvh.has_value())
		{
			hit = bvh->get().traceRay(transformedRay);
		}else
		{
			hit = modelNode.getData().getShape().intersect(transformedRay);
		}

		if(hit.has_value() && (!closestHit.has_value() || closestHit->t > hit->t))
		{
			closestHit = hit;
			node = &modelNode;
		}
	}

	if (closestHit.has_value())
	{
		auto normal = node->getTransform().transformNormal(closestHit->normal);
		normal.normalize();
		auto tangent = node->getTransform().transform(closestHit->tangent);
		return SceneRayHitInfo(RayHitInfo(ray, closestHit->t, normal, closestHit->texCoord, tangent, closestHit->triangleIndex), *node);
	}
	else
	{
		return std::nullopt;
	}
}

void InstancedModelList::traceRays(RBSize_t startIdx, RBSize_t endIdx, RayBundle &rays, RayBundlePermutation &perm, HitBundle<SceneRayHitInfo> &result, std::array<bool, RayBundleSize>& foundBetterHit) const
{
    HitBundle<RayHitInfo> closestHits {};
    std::array<SceneNode<Model>*, RayBundleSize> nodes {};

    for(ModelVector::iterator it = begin; it < end; ++it)
    {
        auto& modelNode = *it;

        RayBundle transformedRays;
        for(RBSize_t i = startIdx; i < endIdx; ++i)
        {
            transformedRays[i] = modelNode.getTransform().transformInverse(rays[i]);
        }

        RayBundlePermutation subPerm {};
        std::iota(subPerm.begin(), subPerm.end(), 0);

        auto bvh = this->data->findShapeBVH(modelNode.getData().getShape());

        if(bvh.has_value())
        {
            std::array<bool, RayBundleSize> subFoundBetterHit {};
            bvh->get().traceRays(startIdx, endIdx, transformedRays, subPerm, closestHits, subFoundBetterHit);
            for(RBSize_t i = startIdx; i < endIdx; ++i)
            {
                if(subFoundBetterHit[i])
                {
                    nodes[i] = &modelNode;
                }
            }
        }
        else
        {
            for(RBSize_t i = startIdx; i < endIdx; ++i)
            {
                auto& closestHit = closestHits[subPerm[i]];
                std::optional<RayHitInfo> hit = modelNode.getData().getShape().intersect(transformedRays[i]);
                if(hit.has_value() && (!closestHit.has_value() || closestHit->t > hit->t))
                {
                    closestHit = hit;
                    nodes[i] = &modelNode;
                }
            }
        }
    }

    for(RBSize_t i = startIdx; i < endIdx; ++i)
    {
        auto& curClosestHit = closestHits[i];
        auto& resultClosestHit = result[perm[i]];
        auto& ray = rays[i];
        auto* node = nodes[i];
        if(curClosestHit.has_value() && (!resultClosestHit.has_value() || resultClosestHit->t > curClosestHit->t))
        {
            auto normal = node->getTransform().transformNormal(curClosestHit->normal);
            normal.normalize();
            auto tangent = node->getTransform().transform(curClosestHit->tangent);
            resultClosestHit = SceneRayHitInfo(RayHitInfo(ray, curClosestHit->t, normal, curClosestHit->texCoord, tangent, curClosestHit->triangleIndex), *node);
            foundBetterHit[perm[i]] = true;
        }
    }
}


std::optional<SceneRayHitInfo> InstancedModelList::testVisibility(const Ray &ray, float maxT) const
{
    for(ModelVector::iterator it = begin; it < end; ++it)
    {
        auto& modelNode = *it;
        const auto transformedRay = modelNode.getTransform().transformInverse(ray);

        auto bvh = this->data->findShapeBVH(modelNode.getData().getShape());

        std::optional<RayHitInfo> hit;
        if(bvh.has_value())
        {
            hit = bvh->get().testVisibility(transformedRay, maxT);
        }else
        {
            hit = modelNode.getData().getShape().testVisibility(transformedRay, maxT);
        }

        if(hit.has_value())
        {
            auto normal = modelNode.getTransform().transformNormal(hit->normal);
            normal.normalize();
            auto tangent = modelNode.getTransform().transform(hit->tangent);
            return SceneRayHitInfo(RayHitInfo(ray, hit->t, normal, hit->texCoord, tangent, hit->triangleIndex), modelNode);
        }
    }

    return std::nullopt;
}

InstancedModelList* InstancedModelList::cloneImpl() const
{
    return new InstancedModelList(*this);
}
