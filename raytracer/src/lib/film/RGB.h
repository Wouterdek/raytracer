#pragma once

#include <sstream>

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

	bool isValidColorComponent(component value) const;

	RGB add(RGB rgb) const;
	RGB add(component r, component g, component b) const;
	RGB subtract(RGB rgb) const;
	RGB subtract(component r, component g, component b) const;
	RGB scale(double scalar) const;
	RGB divide(double divisor) const;
    RGB divide(RGB divisor) const;
	RGB multiply(RGB spectrum) const;
	RGB pow(double power) const;
	RGB clamp(component low, component high) const;
	bool isBlack() const;
	int toRGB() const;

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