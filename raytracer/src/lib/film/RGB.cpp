#include "RGB.h"
#include <exception>
#include <algorithm>
#include <cmath>

const RGB RGB::BLACK(0,0,0);

RGB::RGB()
	: red(0), green(0), blue(0)
{ }

RGB::RGB(component value)
	: RGB(value, value, value)
{ }

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

RGB RGB::add(RGB rgb) const
{
	return RGB(this->red + rgb.red, this->green + rgb.green, this->blue + rgb.blue);
}

RGB RGB::add(component r, component g, component b) const
{
	return this->add(RGB(r, g, b));
}

RGB RGB::subtract(RGB rgb) const
{
	return RGB(this->red - rgb.red, this->green - rgb.green, this->blue - rgb.blue);
}

RGB RGB::subtract(component r, component g, component b) const
{
	return this->subtract(RGB(r, g, b));
}

RGB RGB::scale(double scalar) const
{
	return RGB(this->red * scalar, this->green * scalar, this->blue * scalar);
}

RGB RGB::divide(double divisor) const
{
	return scale(1.0/divisor);
}

RGB RGB::multiply(RGB spectrum) const
{
	return RGB(this->red * spectrum.red, this->green * spectrum.green, this->blue * spectrum.blue);
}

RGB RGB::pow(double power) const
{
	return RGB(std::pow(this->red, power), std::pow(this->green, power), std::pow(this->blue, power));
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

int RGB::toRGB() const
{
	int a = 255;
	int r = std::min(255, static_cast<int>(round(red)));
	int g = std::min(255, static_cast<int>(round(green)));
	int b = std::min(255, static_cast<int>(round(blue)));

	return (a << 24) + (r << 16) + (g << 8) + b;
}

std::ostream & operator<<(std::ostream & in, const RGB & rgb)
{
	in << "(" << rgb.getRed() << ", " << rgb.getGreen() << ", " << rgb.getBlue() << ")";
	return in;
}
