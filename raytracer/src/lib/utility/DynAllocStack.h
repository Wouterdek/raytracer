#pragma once

#include <array>
#include <typeinfo>

template<size_t bufferSize, size_t maxObjects, typename sizetype = unsigned int>
class DynAllocStack {
    struct Allocation
    {
        std::type_info type;
        void(*destroy)(const void*) = nullptr;
        sizetype size;
    };

    std::array<Allocation, maxObjects> allocations;
    std::array<char, bufferSize> buffer;

    sizetype allocatedBytes = 0;
    sizetype allocatedObjects = 0;
    sizetype bytePointer = 0;
    sizetype objectPointer = 0;

public:
    DynAllocStack() : allocations(), buffer()
    {}

    ~DynAllocStack()
    {
        clear();
    }

    template<typename T, typename ... Args>
    T* alloc(Args&& ... args)
    {
        if(allocatedObjects == maxObjects || sizeof(T) > (bufferSize - allocatedBytes))
        {
            return nullptr;
        }

        auto objI = allocatedObjects;
        allocatedObjects++;
        allocations[objI].type = typeid(T);
        allocations[objI].size = sizeof(T);
        allocations[objI].destroy = [](const void* x) { static_cast<const T*>(x)->~T(); };
        auto bytesI = allocatedBytes;
        allocatedBytes += sizeof(T);
        return new(&buffer[bytesI]) T(std::forward<Args>(args)...);
    }

    template<typename T>
    T* read()
    {
        if(objectPointer == allocatedObjects || allocations[objectPointer].type != typeid(T))
        {
            return nullptr;
        }

        bytePointer += allocations[objectPointer].size;
        objectPointer++;

        return static_cast<T*>(&buffer[bytePointer]);
    }

    void resetPointer()
    {
        bytePointer = objectPointer = 0;
    }

    void clear()
    {
        char* ptr = buffer.data();
        for(sizetype i = 0; i < allocatedObjects; i++)
        {
            const auto& alloc = allocations[i];
            alloc.destroy(static_cast<void*>(ptr));
            ptr += alloc.size;
            alloc.destroy = nullptr;
            alloc.size = 0;
        }
        allocatedObjects = allocatedBytes = 0;
        bytePointer = objectPointer = 0;
    }
};

void test()
{
    DynAllocStack<16, 2> a;
    a.
}