#pragma once

template<typename T>
class raw_pointer
{
public:
	raw_pointer(T* t) : ptr(t){}
	raw_pointer() : ptr(nullptr) {}

    operator T*()
    {
        return ptr;
    }

    operator const T*() const
    {
        return ptr;
    }

	T& operator*() const
	{
		return *ptr;
	}

	T* operator->() const
	{
		return ptr;
	}

	T* ptr;
};