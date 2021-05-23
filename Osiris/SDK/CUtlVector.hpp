#pragma once
#include <cassert>

#include "../Memory.h"

inline int UtlMemory_CalcNewAllocationcount(int nAllocationcount, int nGrowSize, int nNewSize, int nBytesItem) {
	if (nGrowSize) {
		nAllocationcount = ((1 + ((nNewSize - 1) / nGrowSize)) * nGrowSize);
	}
	else {
		if (!nAllocationcount) {
			// Compute an allocation which is at least as big as a cache line...
			nAllocationcount = (31 + nBytesItem) / nBytesItem;
		}

		while (nAllocationcount < nNewSize) {
			nAllocationcount *= 2;
		}
	}

	return nAllocationcount;
}

template< class T, class I = int >
class CUtlMemory {
public:
	bool isValidIndex(I i) const {
		long x = i;
		return (x >= 0) && (x < AllocCount);
	}

	T& operator[](I i);
	const T& operator[](I i) const;

	T* base() {
		return memory;
	}

	int numAllocated() const {
		return AllocCount;
	}

	void grow(int num) {
		assert(num > 0);

		auto old_alloc_count = AllocCount;
		// Make sure we have at least numallocated + num allocations.
		// Use the grow rules specified for this memory (in growSize)
		int alloc_requested = AllocCount + num;

		int new_alloc_count = UtlMemory_CalcNewAllocationcount(AllocCount, growSize, alloc_requested, sizeof(T));

		// if m_alloc_requested wraps index type I, recalculate
		if ((int)(I)new_alloc_count < alloc_requested) {
			if ((int)(I)new_alloc_count == 0 && (int)(I)(new_alloc_count - 1) >= alloc_requested) {
				--new_alloc_count; // deal w/ the common case of AllocCount == MAX_USHORT + 1
			}
			else {
				if ((int)(I)alloc_requested != alloc_requested) {
					// we've been asked to grow memory to a size s.t. the index type can't address the requested amount of memory
					assert(0);
					return;
				}
				while ((int)(I)new_alloc_count < alloc_requested) {
					new_alloc_count = (new_alloc_count + alloc_requested) / 2;
				}
			}
		}

		AllocCount = new_alloc_count;

		if (memory) {
			auto ptr = new unsigned char[AllocCount * sizeof(T)];

			memcpy(ptr, memory, old_alloc_count * sizeof(T));
			memory = (T*)ptr;
		}
		else {
			memory = (T*)new unsigned char[AllocCount * sizeof(T)];
		}
	}

protected:
	T* memory;
	int AllocCount;
	int growSize;
};

template< class T, class I >
T& CUtlMemory< T, I >::operator[](I i) {
	assert(isValidIndex(i));
	return memory[i];
}

template< class T, class I >
const T& CUtlMemory< T, I >::operator[](I i) const {
	assert(isValidIndex(i));
	return memory[i];
}

template< class T >
void destruct(T* memory) {
	memory->~T();
}

template< class T >
T* construct(T* memory) {
	return ::new(memory) T;
}

template< class T >
T* copyConstruct(T* memory, T const& src) {
	return ::new(memory) T(src);
}

template< class T, class A = CUtlMemory< T > >
class CUtlVector {
	typedef A Allocator;

	typedef T* iterator;
	typedef const T* const_iterator;
public:
	T& operator[](int i);
	const T& operator[](int i) const;

	T& element(int i) {
		return memory[i];
	}

	const T& element(int i) const {
		assert(isValidIndex(i));
		return memory[i];
	}

	T* base() {
		return memory.base();
	}

	int count() const {
		return size;
	}

	void removeAll() {
		for (int i = size; --i >= 0; )
			destruct(&element(i));

		size = 0;
	}

	bool isValidIndex(int i) const {
		return (i >= 0) && (i < size);
	}

	void shiftElementsRight(int elem, int num = 1) {
		assert(isValidIndex(elem) || (size == 0) || (num == 0));
		int num_to_move = size - elem - num;
		if ((num_to_move > 0) && (num > 0))
			memmove(&element(elem + num), &element(elem), num_to_move * sizeof(T));
	}

	void shiftElementsLeft(int elem, int num = 1) {
		assert(isValidIndex(elem) || (size == 0) || (num == 0));
		int numToMove = size - elem - num;
		if ((numToMove > 0) && (num > 0))
			memmove(&element(elem), &element(elem + num), numToMove * sizeof(T));
	}

	void growVector(int num = 1) {
		if (size + num > memory.numAllocated()) {
			memory.grow(size + num - memory.numAllocated());
		}

		size += num;
	}

	int insertBefore(int elem) {
		// Can insert at the end
		assert((elem == count()) || isValidIndex(elem));

		growVector();
		shiftElementsRight(elem);
		construct(&element(elem));
		return elem;
	}

	int addToHead() {
		return insertBefore(0);
	}

	int addToTail() {
		return insertBefore(size);
	}

	int insertBefore(int elem, const T& src) {
		// Can't insert something that's in the list... reallocation may hose us
		assert((base() == NULL) || (&src < base()) || (&src >= (base() + count())));

		// Can insert at the end
		assert((elem == count()) || isValidIndex(elem));

		growVector();
		shiftElementsRight(elem);
		copyConstruct(&element(elem), src);
		return elem;
	}

	int addToTail(const T& src) {
		// Can't insert something that's in the list... reallocation may hose us
		return insertBefore(size, src);
	}

	int find(const T& src) const {
		for (int i = 0; i < count(); ++i) {
			if (element(i) == src)
				return i;
		}
		return -1;
	}

	void remove(int elem) {
		destruct(&element(elem));
		shiftElementsLeft(elem);
		--size;
	}

	bool findAndRemove(const T& src) {
		int elem = find(src);
		if (elem != -1) {
			remove(elem);
			return true;
		}
		return false;
	}

	iterator begin() {
		return base();
	}

	const_iterator begin() const {
		return base();
	}

	iterator end() {
		return base() + count();
	}

	const_iterator end() const {
		return base() + count();
	}

protected:
	Allocator memory;
	int size;
	T* elements;
};

template< typename T, class A >
T& CUtlVector< T, A >::operator[](int i) {
	assert(i < size);
	return memory[i];
}

template< typename T, class A >
const T& CUtlVector< T, A >::operator[](int i) const {
	assert(i < size);
	return memory[i];
}