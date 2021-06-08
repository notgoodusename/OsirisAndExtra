#pragma once

#include "../Memory.h"

template <typename T>
void destruct(T* memory) noexcept
{
    memory->~T();
}

template <typename T>
T* construct(T* memory) noexcept
{
    return ::new(memory) T;
}

inline int utlMemoryCalcNewAllocationcount(int allocationcount, int growSize, int newSize, int bytesItem) noexcept
{
	if (growSize) {
		allocationcount = ((1 + ((newSize - 1) / growSize)) * growSize);
	}
	else {
		if (!allocationcount) {
			allocationcount = (31 + bytesItem) / bytesItem;
		}

		while (allocationcount < newSize) {
			allocationcount *= 2;
		}
	}

	return allocationcount;
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
