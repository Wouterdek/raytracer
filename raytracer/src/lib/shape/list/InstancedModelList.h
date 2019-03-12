#pragma once

#include "IShapeList.h"
#include "math/Transformation.h"
#include <memory>
#include "model/Model.h"
#include "scene/renderable/SceneRayHitInfo.h"
#include "shape/bvh/BVH.h"

struct InstancedModelListData
{
	using ShapeBVH = BVH<IShapeList<RayHitInfo>, RayHitInfo, 2>;

	std::vector<SceneNode<Model>> shapes;
	std::unordered_map<std::shared_ptr<IShape>, ShapeBVH> shapeBVHs;

	explicit InstancedModelListData(std::vector<SceneNode<Model>>&& shapes);

	std::optional<std::reference_wrapper<ShapeBVH>> InstancedModelListData::findShapeBVH(IShape& shape);
};

class InstancedModelList : public IShapeList<SceneRayHitInfo>
{
public:
	using ModelVector = std::vector<SceneNode<Model>>;

	InstancedModelList(ModelVector&& shapes);
	InstancedModelList(std::shared_ptr<InstancedModelListData> data, ModelVector::iterator begin, ModelVector::iterator end);

	Box getAABB(size_type index) const override;
	size_type count() const override;
	
	void sortByCentroid(Axis axis) override;

	void buildShapeBVHCache() const;

	std::optional<SceneRayHitInfo> traceRay(const Ray& ray) const override;

private:
	InstancedModelList* cloneImpl() override;
	std::pair<IShapeList<SceneRayHitInfo>*, IShapeList<SceneRayHitInfo>*> splitImpl(size_type leftSideElemCount) const override;

	std::shared_ptr<InstancedModelListData> data;
	ModelVector::iterator begin;
	ModelVector::iterator end;
};
