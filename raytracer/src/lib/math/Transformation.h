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
	static Transformation rotateX(double rad);
	static Transformation rotateY(double rad);
	static Transformation rotateZ(double rad);
	static Transformation rotate(const Vector3& axis, double rad);
	static Transformation rotateQuaternion(double i, double j, double k, double r);
	static Transformation lookat(Vector3 direction, Vector3 up);

	static const Transformation IDENTITY;

private:
	Matrix matrix;
	Matrix inverse;
};

inline Transformation::Transformation(Matrix matrix, Matrix inverse)
        : matrix(std::move(matrix)), inverse(std::move(inverse))
{ }

inline const Matrix& Transformation::getMatrix() const
{
    return this->matrix;
}

inline const Matrix& Transformation::getInverse() const
{
    return this->inverse;
}

inline Transformation Transformation::invert() const
{
    return Transformation(this->inverse, this->matrix);
}

inline Transformation Transformation::append(const Transformation& transform) const
{
    return Transformation(this->matrix * transform.matrix, transform.inverse * this->inverse);
}

inline Point Transformation::transform(const Point & point) const
{
    Eigen::Vector4d v = (this->matrix * Eigen::Vector4d{ point.x(), point.y(), point.z(), 1.0 });
    Point p { (float)(v.x() / v.w()), (float)(v.y() / v.w()), (float)(v.z() / v.w()) };
    return p;
}

inline Point Transformation::transformInverse(const Point & point) const
{
    return (this->inverse * Eigen::Vector4d{ point.x(), point.y(), point.z(), 1.0 }).hnormalized().cast<float>();
}

inline Vector3 Transformation::transform(const Vector3 & vector) const
{
    Eigen::Vector4f val = (this->matrix * Eigen::Vector4d{ vector.x(), vector.y(), vector.z(), 0.0 }).cast<float>();
    return Vector3 { val.x(), val.y(), val.z() };
}

inline Vector3 Transformation::transformInverse(const Vector3 & vector) const
{
    Eigen::Vector4f val = (this->inverse * Eigen::Vector4d{ vector.x(), vector.y(), vector.z(), 0.0 }).cast<float>();
    return Vector3{ val.x(), val.y(), val.z() };
}

inline Vector3 Transformation::transformNormal(const Vector3& normal) const
{
    Eigen::Vector4f val = (Eigen::Vector4d{ normal.x(), normal.y(), normal.z(), 0.0 }.transpose() * this->inverse).cast<float>();
    return Vector3{ val.x(), val.y(), val.z() };
}

inline Ray Transformation::transform(const Ray & ray) const
{
    auto point = transform(ray.getOrigin());
    auto direction = transform(ray.getDirection());
    return Ray(point, direction);
}

inline Ray Transformation::transformInverse(const Ray & ray) const
{
    auto point = transformInverse(ray.getOrigin());
    auto direction = transformInverse(ray.getDirection());
    return Ray(point, direction);
}

inline std::pair<Ray, double> Transformation::transformInverseAndGetTScale(const Ray & ray) const
{
    auto point = transformInverse(ray.getOrigin());
    auto direction = transformInverse(ray.getDirection());
    auto scale = ray.getDirection().norm() / direction.norm();
    return std::make_pair(Ray(point, direction), scale);
}

inline Transformation Transformation::translate(double x, double y, double z)
{
    Matrix transform {};
    transform << 1.0f, 0.0f, 0.0f, x,
            0.0f, 1.0f, 0.0f, y,
            0.0f, 0.0f, 1.0f, z,
            0.0f, 0.0f, 0.0f, 1.0f;

    Matrix inverse {};
    inverse << 1.0f, 0.0f, 0.0f, -x,
            0.0f, 1.0f, 0.0f, -y,
            0.0f, 0.0f, 1.0f, -z,
            0.0f, 0.0f, 0.0f, 1.0f;

    return Transformation(transform, inverse);
}