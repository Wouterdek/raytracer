#include "RGB.h"
#include <exception>
#include <algorithm>

const RGB RGB::BLACK(0,0,0);

RGB::RGB(component red, component green, component blue)
	: red(red), green(green), blue(blue)
{
	if(!isValidColorComponent(red) || !isValidColorComponent(green) || !isValidColorComponent(blue))
	{
		throw std::invalid_argument("one or more values are not valid RGB components");
	}
}

bool RGB::isValidColorComponent(component value) const
{
	return !std::isinf(value) && !std::isnan(value) && value >= 0;
}

RGB RGB::clamp(component low, component high) const
{
	return RGB(
		std::clamp(this->red, low, high), 
		std::clamp(this->green, low, high), 
		std::clamp(this->blue, low, high)
	);
}

bool RGB::isBlack() const
{
	return this->red == 0 && this->green == 0 && this->blue == 0;
}

std::ostream & operator<<(std::ostream & in, const RGB & rgb)
{
	in << "(" << rgb.getRed() << ", " << rgb.getGreen() << ", " << rgb.getBlue() << ")";
	return in;
}
