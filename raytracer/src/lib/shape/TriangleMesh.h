#include "IShape.h"
#include "../math/Vector3.h"

class TriangleMesh : public IShape
{
public:
	TriangleMesh(std::vector<Point> vertices, std::vector<uint32_t> vertexIndices, std::vector<Vector3> normals, std::vector<uint32_t> normalIndices);

	std::optional<RayHitInfo> intersect(const Ray& ray, const Transformation& transform) const override;
//private:
	std::vector<Point> vertices;
	std::vector<uint32_t> vertexIndices;
	std::vector<Vector3> normals;
	std::vector<uint32_t> normalIndices;
};