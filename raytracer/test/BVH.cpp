#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "shape/Sphere.h"
#include "shape/bvh/BVH.h"
#include "shape/bvh/BVHBuilder.h"
#include "shape/list/InstancedModelList.h"
#include "shape/list/InstancedModelList.h"
#include "material/NormalMaterial.h"

using namespace testing;

auto make_sphere()
{
	return std::make_unique<Model>(std::make_shared<Sphere>(), std::make_shared<NormalMaterial>());
}

TEST(BVH, TwoSpheres)
{
	std::vector<SceneNode<Model>> models;
	auto sphereModel1 = make_sphere();
	auto sphere1 = sphereModel1->getShapePtr();
	models.emplace_back(Transformation::translate(-2, 0, 0), std::move(sphereModel1));
	auto sphereModel2 = make_sphere();
	auto sphere2 = sphereModel2->getShapePtr();
	models.emplace_back(Transformation::translate(2, 0, 0), std::move(sphereModel2));
	InstancedModelList list(std::move(models));
	auto bvh = BVHBuilder<SceneRayHitInfo>::buildBVH(list);

	{
		auto dir = Vector3(-2, 0, 0);
		dir.normalize();
		Ray ray(Point(0, 0, 0), dir);

		auto hit = bvh.traceRay(ray);
		ASSERT_TRUE(hit.has_value());
		ASSERT_EQ(&hit->getModelNode().getData().getShape(), &*sphere1);
	}

	{
		auto dir = Vector3(2, 0, 0);
		dir.normalize();
		Ray ray(Point(0, 0, 0), dir);

		auto hit = bvh.traceRay(ray);
		ASSERT_TRUE(hit.has_value());
		ASSERT_EQ(&hit->getModelNode().getData().getShape(), &*sphere2);
	}
}

TEST(BVH, TriangleMesh)
{
	std::vector<SceneNode<Model>> models;
	auto sphereModel1 = make_sphere();
	auto sphere1 = sphereModel1->getShapePtr();
	models.emplace_back(Transformation::translate(-2, 0, 0), std::move(sphereModel1));
	auto sphereModel2 = make_sphere();
	auto sphere2 = sphereModel2->getShapePtr();
	models.emplace_back(Transformation::translate(2, 0, 0), std::move(sphereModel2));
	InstancedModelList list(std::move(models));
	auto bvh = BVHBuilder<SceneRayHitInfo>::buildBVH(list);

	{
		auto dir = Vector3(-2, 0, 0);
		dir.normalize();
		Ray ray(Point(0, 0, 0), dir);

		auto hit = bvh.traceRay(ray);
		ASSERT_TRUE(hit.has_value());
		ASSERT_EQ(&hit->getModelNode().getData().getShape(), &*sphere1);
	}

	{
		auto dir = Vector3(2, 0, 0);
		dir.normalize();
		Ray ray(Point(0, 0, 0), dir);

		auto hit = bvh.traceRay(ray);
		ASSERT_TRUE(hit.has_value());
		ASSERT_EQ(&hit->getModelNode().getData().getShape(), &*sphere2);
	}
}