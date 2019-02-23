#include "Transformation.h"
#include "Constants.h"

const Transformation Transformation::IDENTITY = Transformation(Matrix::Identity(), Matrix::Identity());

Transformation::Transformation(Matrix matrix, Matrix inverse)
	: matrix(std::move(matrix)), inverse(std::move(inverse))
{ }

const Matrix& Transformation::getMatrix() const
{
	return this->matrix;
}

const Matrix& Transformation::getInverse() const
{
	return this->inverse;
}

Transformation Transformation::invert() const
{
	return Transformation(this->inverse, this->matrix);
}

Transformation Transformation::append(const Transformation& transform) const
{
	return Transformation(this->matrix * transform.matrix, this->inverse * transform.inverse);
}

Point Transformation::transform(const Point & point) const
{
	return (this->matrix * Eigen::Vector4d{point.x(), point.y(), point.z(), 1.0 }).hnormalized().cast<float>();
}

Point Transformation::transformInverse(const Point & point) const
{
	return (this->inverse * Eigen::Vector4d{ point.x(), point.y(), point.z(), 1.0 }).hnormalized().cast<float>();
}

Vector3 Transformation::transform(const Vector3 & vector) const
{
	Eigen::Vector4f val = (this->matrix * Eigen::Vector4d{ vector.x(), vector.y(), vector.z(), 0.0 }).cast<float>();
	return Vector3 { val.x(), val.y(), val.z() };
}

Vector3 Transformation::transformInverse(const Vector3 & vector) const
{
	Eigen::Vector4f val = (this->inverse * Eigen::Vector4d{ vector.x(), vector.y(), vector.z(), 0.0 }).cast<float>();
	return Vector3{ val.x(), val.y(), val.z() };
}

Ray Transformation::transform(const Ray & ray) const
{
	auto point = transform(ray.getOrigin());
	auto direction = transform(ray.getDirection());
	return Ray(point, direction);
}

Ray Transformation::transformInverse(const Ray & ray) const
{
	auto point = transformInverse(ray.getOrigin());
	auto direction = transformInverse(ray.getDirection());
	return Ray(point, direction);
}

Transformation Transformation::translate(double x, double y, double z)
{
	Matrix transform {};
	transform << 1.0f, 0.0f, 0.0f, x,
				 0.0f, 1.0f, 0.0f, y,
				 0.0f, 0.0f, 1.0f, y,
				 0.0f, 0.0f, 0.0f, 1.0f;

	Matrix inverse {};
	inverse << 1.0f, 0.0f, 0.0f, -x,
			   0.0f, 1.0f, 0.0f, -y,
			   0.0f, 0.0f, 1.0f, -y,
			   0.0f, 0.0f, 0.0f, 1.0f;

	return Transformation(transform, inverse);
}

Transformation Transformation::scale(double x, double y, double z)
{
	Matrix transform{};
	transform << x, 0, 0, 0,
				 0, y, 0, 0,
				 0, 0, z, 0,
				 0, 0, 0, 1;

	Matrix inverse{};
	transform << 1/x, 0,   0,   0,
				 0,   1/y, 0,   0,
				 0,   0,   1/z, 0,
				 0,   0,   0,   1;

	return Transformation(transform, inverse);
}

Transformation Transformation::rotateX(double angle)
{
	auto rad = (angle / 180.0) * PI;
	auto sinval = sin(rad);
	auto cosval = cos(rad);

	Matrix transform{};
	transform << 1, 0, 0, 0,
				 0, cosval, -sinval, 0,
				 0, sinval, cosval, 0,
				 0, 0, 0, 1;
	Matrix inverse = transform.transpose();

	return Transformation(transform, inverse);
}

Transformation Transformation::rotateY(double angle)
{
	auto rad = (angle / 180.0) * PI;
	auto sinval = sin(rad);
	auto cosval = cos(rad);

	Matrix transform{};
	transform << cosval, 0, sinval, 0,
				 0, 1, 0, 0,
				 -sinval, 0, cosval, 0,
				 0, 0, 0, 1;
	Matrix inverse = transform.transpose();

	return Transformation(transform, inverse);
}

Transformation Transformation::rotateZ(double angle)
{
	auto rad = (angle / 180.0) * PI;
	auto sinval = sin(rad);
	auto cosval = cos(rad);

	Matrix transform{};
	transform << cosval, -sinval, 0, 0,
				 sinval, cosval, 0, 0,
				 0, 0, 1, 0,
				 0, 0, 0, 1;
	Matrix inverse = transform.transpose();

	return Transformation(transform, inverse);
}

Transformation Transformation::rotate(const Vector3 & axis, double angle)
{
	auto n = axis.normalized();

	auto rad = (angle / 180.0) * PI;
	auto sinval = sin(rad);
	auto cosval = cos(rad);
	auto ncos = 1.0 - cosval;

	Matrix transform{};
	transform << n.x() * n.x() * ncos + cosval,
		n.y() * n.x() * ncos - n.z() * sinval,
		n.z() * n.x() * ncos + n.y() * sinval, 0,
		n.x() * n.y() * ncos + n.z() * sinval,
		n.y() * n.y() * ncos + cosval,
		n.z() * n.y() * ncos - n.x() * sinval, 0,
		n.x() * n.z() * ncos - n.y() * sinval,
		n.y() * n.z() * ncos + n.x() * sinval,
		n.z() * n.z() * ncos + cosval, 0,
		0, 0, 0, 1;
	Matrix inverse = transform.transpose();

	return Transformation(transform, inverse);
}
