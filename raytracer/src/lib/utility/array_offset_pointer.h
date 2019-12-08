#pragma once

template<typename T>
class array_offset_pointer
{
public:
    array_offset_pointer(T* t) : ptr(t){}
    array_offset_pointer() : ptr(nullptr) {}

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

    static T* array;
    T* ptr;

    int offset() const
    {
        return ptr - array;
    }
};

