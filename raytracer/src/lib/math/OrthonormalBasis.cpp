#include "OrthonormalBasis.h"
#include <exception>
#include <iostream>

OrthonormalBasis::OrthonormalBasis(Vector3 u, Vector3 v, Vector3 w)
	: u(std::move(u)), v(std::move(v)), w(std::move(w))
{ }

OrthonormalBasis::OrthonormalBasis(const Vector3 & a)
{
	auto length = a.norm();
	if(length == 0)
	{
		throw std::invalid_argument("The given vector has length zero.");
	}

	this->w = a / length;

	if(abs(w.x()) > abs(w.y()))
	{
		auto inv_length = static_cast<float>(1.0f / std::sqrt((w.x() * w.x()) + (w.z() * w.z())));
		this->u = Vector3(-w.z() * inv_length, 0.0f, w.x() * inv_length);
	}
	else
	{
		auto inv_length = static_cast<float>(1.0f / std::sqrt((w.y() * w.y()) + (w.z() * w.z())));
		this->u = Vector3(0.0f, w.z() * inv_length, -w.y() * inv_length);
	}
	this->v = w.cross(u);
}

OrthonormalBasis::OrthonormalBasis(const Vector3 & a, const Vector3 & b)
{
	auto cross = b.cross(a);
	auto length = cross.norm();

	if(length == 0)
	{
		throw std::invalid_argument("The vectors are colinear!");
	}
	else if(length < 1e-8)
	{
		std::cout << "Warning: vector a and b are nearly colinear" << std::endl;
		std::cout << "a = " << formatRow(a) << ", b = " << formatRow(b) << std::endl;
	}

	w = a.normalized();
	u = cross / length;
	v = w.cross(u);
}

std::ostream & operator<<(std::ostream & in, const OrthonormalBasis & b)
{
	in << "[OrthonormalBasis]"
		<< " u" << formatRow(b.getU())
		<< " v" << formatRow(b.getV())
		<< " w" << formatRow(b.getW());
	return in;
}
