#pragma once

#include <memory>

//Provides shallow cloning (copy constructor)
template<class Derived>
class ICloneable
{
public:
	virtual ~ICloneable() = default;

	std::unique_ptr<Derived> clone() const
	{
		return std::unique_ptr<Derived>(this->cloneImpl());
	}

private:
	virtual Derived* cloneImpl() const = 0;
};
