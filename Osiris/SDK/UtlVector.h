#pragma once

#include "../Memory.h"

template <typename T>
void destruct(T* memory) {
    memory->~T();
}

template <typename T>
T* construct(T* memory) {
    return ::new(memory) T;
}

inline int utlMemoryCalcNewAllocationcount(int nAllocationcount, int nGrowSize, int nNewSize, int nBytesItem) 
{
	if (nGrowSize) {
		nAllocationcount = ((1 + ((nNewSize - 1) / nGrowSize)) * nGrowSize);
	}
	else {
		if (!nAllocationcount) {
			nAllocationcount = (31 + nBytesItem) / nBytesItem;
		}

		while (nAllocationcount < nNewSize) {
			nAllocationcount *= 2;
		}
	}

	return nAllocationcount;
}

template <typename T>
class UtlVector {
public:
    constexpr T& operator[](int i) noexcept { return memory[i]; };
    constexpr const T& operator[](int i) const noexcept { return memory[i]; };

    T* memory;
    int allocationCount;
    int growSize;
    int size;
    T* elements;

	void grow(int num) noexcept
	{
		auto oldAllocationCount = allocationCount;

		int allocationRequested = allocationCount + num;

		int newAllocationCount = utlMemoryCalcNewAllocationcount(allocationCount, growSize, allocationRequested, sizeof(T));

		if ((int)(int)newAllocationCount < allocationRequested) {
			if ((int)(int)newAllocationCount == 0 && (int)(int)(newAllocationCount - 1) >= allocationRequested) {
				--newAllocationCount;
			}
			else {
				if ((int)(int)allocationRequested != allocationRequested) {
					return;
				}
				while ((int)(int)newAllocationCount < allocationRequested) {
					newAllocationCount = (newAllocationCount + allocationRequested) / 2;
				}
			}
		}

		allocationCount = newAllocationCount;

		if (memory) {
			auto ptr = new unsigned char[allocationCount * sizeof(T)];

			std::memcpy(ptr, memory, oldAllocationCount * sizeof(T));
			memory = (T*)ptr;
		}
		else {
			memory = (T*)new unsigned char[allocationCount * sizeof(T)];
		}
	}

    void removeAll() noexcept
    {
        for (int i = size; --i >= 0; )
            destruct(&memory[i]);

        size = 0;
    }

    void growVector(int num = 1) noexcept
    {
        if (size + num > allocationCount) {
            grow(size + num - allocationCount);
        }

        size += num;
    }
};
