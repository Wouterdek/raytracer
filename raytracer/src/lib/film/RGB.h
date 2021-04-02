#pragma once

#include <sstream>
#include <cmath>
#include <cassert>

class RGB
{
public:
	static const RGB BLACK;
	using component = float;

    RGB() : red(0), green(0), blue(0)
    { }

    explicit RGB(component value)
            : RGB(value, value, value)
    { }

	RGB(component red, component green, component blue);

	static bool isValidColorComponent(component value);

	RGB add(RGB rgb) const
    {
        return RGB(this->red + rgb.red, this->green + rgb.green, this->blue + rgb.blue);
    }

	RGB add(component r, component g, component b) const
    {
        return this->add(RGB(r, g, b));
    }

	RGB subtract(RGB rgb) const
    {
        return RGB(this->red - rgb.red, this->green - rgb.green, this->blue - rgb.blue);
    }

	RGB subtract(component r, component g, component b) const
    {
        return this->subtract(RGB(r, g, b));
    }

	RGB scale(double scalar) const
    {
        return RGB(this->red * scalar, this->green * scalar, this->blue * scalar);
    }

    RGB scale(float scalar) const
    {
        return RGB(this->red * scalar, this->green * scalar, this->blue * scalar);
    }

    RGB scale(int intScalar) const
    {
	    float scalar = static_cast<float>(intScalar);
        return RGB(this->red * scalar, this->green * scalar, this->blue * scalar);
    }

	RGB divide(double divisor) const
    {
        return scale(1.0/divisor);
    }

    RGB divide(float divisor) const
    {
        return scale(1.0f/divisor);
    }

    RGB divide(int divisor) const
    {
        return scale(1.0f/(float)divisor);
    }

    RGB divide(RGB divisor) const
    {
        return RGB{red/divisor.red, green/divisor.green, blue/divisor.blue};
    }

	RGB multiply(RGB spectrum) const
    {
        return RGB(this->red * spectrum.red, this->green * spectrum.green, this->blue * spectrum.blue);
    }

	RGB pow(double power) const
    {
        return RGB(std::pow(this->red, power), std::pow(this->green, power), std::pow(this->blue, power));
    }

	RGB clamp(component low, component high) const;
	bool isBlack() const;

	RGB& operator+=(const RGB& rhs)
	{
		this->red += rhs.red;
		this->green += rhs.green;
		this->blue += rhs.blue;
		return *this;
	}

	friend RGB operator+(RGB lhs, const RGB& rhs)
	{
		lhs += rhs;
		return lhs;
	}

	RGB& operator-=(const RGB& rhs)
	{
        assert(isValidColorComponent(red - rhs.red));
        assert(isValidColorComponent(green - rhs.green));
        assert(isValidColorComponent(blue - rhs.blue));

		this->red -= rhs.red;
		this->green -= rhs.green;
		this->blue -= rhs.blue;
		return *this;
	}

	friend RGB operator-(RGB lhs, const RGB& rhs)
	{
		lhs -= rhs;
		return lhs;
	}

	friend RGB operator*(RGB lhs, double rhs)
	{
        assert(isValidColorComponent(lhs.red * rhs));
        assert(isValidColorComponent(lhs.green * rhs));
        assert(isValidColorComponent(lhs.blue * rhs));

		lhs.red *= rhs;
		lhs.green *= rhs;
		lhs.blue *= rhs;
		return lhs;
	}

    component getRed() const
	{
		return this->red;
	}

    component getGreen() const
	{
		return this->green;
	}

    component getBlue() const
	{
		return this->blue;
	}

private:
    component red;
    component green;
    component blue;
};

std::ostream& operator<<(std::ostream& in, const RGB& rgb);