#include "GLTF.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "io/lib/tiny_gltf.h"
#include "io/JPEG.h"
#include "shape/TriangleMesh.h"
#include "material/DiffuseMaterial.h"
#include "camera/PerspectiveCamera.h"
#include <Eigen/Dense>
#include <material/MixMaterial.h>
#include <material/FresnelMixMaterial.h>
#include <math/Constants.h>
#include <material/TexCoordMaterial.h>
#include <material/GlassMaterial.h>
#include <material/EmissiveMaterial.h>
#include <material/AddMaterial.h>
#include <material/environment/ColorEnvironment.h>
#include <material/TransparentMaterial.h>
#include "material/GlossyMaterial.h"
#include "material/NormalMaterial.h"
#include "material/PositionMaterial.h"
#include "material/Texture.h"

namespace {

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
			indices.push_back(curTriangle);
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
		loadIndicesImpl<unsigned int>(file, accessor, indices);
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
	case TINYGLTF_COMPONENT_TYPE_SHORT:
		loadIndicesImpl<unsigned short>(file, accessor, indices);
		break;
	case TINYGLTF_COMPONENT_TYPE_BYTE:
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
		loadIndicesImpl<unsigned char>(file, accessor, indices);
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
		float coord[3];
		memcpy(coord, &buffer.data[i], sizeof(coord));
		vectors.emplace_back(coord[0], coord[1], coord[2]);
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

const tinygltf::Value* tryGetExtras(const tinygltf::Node* node)
{
    return node == nullptr ? nullptr : &node->extras;
}

bool getBoolOrDefault(const tinygltf::Value* extras, const std::string &paramName, bool defaultVal = false)
{
    if(extras != nullptr && extras->Has(paramName)){
        const auto& val = extras->Get(paramName);
        if(val.IsBool()){
            return val.Get<bool>();
        }else if(val.IsInt()){
            return val.Get<int>() != 0;
        }else if(val.IsNumber()){
            return val.Get<double>() > 0;
        }
    }

    return defaultVal;
}

double asDoubleOrDefault(const tinygltf::Value& val, double defaultVal)
{
    if(val.IsBool()){
        return val.Get<bool>() ? 1.0 : 0.0;
    }else if(val.IsInt()){
        return static_cast<double>(val.Get<int>());
    }else if(val.IsNumber()){
        return val.Get<double>();
    }else{
        return defaultVal;
    }
}

double getDoubleOrDefault(const tinygltf::Value* extras, const std::string &paramName, double defaultVal = 0.0)
{
    if(extras != nullptr && extras->Has(paramName)){
        const auto& val = extras->Get(paramName);
        return asDoubleOrDefault(val, defaultVal);
    }

    return defaultVal;
}

RGB getColorOrDefault(const tinygltf::Value* extras, const std::string &paramName, RGB defaultVal)
{
    if(extras != nullptr && extras->Has(paramName)){
        const auto& val = extras->Get(paramName);
        if(val.IsArray()){
            auto arr = val.Get<tinygltf::Value::Array>();
            if(arr.size() < 3)
            {
                return defaultVal;
            }

            return RGB(
                    asDoubleOrDefault(arr[0], 1.0),
                    asDoubleOrDefault(arr[1], 1.0),
                    asDoubleOrDefault(arr[2], 1.0)
                );
        }
    }

    return defaultVal;
}

std::shared_ptr<TriangleMesh> loadPrimitiveShape(tinygltf::Model& file, tinygltf::Primitive& primitive)
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

	if(texCoords.empty())
	{
        return std::make_shared<TriangleMesh>(vertices, indices, normals, indices, texCoords, std::vector<std::array<uint32_t,3>>());
	}
	else
    {
        return std::make_shared<TriangleMesh>(vertices, indices, normals, indices, texCoords, indices);
	}
}

struct VideoFrameDefinition
{
    std::string frameDirectory;
    int frameIndex;
};
using VideoImageMapping = std::map<std::string, VideoFrameDefinition>;

VideoImageMapping loadVideoImageMapping(tinygltf::Material& mat)
{
    VideoImageMapping videoImageMapping;
    if(mat.extras.Has("VideoImageMapping"))
    {
        const auto& viMapping = mat.extras.Get("VideoImageMapping");
        if(viMapping.IsArray())
        {
            for(size_t i = 0; i < viMapping.ArrayLen(); i++)
            {
                const auto& curVIMapping = viMapping.Get(i);
                if(!curVIMapping.IsObject())
                {
                    continue;
                }
                auto imageName = curVIMapping.Get("ImageName").Get<std::string>();
                auto frameDirectory = curVIMapping.Get("FrameDirectory").Get<std::string>();
                auto frameIndex = curVIMapping.Get("Frame").Get<int>();
                VideoFrameDefinition entry;
                entry.frameIndex = frameIndex;
                entry.frameDirectory = frameDirectory;
                videoImageMapping[imageName] = entry;
            }
        }
    }
    return videoImageMapping;
}

std::shared_ptr<TextureUInt8> loadImage(tinygltf::Model& file, tinygltf::Image& img, const VideoImageMapping& viMapping)
{
    const auto& viEntry = viMapping.find(img.name);
    if(viEntry != viMapping.end())
    {
        std::ostringstream filename;
        filename << viEntry->second.frameDirectory << "/" << viEntry->second.frameIndex << ".jpg";

        std::vector<uint8_t> data;
        unsigned int width, height;
        read_jpeg_file(data, width, height, filename.str());
        return std::make_shared<TextureUInt8>(data, width, height);
    }

    if(img.component != 4) {
        throw std::runtime_error("Image data must be in RGBA format");
    }

    if(img.image.size() != img.width * img.height * img.component){
        throw std::runtime_error("Can't load image data");
    }

    return std::make_shared<TextureUInt8>(img.image, img.width, img.height);
}

std::shared_ptr<TextureUInt8> loadTexture(tinygltf::Model& file, tinygltf::Texture& tex, const VideoImageMapping& viMapping)
{
    return loadImage(file, file.images[tex.source], viMapping);
}

std::shared_ptr<IMaterial> loadMaterial(tinygltf::Model& file, tinygltf::Material& mat, tinygltf::Value& nodeProps)
{
    auto videoImageMapping = loadVideoImageMapping(mat);

    bool clearCoat = getBoolOrDefault(&nodeProps, "Material.ClearCoat", false);

    std::shared_ptr<IMaterial> resultMaterial;

    std::shared_ptr<MixMaterial> mixMat;
    if(clearCoat)
    {
        auto clearCoatMixMat = std::make_shared<FresnelMixMaterial>();
        auto clearCoatIor = getDoubleOrDefault(&nodeProps, "Material.ClearCoatIOR", 1.45f);
        clearCoatMixMat->IOR = clearCoatIor;
        mixMat = clearCoatMixMat;
    }
    else
    {
        auto constMixMat = std::make_shared<ConstMixMaterial>();

        auto metallicFactorIt = mat.values.find("metallicFactor");
        if(metallicFactorIt != mat.values.end()){
            auto metallicFactor = metallicFactorIt->second.number_value;
            constMixMat->mixFactor = metallicFactor;
        }

        mixMat = constMixMat;
    }
    resultMaterial = mixMat;
    auto diffuse = std::make_shared<DiffuseMaterial>();
    auto glossy = std::make_shared<GlossyMaterial>();
    mixMat->first = diffuse;
    mixMat->second = glossy;

    auto baseColorTextureIt = mat.values.find("baseColorTexture");
    if(baseColorTextureIt != mat.values.end()){
        int index = static_cast<int>(baseColorTextureIt->second.json_double_value["index"]);
        auto tex = loadTexture(file, file.textures[index], videoImageMapping);
        tex->setGammaFactor(2.2); //sRGB to raw radiance
        diffuse->albedoMap = tex;
        //baseColorTextureIt->second.json_double_value["texCoord"]
    }

    auto baseColorFactorIt = mat.values.find("baseColorFactor");
    RGB baseColor{1.0f};
    if(baseColorFactorIt != mat.values.end()){
        auto baseColorFactor = baseColorFactorIt->second.number_array;
        baseColor = RGB(baseColorFactor[0], baseColorFactor[1], baseColorFactor[2]);
        diffuse->diffuseColor = baseColor;
        glossy->color = baseColor;
    }

    auto roughnessFactorIt = mat.values.find("roughnessFactor");
    if(roughnessFactorIt != mat.values.end()){
        auto roughnessFactor = roughnessFactorIt ->second.number_value;
        glossy->roughness = roughnessFactor;
    }

    auto normalTextureIt = mat.additionalValues.find("normalTexture");
    if(normalTextureIt != mat.additionalValues.end()){
        int index = static_cast<int>(normalTextureIt->second.json_double_value["index"]);
        auto tex = loadTexture(file, file.textures[index], videoImageMapping);
        diffuse->normalMap = tex;
        glossy->normalMap = tex;
        //normalTextureIt->second.json_double_value["texCoord"]
    }

    auto ior = getDoubleOrDefault(&nodeProps, "Material.IOR", 1.0);
    auto transmission = getDoubleOrDefault(&nodeProps, "Material.Transmission", 0.0);
    if(transmission > 0)
    {
        auto parentMixMat = std::make_shared<ConstMixMaterial>();
        auto glass = std::make_shared<GlassMaterial>();
        glass->ior = ior;
        glass->color = baseColor;
        parentMixMat->first = resultMaterial;
        parentMixMat->second = glass;
        parentMixMat->mixFactor = transmission;
        resultMaterial = parentMixMat;
    }

    auto emissiveFactorIt = mat.additionalValues.find("emissiveFactor");
    auto emissiveTextureIt = mat.additionalValues.find("emissiveTexture");
    if(emissiveFactorIt != mat.additionalValues.end() || emissiveTextureIt != mat.additionalValues.end())
    {
        auto emissive = std::make_shared<EmissiveMaterial>();

        if(emissiveTextureIt != mat.additionalValues.end())
        {
            int index = static_cast<int>(emissiveTextureIt->second.json_double_value["index"]);
            emissive->emissionMap = loadTexture(file, file.textures[index], videoImageMapping);
        }

        if(emissiveFactorIt != mat.additionalValues.end())
        {
            auto arr = emissiveFactorIt->second.number_array;
            emissive->color = RGB(arr[0], arr[1], arr[2]);
        }

        auto addMat = std::make_shared<AddMaterial>();
        addMat->first = emissive;
        addMat->second = resultMaterial;
        resultMaterial = addMat;
    }

    auto alpha = getDoubleOrDefault(&nodeProps, "Material.Alpha", 1.0);
    if(alpha < 1.0)
    {
        auto parentMixMat = std::make_shared<ConstMixMaterial>();
        parentMixMat->first = std::make_shared<TransparentMaterial>();
        parentMixMat->second = resultMaterial;
        parentMixMat->mixFactor = alpha;
        resultMaterial = parentMixMat;
    }

    return resultMaterial;
}

std::unique_ptr<Model> loadPrimitive(tinygltf::Model& file, tinygltf::Primitive& primitive, tinygltf::Value& nodeProps,
        std::map<tinygltf::Primitive*, std::shared_ptr<TriangleMesh>>& meshCache, std::map<int32_t, std::shared_ptr<IMaterial>>& materialCache)
{
    std::shared_ptr<TriangleMesh> shape;
    auto shapeIt = meshCache.find(&primitive);
    if(shapeIt == meshCache.end())
    {
        shape = loadPrimitiveShape(file, primitive);
        meshCache[&primitive] = shape;
    }
    else
    {
        shape = shapeIt->second;
    }

	std::shared_ptr<IMaterial> mat;
    auto it = materialCache.find(primitive.material);
    if(it == materialCache.end())
    {
        if (primitive.material != -1)
        {
            mat = loadMaterial(file, file.materials[primitive.material], nodeProps);
        }
        else
        {
            auto diff = std::make_shared<DiffuseMaterial>();
            diff->diffuseColor = { 1.0, 1.0, 1.0 };
            diff->diffuseIntensity = 0.8;
            mat = diff;
        }
        materialCache[primitive.material] = mat;
    }
    else
    {
        mat = it->second;
    }

	return std::make_unique<Model>(shape, mat);
}

std::unique_ptr<DynamicSceneNode> loadNode(tinygltf::Model& file, int nodeI, float imageAspectRatio,
       std::map<tinygltf::Primitive*, std::shared_ptr<TriangleMesh>>& meshCache, std::map<int32_t, std::shared_ptr<IMaterial>>& materialCache, tinygltf::Node* parent = nullptr)
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

    if(getBoolOrDefault(tryGetExtras(&node), "IsAreaLight", false))
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
        result->areaLight->intensity = getDoubleOrDefault(tryGetExtras(&node), "LightIntensity", 500);
        result->areaLight->color = getColorOrDefault(tryGetExtras(&node), "LightColor", RGB(1.0, 1.0, 1.0));
    }
    else if(getBoolOrDefault(tryGetExtras(&node), "IsPointLight", false))
    {
        result->pointLight = std::make_unique<PointLight>();
        result->pointLight->intensity = getDoubleOrDefault(tryGetExtras(&node), "LightIntensity", 500);
        result->pointLight->color = getColorOrDefault(tryGetExtras(&node), "LightColor", RGB(1.0, 1.0, 1.0));
    }
    else if(getBoolOrDefault(tryGetExtras(&node), "IsDirectionalLight", false))
    {
        result->directionalLight = std::make_unique<DirectionalLight>();
        result->directionalLight->intensity = getDoubleOrDefault(tryGetExtras(&node), "LightIntensity", 500);
        result->directionalLight->angle = 0.5 * getDoubleOrDefault(tryGetExtras(&node), "DirectionalLight.Angle", 0.009180);
        result->directionalLight->direction = Vector3(0, -1, 0);
        result->directionalLight->color = getColorOrDefault(tryGetExtras(&node), "LightColor", RGB(1.0, 1.0, 1.0));
    }
	else if (node.mesh != -1)
	{
		auto& meshDef = file.meshes[node.mesh];

		if (meshDef.primitives.size() == 1)
		{
			result->model = loadPrimitive(file, meshDef.primitives[0], node.extras, meshCache, materialCache);
		}
		else
		{
			for (auto& primitive : meshDef.primitives)
			{
				auto child = std::make_unique<DynamicSceneNode>();
				child->transform = Transformation::IDENTITY;
				child->model = loadPrimitive(file, primitive, node.extras, meshCache, materialCache);
				result->children.push_back(std::move(child));
			}
		}
	}
	else if (node.camera != -1)
	{
		auto& cameraDef = file.cameras[node.camera];
		if (cameraDef.type == "perspective")
		{
		    double xFOVRad = std::atan(std::tan(cameraDef.perspective.yfov/2.0) * imageAspectRatio)*2.0;
		    double xFOV = (xFOVRad / PI) * 180.0;
			result->camera = std::make_unique<PerspectiveCamera>(xFOV);

            result->camera->isMainCamera = getBoolOrDefault(tryGetExtras(&node), "IsMainCamera", false) || getBoolOrDefault(tryGetExtras(parent), "IsMainCamera", false);

            if(node.extras.Has("FocalDistance")){
                result->camera->focalDistance = getDoubleOrDefault(tryGetExtras(&node), "FocalDistance", 0.0);
            }else{
                result->camera->focalDistance = getDoubleOrDefault(tryGetExtras(parent), "FocalDistance", 0.0);
            }

            if(node.extras.Has("FStop") && result->camera->focalDistance > 0){
                auto fstop = getDoubleOrDefault(tryGetExtras(&node), "FStop", 0.0);
                result->camera->aperture = 0.5 * result->camera->focalDistance / fstop;
            }else if(parent->extras.Has("FStop") && result->camera->focalDistance > 0){
                auto fstop = getDoubleOrDefault(tryGetExtras(parent), "FStop", 0.0);
                result->camera->aperture = 0.5 * result->camera->focalDistance / fstop;
            }else if(node.extras.Has("Aperture")){
                result->camera->aperture = getDoubleOrDefault(tryGetExtras(&node), "Aperture", 0.0);
            }else{
                result->camera->aperture = getDoubleOrDefault(tryGetExtras(parent), "Aperture", 0.0);
            }
		}
	}

	for (auto subNodeI : node.children)
	{
		result->children.push_back(loadNode(file, subNodeI, imageAspectRatio, meshCache, materialCache, &node));
	}

	return result;
}

void loadEnvironmentInfo(const tinygltf::Scene& gltfScene, DynamicScene& outputScene)
{
    double envIntensity = getDoubleOrDefault(&gltfScene.extras, "EnvironmentIntensity", 0.0);
    if(envIntensity > 0)
    {
        if(gltfScene.extras.Has("EnvironmentColor"))
        {
            RGB color = getColorOrDefault(&gltfScene.extras, "EnvironmentColor", RGB::BLACK);
            auto env = std::make_unique<ColorEnvironment>();
            env->color = color;
            env->intensity = envIntensity;
            outputScene.environmentMaterial = std::move(env);
        }
    }
    //
    //model.materials
}

}

DynamicScene loadGLTFScene(const std::string& file, float imageAspectRatio)
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

    std::map<tinygltf::Primitive*, std::shared_ptr<TriangleMesh>> meshCache{};
    std::map<int32_t, std::shared_ptr<IMaterial>> materialCache{};

	DynamicScene scene;
	scene.root = std::make_unique<DynamicSceneNode>();
	for (auto nodeI : model.scenes[model.defaultScene].nodes)
	{
		scene.root->children.push_back(loadNode(model, nodeI, imageAspectRatio, meshCache, materialCache));
	}
	loadEnvironmentInfo(model.scenes[model.defaultScene], scene);

	return scene;
}