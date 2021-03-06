#include "Transformation.h"
#include "Constants.h"
#include "OrthonormalBasis.h"

const Transformation Transformation::IDENTITY = Transformation(Matrix::Identity(), Matrix::Identity());

std::ostream & operator<<(std::ostream & in, const Transformation& t)
{
    in << "[" << t.getMatrix() << "]" << std::endl;
    return in;
}

Transformation Transformation::translate(Vector3 vect)
{
	return translate(vect[0], vect[1], vect[2]);
}

Transformation Transformation::scale(double x, double y, double z)
{
	Matrix transform{};
	transform << x, 0, 0, 0,
				 0, y, 0, 0,
				 0, 0, z, 0,
				 0, 0, 0, 1;

	Matrix inverse{};
	inverse << 1/x, 0,   0,   0,
				 0,   1/y, 0,   0,
				 0,   0,   1/z, 0,
				 0,   0,   0,   1;

	return Transformation(transform, inverse);
}

Transformation Transformation::rotateX(double rad)
{
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

Transformation Transformation::rotateY(double rad)
{
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

Transformation Transformation::rotateZ(double rad)
{
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

Transformation Transformation::rotate(const Vector3 & axis, double rad)
{
	auto n = axis.normalized();

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

Transformation Transformation::rotateQuaternion(double i, double j, double k, double r)
{
	auto s = 1.0 / Eigen::Vector4d(r, i, j, k).squaredNorm();

	Matrix transform{};
	transform << 
		1 - 2 * s * ((j * j) + (k * k)), 2 * s * (i * j - k * r),           2 * s * (i * k + j * r), 0,
		2 * s * (i * j + k * r),         1.0 - 2 * s * ((i * i) + (k * k)), 2 * s * (j * k - i * r), 0,
		2 * s * (i * k - j * r),         2 * s * (j * k + i * r),           1 - 2 * s * ((i * i) + (j * j)), 0,
		0, 0, 0, 1;
	Matrix inverse = transform.inverse();

	return Transformation(transform, inverse);
}

Transformation Transformation::lookat(Vector3 direction, Vector3 up)
{
	OrthonormalBasis basis(-direction, up);

	Matrix transMat = Matrix::Zero();
	transMat.block<3, 1>(0, 0) = basis.getU().cast<double>();
	transMat.block<3, 1>(0, 1) = basis.getV().cast<double>();
	transMat.block<3, 1>(0, 2) = basis.getW().cast<double>();
	transMat(3, 3) = 1;

	return Transformation(transMat, transMat.inverse());
}

Transformation Transformation::tangentToObject(Vector3 tangent, Vector3 normal)
{
    tangent.normalize();
    normal.normalize();

    Matrix transform = Matrix::Zero();
    transform.block<3, 1>(0, 0) = tangent.cast<double>();
    transform.block<3, 1>(0, 1) = normal.cross(tangent).cast<double>();
    transform.block<3, 1>(0, 2) = normal.cast<double>();
    transform(3, 3) = 1;
    Matrix inverse = transform.transpose();

    return Transformation(transform, inverse);
}

