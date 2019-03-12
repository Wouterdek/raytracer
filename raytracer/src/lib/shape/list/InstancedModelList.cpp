#include "InstancedModelList.h"
#include <utility>
#include "shape/bvh/BVHBuilder.h"

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

Box InstancedModelList::getAABB(size_type index) const
{
	const auto& node = this->begin[index];
	auto aabb = node.getData().getShape().getAABB();
	return aabb.getAABBOfTransformed(node.getTransform());
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

void InstancedModelList::sortByCentroid(Axis axis)
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

void InstancedModelList::buildShapeBVHCache() const
{
	// Calculate shape BVHs
	for (auto& modelNode : this->data->shapes)
	{
		auto* list = dynamic_cast<IShapeList<RayHitInfo>*>(&modelNode.getData().getShape());
		if (list != nullptr)
		{
			auto shape = modelNode.getData().getShapePtr();
			auto bvh = BVHBuilder<RayHitInfo>::buildBVH(*list);
			this->data->shapeBVHs.insert({ shape, std::move(bvh) });
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
	for(auto& modelNode : this->data->shapes)
	{
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
		return SceneRayHitInfo(*closestHit, *node);
	}
	else
	{
		return std::nullopt;
	}
}

InstancedModelList* InstancedModelList::cloneImpl()
{
	return new InstancedModelList(*this);
}