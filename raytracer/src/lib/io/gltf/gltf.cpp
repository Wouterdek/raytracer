#include "gltf.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"
#include "shape/TriangleMesh.h"
#include "material/DiffuseMaterial.h"
#include "camera/PerspectiveCamera.h"
#include <Eigen/Dense>

template <typename T>
void loadIndicesImpl(tinygltf::Model& file, tinygltf::Accessor& accessor, std::vector<std::array<uint32_t, 3>>& indices)
{
	auto& bufferView = file.bufferViews[accessor.bufferView];
	const auto start = bufferView.byteOffset + accessor.byteOffset;
	const auto end = bufferView.byteLength + bufferView.byteOffset;
	auto stride = bufferView.byteStride;
	if (stride == 0)
	{
		stride = sizeof(T);
	}

	auto& buffer = file.buffers[bufferView.buffer];
	std::array<uint32_t, 3> curTriangle;
	auto j = 0;
	for (auto i = start; i < end; i += stride)
	{
		T val;
		memcpy(&val, &buffer.data[i], sizeof(val));
		curTriangle[j] = val;
		j = (j + 1) % 3;
		if (j == 0)
		{
			indices.emplace_back(curTriangle);
		}
	}
}

void loadIndices(tinygltf::Model& file, tinygltf::Accessor& accessor, std::vector<std::array<uint32_t, 3>>& indices)
{
	if (accessor.type != TINYGLTF_TYPE_SCALAR)
	{
		throw std::runtime_error("Unsupported index accessor");
	}

	switch (accessor.componentType)
	{
	case TINYGLTF_COMPONENT_TYPE_INT:
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		loadIndicesImpl<int>(file, accessor, indices);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		loadIndicesImpl<short>(file, accessor, indices);
		break;
	case TINYGLTF_COMPONENT_TYPE_BYTE:
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		loadIndicesImpl<char>(file, accessor, indices);
		break;
	default:
		throw std::runtime_error("Unsupported index component type");
	}
}

template<typename VectorType>
void loadVec3s(tinygltf::Model& file, tinygltf::Accessor& accessor, std::vector<VectorType>& vectors)
{
	if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.type != TINYGLTF_TYPE_VEC3)
	{
		throw std::runtime_error("Unsupported vector accessor");
	}

	auto& bufferView = file.bufferViews[accessor.bufferView];
	const auto start = bufferView.byteOffset + accessor.byteOffset;
	const auto end = bufferView.byteLength + bufferView.byteOffset;
	auto stride = bufferView.byteStride;
	if (stride == 0)
	{
		stride = sizeof(float[3]);
	}

	auto& buffer = file.buffers[bufferView.buffer];
	for (auto i = start; i < end; i += stride)
	{
		float x, y, z;
		memcpy(&x, &buffer.data[i], sizeof(float));
		memcpy(&y, &buffer.data[i + sizeof(float)], sizeof(float));
		memcpy(&z, &buffer.data[i + 2 * sizeof(float)], sizeof(float));
		vectors.emplace_back(x, y, z);
	}
}

void loadVec2s(tinygltf::Model & file, tinygltf::Accessor & accessor, std::vector<Vector2> & vectors)
{
	if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.type != TINYGLTF_TYPE_VEC2)
	{
		throw std::runtime_error("Unsupported vector accessor");
	}

	auto& bufferView = file.bufferViews[accessor.bufferView];
	const auto start = bufferView.byteOffset + accessor.byteOffset;
	const auto end = bufferView.byteLength + bufferView.byteOffset;
	auto stride = bufferView.byteStride;
	if (stride == 0)
	{
		stride = sizeof(float[2]);
	}

	auto& buffer = file.buffers[bufferView.buffer];
	for (auto i = start; i < end; i += stride)
	{
		float x, y;
		memcpy(&x, &buffer.data[i], sizeof(float));
		memcpy(&y, &buffer.data[i + sizeof(float)], sizeof(float));
		vectors.emplace_back(x, y);
	}
}

std::shared_ptr<TriangleMesh> loadPrimitiveShape(tinygltf::Model & file, tinygltf::Primitive & primitive)
{
	if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
	{
		throw std::runtime_error("Unsupported primitive type");
	}

	std::vector<Point> vertices;
	std::vector<std::array<uint32_t, 3>> indices;
	std::vector<Vector3> normals;
	std::vector<Vector2> texCoords;

	if (primitive.indices != -1)
	{
		auto& indicesAcc = file.accessors[primitive.indices];
		loadIndices(file, indicesAcc, indices);
	}

	//auto& accessor = file.accessors[primitive.indices];
	std::map<std::string, int>::iterator vertPosEntry;
	if ((vertPosEntry = primitive.attributes.find("POSITION")) != primitive.attributes.end())
	{
		auto& vertPos = file.accessors[vertPosEntry->second];
		loadVec3s(file, vertPos, vertices);
	}

	std::map<std::string, int>::iterator vertNormEntry;
	if ((vertNormEntry = primitive.attributes.find("NORMAL")) != primitive.attributes.end())
	{
		auto& vertNorm = file.accessors[vertNormEntry->second];
		loadVec3s(file, vertNorm, normals);
	}
	else
	{
		throw std::runtime_error("Implicit normals are not supported");
	}

	std::map<std::string, int>::iterator vertTexCoordEntry;
	if ((vertTexCoordEntry = primitive.attributes.find("TEXCOORD_0")) != primitive.attributes.end())
	{
		auto& vertTexCoord = file.accessors[vertTexCoordEntry->second];
		loadVec2s(file, vertTexCoord, texCoords);
	}

	return std::make_shared<TriangleMesh>(vertices, indices, normals, indices, texCoords, indices);
}

std::shared_ptr<IMaterial> loadMaterial(tinygltf::Model & file, tinygltf::Material & mat)
{
	auto result = std::make_shared<DiffuseMaterial>();
	result->diffuseColor = { 1.0, 1.0, 1.0 };
	result->ambientColor = { 1.0, 1.0, 1.0 };
	result->ambientIntensity = 0;
	result->diffuseIntensity = 0.8;
	return result;
}

std::unique_ptr<Model> loadPrimitive(tinygltf::Model & file, tinygltf::Primitive & primitive)
{
	std::shared_ptr<IMaterial> mat;
	if (primitive.material != -1)
	{
		mat = loadMaterial(file, file.materials[primitive.material]);
	}
	else
	{
		auto diff = std::make_shared<DiffuseMaterial>();
		diff->diffuseColor = { 1.0, 1.0, 1.0 };
		diff->ambientColor = { 1.0, 1.0, 1.0 };
		diff->ambientIntensity = 0;
		diff->diffuseIntensity = 0.8;
		mat = diff;
	}
	return std::make_unique<Model>(loadPrimitiveShape(file, primitive), mat);
}

std::unique_ptr<DynamicSceneNode> loadNode(tinygltf::Model & file, int nodeI)
{
	auto& node = file.nodes[nodeI];
	auto result = std::make_unique<DynamicSceneNode>();

	auto transform = Transformation::IDENTITY;
	if (!node.matrix.empty())
	{
		Matrix transMat(node.matrix.data());
		Matrix invMat = transMat.inverse();
		transform = Transformation(transMat, invMat);
	}
	else
	{
		if (!node.translation.empty()) {
			transform = transform.append(Transformation::translate(node.translation[0], node.translation[1], node.translation[2]));
		}
		if (!node.rotation.empty()) {
			transform = transform.append(Transformation::rotateQuaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]));
		}
		if (!node.scale.empty()) {
			transform = transform.append(Transformation::scale(node.scale[0], node.scale[1], node.scale[2]));
		}
	}
	result->transform = transform;

	if (node.name.find("$AREALIGHT") == 0)
	{
		result->areaLight = std::make_unique<AreaLight>();

		if (node.mesh == -1)
		{
			throw std::runtime_error("Invalid area light definition");
		}

		auto& meshDef = file.meshes[node.mesh];
		if (meshDef.primitives.size() != 1)
		{
			throw std::runtime_error("Invalid area light definition");
		}

		auto prim = loadPrimitiveShape(file, meshDef.primitives[0]);
		auto primData = prim->getData();
		if (primData.vertices.size() != 3 || primData.vertexIndices.size() != 1)
		{
			throw std::runtime_error("Invalid area light definition: must be triangular");
		}

		result->areaLight->a = primData.vertices[primData.vertexIndices[0][0]];
		result->areaLight->b = primData.vertices[primData.vertexIndices[0][1]];
		result->areaLight->c = primData.vertices[primData.vertexIndices[0][2]];
	}
	else if (node.mesh != -1)
	{
		auto& meshDef = file.meshes[node.mesh];

		if (meshDef.primitives.size() == 1)
		{
			result->model = loadPrimitive(file, meshDef.primitives[0]);
		}
		else
		{
			for (auto& primitive : meshDef.primitives)
			{
				auto child = std::make_unique<DynamicSceneNode>();
				child->transform = Transformation::IDENTITY;
				child->model = loadPrimitive(file, primitive);
				result->children.push_back(std::move(child));
			}
		}
	}
	else if (node.camera != -1)
	{
		auto& cameraDef = file.cameras[node.camera];
		if (cameraDef.type == "perspective")
		{
			double fov = 60; //TODO: fix camera
			result->camera = std::make_unique<PerspectiveCamera>(fov);
		}
	}

	for (auto subNodeI : node.children)
	{
		result->children.push_back(loadNode(file, subNodeI));
	}

	return result;
}

DynamicScene loadGLTFScene(std::string file)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, file);

	if (!warn.empty()) {
		printf("GLTF warning: %s\n", warn.c_str());
	}

	if (!err.empty()) {
		printf("GLTF error: %s\n", err.c_str());
	}

	if (!ret) {
		throw std::runtime_error("Failed to load gltf");
	}

	DynamicScene scene;
	scene.root = std::make_unique<DynamicSceneNode>();
	for (auto nodeI : model.scenes[model.defaultScene].nodes)
	{
		scene.root->children.push_back(loadNode(model, nodeI));
	}
	return scene;
}