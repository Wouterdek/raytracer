#include "../shape/IShape.h"

class Box : public IShape
{
public:
	Box(Vector3 end);
	std::optional<RayHitInfo> intersect(const Ray& ray, const Transformation& transform) const override;
//private:
	Vector3 end;
};