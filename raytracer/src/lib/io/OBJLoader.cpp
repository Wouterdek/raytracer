#include "OBJLoader.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "io/lib/tiny_obj_loader.h"

TriangleMesh loadOBJMesh(std::string path)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str());

	if (ret)
	{
		std::vector<Point> vertices;
		vertices.reserve(attrib.vertices.size() / 3);

		for (int i = 0; i < attrib.vertices.size(); i += 3)
		{
			vertices.emplace_back(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]);
		}

		std::vector<Vector3> normals;
		normals.reserve(attrib.normals.size() / 3);

		for (int i = 0; i < attrib.normals.size(); i += 3)
		{
			normals.emplace_back(attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2]);
		}

		std::vector<Vector2> texCoords;
		texCoords.reserve(attrib.texcoords.size() / 2);

		for (int i = 0; i < attrib.texcoords.size(); i += 2)
		{
			texCoords.emplace_back(attrib.texcoords[i], 1 - attrib.texcoords[i + 1]);
		}

		std::vector<std::array<uint32_t, 3>> indices;
		std::vector<std::array<uint32_t, 3>> normalIndices;
		std::vector<std::array<uint32_t, 3>> texCoordIndices;
		auto& objIndices = shapes[0].mesh.indices;
		indices.reserve(objIndices.size() / 3);
		normalIndices.reserve(objIndices.size() / 3);
		texCoordIndices.reserve(objIndices.size() / 3);
		for (int i = 0; i < objIndices.size(); i += 3)
		{
			indices.emplace_back<std::array<uint32_t, 3>>({
				static_cast<uint32_t>(objIndices[i].vertex_index),
				static_cast<uint32_t>(objIndices[i + 1].vertex_index),
				static_cast<uint32_t>(objIndices[i + 2].vertex_index)
				});
			normalIndices.emplace_back<std::array<uint32_t, 3>>({
				static_cast<uint32_t>(objIndices[i].normal_index),
				static_cast<uint32_t>(objIndices[i + 1].normal_index),
				static_cast<uint32_t>(objIndices[i + 2].normal_index)
				});
			texCoordIndices.emplace_back<std::array<uint32_t, 3>>({
				static_cast<uint32_t>(objIndices[i].texcoord_index),
				static_cast<uint32_t>(objIndices[i + 1].texcoord_index),
				static_cast<uint32_t>(objIndices[i + 2].texcoord_index)
				});
		}

		return TriangleMesh(vertices, indices, normals, normalIndices, texCoords, texCoordIndices);
	}

	throw std::runtime_error("bad file");
}
