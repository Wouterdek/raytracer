#pragma once

#include "Matrix.h"
#include "Vector3.h"
#include "Ray.h"

class Transformation
{
public:
	Transformation(Matrix matrix, Matrix inverse);

	const Matrix& getMatrix() const;
	const Matrix& getInverse() const;
	Transformation invert() const;
	Transformation append(const Transformation& transform) const;
	
	Point transform(const Point& point) const;
	Point transformInverse(const Point& point) const;
	Vector3 transform(const Vector3& vector) const;
	Vector3 transformInverse(const Vector3& vector) const;
	Vector3 transformNormal(const Vector3& normal) const;
	Ray transform(const Ray& ray) const;
	Ray transformInverse(const Ray& ray) const;
	std::pair<Ray, double> transformInverseAndGetTScale(const Ray & ray) const;

	static Transformation translate(double x, double y, double z);
	static Transformation translate(Vector3 vect);
	static Transformation scale(double x, double y, double z);
	static Transformation rotateX(double angle);
	static Transformation rotateY(double angle);
	static Transformation rotateZ(double angle);
	static Transformation rotate(const Vector3& axis, double angle);
	static Transformation rotateQuaternion(double i, double j, double k, double r);
	static Transformation lookat(Vector3 direction, Vector3 up);

	static const Transformation IDENTITY;

private:
	Matrix matrix;
	Matrix inverse;
};