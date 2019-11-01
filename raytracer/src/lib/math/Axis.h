#pragma once

#include <array>

enum class Axis : unsigned char
{
	x = 0, y = 1, z = 2
};
constexpr int nbOfAxes = 3;

static constexpr std::array<Axis, nbOfAxes> Axes = {Axis::x, Axis::y, Axis::z };

inline std::array<Axis, nbOfAxes> getAxesStartingWith(Axis start)
{
	std::array<Axis, nbOfAxes> axes{};
	std::array<Axis, nbOfAxes>::size_type i = 0;

	axes[i] = start;
	i++;
	for (auto axis : Axes)
	{
		if(axis != start)
		{
			axes[i] = axis;
			i++;
		}
	}
	return axes;
}