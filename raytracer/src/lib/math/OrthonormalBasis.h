#pragma once

#include "Vector3.h"
#include "Transformation.h"

class OrthonormalBasis
{
public:
	OrthonormalBasis(Vector3 u, Vector3 v, Vector3 w);
	explicit OrthonormalBasis(const Vector3& a);
	OrthonormalBasis(const Vector3& a, const Vector3& b);

	const Vector3& getU() const
    {
        return this->u;
    }

	const Vector3& getV() const
    {
        return this->v;
    }

	const Vector3& getW() const
    {
        return this->w;
    }

    Vector3 applyBasisTo(const Vector3 &vect) const;
    Vector3 invertApplyBasisTo(const Vector3 &vect) const;

private:
	Vector3 u;
	Vector3 v;
	Vector3 w;
};

std::ostream& operator<<(std::ostream& in, const OrthonormalBasis& ray);