#pragma once

#include <sstream>

class RGB
{
public:
	static const RGB BLACK;

	RGB();
	explicit RGB(double value);
	RGB(double red, double green, double blue);

	bool isValidColorComponent(double value) const;

	RGB add(RGB rgb) const;
	RGB add(double r, double g, double b) const;
	RGB subtract(RGB rgb) const;
	RGB subtract(double r, double g, double b) const;
	RGB scale(double scalar) const;
	RGB divide(double divisor) const;
	RGB multiply(RGB spectrum) const;
	RGB pow(double power) const;
	RGB clamp(double low, double high) const;
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

	double getRed() const
	{
		return this->red;
	}

	double getGreen() const
	{
		return this->green;
	}

	double getBlue() const
	{
		return this->blue;
	}

private:
	double red;
	double green;
	double blue;
};

std::ostream& operator<<(std::ostream& in, const RGB& rgb);