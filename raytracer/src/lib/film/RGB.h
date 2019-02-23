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

	double getRed() const;
	double getGreen() const;
	double getBlue() const;

private:
	double red;
	double green;
	double blue;
};

std::ostream& operator<<(std::ostream& in, const RGB& rgb);