#pragma once

#include "Vector3.h"

class OrthonormalBasis
{
public:
	OrthonormalBasis(Vector3 u, Vector3 v, Vector3 w);
	explicit OrthonormalBasis(const Vector3& a);
	OrthonormalBasis(const Vector3& a, const Vector3& b);

	const Vector3& getU() const;
	const Vector3& getV() const;
	const Vector3& getW() const;

private:
	Vector3 u;
	Vector3 v;
	Vector3 w;
};

std::ostream& operator<<(std::ostream& in, const OrthonormalBasis& ray);