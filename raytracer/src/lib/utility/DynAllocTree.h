#pragma once

#include <array>
#include <assert.h>
#include <cmath>

template<size_t MaxObjectSize, size_t MaxTreeDepth, size_t MaxBranchingFactor, typename PointerType = unsigned int>
class DynAllocTree {
#pragma pack(push, 1)
    struct EntryHeader
    {
        bool occupied = false;
        PointerType parent;
        //std::type_info type;
        void(*destroy)(const void*) = nullptr;
    };
#pragma pack(pop)

    static constexpr size_t pow(size_t x, size_t y) noexcept
    {
        return y == 0 ? 1 : x * pow(x, y-1);
    }

    static constexpr size_t NbElements = pow(MaxBranchingFactor, MaxTreeDepth) - 1;
    static constexpr size_t ElementByteSize = sizeof(EntryHeader) + MaxObjectSize;
    static constexpr size_t BufferLength = NbElements * ElementByteSize;

    PointerType index = 0;
    PointerType levelsDeep = 0;
    std::array<char, BufferLength> buffer;

public:

    DynAllocTree() : buffer()
    {
        static_assert(NbElements - 1 <= std::numeric_limits<PointerType>().max());
    }

    ~DynAllocTree()
    {
        clear();
    }

    template<typename T, typename ... Args>
    T* alloc(Args&& ... args)
    {
        static_assert(sizeof(T) <= MaxObjectSize, "Type T is too large to fit in the tree node. Decrease the type size or increase the max tree object size.");

        auto pointer = index * ElementByteSize;

        auto* header = reinterpret_cast<EntryHeader*>(&buffer[pointer]);
        if(header->occupied)
        {
            header->destroy(reinterpret_cast<void*>(buffer.data() + pointer));
        }

        header->occupied = true;
        //header->type = typeid(T);
        header->destroy = [](const void* x) { static_cast<const T*>(x)->~T(); };
        return new(&buffer[pointer+sizeof(EntryHeader)]) T(std::forward<Args>(args)...);
    }

    void dealloc()
    {
        auto pointer = index * ElementByteSize;

        auto* header = reinterpret_cast<EntryHeader*>(&buffer[pointer]);
        if(header->occupied)
        {
            header->destroy(reinterpret_cast<void*>(buffer.data() + pointer));
        }
    }

    template<typename T>
    T* tryRead()
    {
        auto pointer = index * ElementByteSize;

        auto* header = reinterpret_cast<EntryHeader*>(&buffer[pointer]);
        if(!header->occupied /*|| header->type != typeid(T)*/)
        {
            return nullptr;
        }

        return reinterpret_cast<T*>(&buffer[pointer+sizeof(EntryHeader)]);
    }

    template <typename T, typename ... Args>
    T* readOrAlloc(Args&& ... args)
    {
        if(hasValue<T>())
        {
            return tryRead<T>();
        }
        else
        {
            return alloc<T>();
        }
    }

    template<typename T>
    bool hasValue()
    {
        auto pointer = index * ElementByteSize;
        auto* header = reinterpret_cast<EntryHeader*>(&buffer[pointer]);
        return header->occupied /*&& header->type == typeid(T)*/;
    }

    void branch(size_t childIndex)
    {
        assert(childIndex < MaxBranchingFactor);
        assert(levelsDeep+1 < MaxTreeDepth);

        auto parent = index;
        levelsDeep++;
        auto childSize = std::pow(MaxBranchingFactor, MaxTreeDepth - levelsDeep) - 1;
        index += 1 + (childIndex * childSize);
        auto pointer = index * ElementByteSize;
        auto* header = reinterpret_cast<EntryHeader*>(&buffer[pointer]);
        header->parent = parent;
    }

    void up()
    {
        auto pointer = index * ElementByteSize;
        auto* header = reinterpret_cast<EntryHeader*>(&buffer[pointer]);
        index = header->parent;
        levelsDeep--;
    }

    void resetPointer()
    {
        index = 0;
        levelsDeep = 0;
    }

    void clear()
    {
        size_t pointer = 0;
        for(PointerType i = 0; i < NbElements; i++)
        {
            auto* header = reinterpret_cast<EntryHeader*>(&buffer[pointer]);
            if(header->occupied)
            {
                header->destroy(static_cast<void*>(buffer.data() + pointer));
                header->destroy = nullptr;
                header->occupied = false;
            }
            pointer += ElementByteSize;
        }
    }
};
