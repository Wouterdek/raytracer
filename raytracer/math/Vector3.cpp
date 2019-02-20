#include "Vector3.h"
#include <sstream>

std::string formatRow(const Vector3& v)
{
	std::ostringstream s;
	s << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
	return s.str();
}