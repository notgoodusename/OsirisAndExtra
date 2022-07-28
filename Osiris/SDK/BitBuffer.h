#pragma once

#include "Vector.h"

using uint = unsigned int;
using uint8 = std::uint8_t;
using int16 = std::int16_t;
using uint16 = std::uint16_t;
using uint32 = unsigned long;
using int32 = std::int32_t;
using uint64 = std::uint64_t;
using int64 = std::int64_t;


#define	COORD_INTEGER_BITS			14
#define COORD_FRACTIONAL_BITS		5
#define COORD_DENOMINATOR			(1<<(COORD_FRACTIONAL_BITS))
#define COORD_RESOLUTION			(1.0f/(COORD_DENOMINATOR))

#define COORD_INTEGER_BITS_MP		11
#define COORD_FRACTIONAL_BITS_MP_LOWPRECISION 3
#define COORD_DENOMINATOR_LOWPRECISION			(1<<(COORD_FRACTIONAL_BITS_MP_LOWPRECISION))
#define COORD_RESOLUTION_LOWPRECISION			(1.0f/(COORD_DENOMINATOR_LOWPRECISION))

#define NORMAL_FRACTIONAL_BITS		11
#define NORMAL_DENOMINATOR			( (1<<(NORMAL_FRACTIONAL_BITS)) - 1 )
#define NORMAL_RESOLUTION			(1.0f/(NORMAL_DENOMINATOR))

#define maxVarintBytes 10
#define maxVarint32Bytes 5

#define Bits2Bytes(b) ((b+7) >> 3)
#define Bytes2Bits(b) (b << 3)

typedef unsigned char BYTE;

inline void Q_memcpy(void* dest, void* src, int count) // Quake Memcpy
{
	int             i;

	if ((((long)dest | (long)src | count) & 3) == 0)
	{
		count >>= 2;
		for (i = 0; i < count; i++) {
			((int*)dest)[i] = ((int*)src)[i];
		}
	}
	else {
		for (i = 0; i < count; i++) {
			(reinterpret_cast<BYTE*>(dest))[i] = (reinterpret_cast<BYTE*>(src))[i]; // C style cast crashes compiler... but reinterpret_cast doesn't? What MSCV?????
		}
	}
}

template <typename T>
inline T WordSwapC(T w)
{
	unsigned __int16 temp;

	temp = ((*((uint16*)&w) & 0xff00) >> 8);
	temp |= ((*((uint16*)&w) & 0x00ff) << 8);

	return *((T*)&temp);
}

template <typename T>
inline T DWordSwapC(T dw)
{
	uint32 temp;

	temp = *((uint32*)&dw) >> 24;
	temp |= ((*((uint32*)&dw) & 0x00FF0000) >> 8);
	temp |= ((*((uint32*)&dw) & 0x0000FF00) << 8);
	temp |= ((*((uint32*)&dw) & 0x000000FF) << 24);

	return *((T*)&temp);
}

template <typename T>
inline T QWordSwapC(T dw)
{
	uint64 temp;

	temp = *((uint64*)&dw) >> 56;
	temp |= ((*((uint64*)&dw) & 0x00FF000000000000ull) >> 40);
	temp |= ((*((uint64*)&dw) & 0x0000FF0000000000ull) >> 24);
	temp |= ((*((uint64*)&dw) & 0x000000FF00000000ull) >> 8);
	temp |= ((*((uint64*)&dw) & 0x00000000FF000000ull) << 8);
	temp |= ((*((uint64*)&dw) & 0x0000000000FF0000ull) << 24);
	temp |= ((*((uint64*)&dw) & 0x000000000000FF00ull) << 40);
	temp |= ((*((uint64*)&dw) & 0x00000000000000FFull) << 56);

	return *((T*)&temp);
}

#define WordSwap  WordSwapC
#define DWordSwap DWordSwapC
#define QWordSwap QWordSwapC

#define SafeSwapFloat( out, pIn )	(*((unsigned in*)out) = DWordSwap( *((unsigned in*)pIn) ))

#define BigShort( value )				WordSwap( value )
#define BigWord( value )				WordSwap( value )
#define BigLong( value )				DWordSwapC( value )
#define BigDWord( value )				DWordSwap( value )
#define BigQWord( value )				QWordSwap( value ) 
#define LittleShort( value )			( value )
#define LittleWord( value )			( value )
#define LittleLong( value )			( value )
#define LittleDWord( value )			( value )
#define LittleQWord( value )			( value )

#define SwapShort( value )			BigShort( value )
#define SwapWord( value )				BigWord( value )
#define SwapLong( value )				BigLong( value )
#define SwapDWord( value )			BigDWord( value )

// Pass floats by pointer for swaping to avoid truncation in the fpu
#define BigFloat( out, in )		SafeSwapFloat( out, in )
#define LittleFloat( out, in )	( *out = *in )
#define SwapFloat( out, in )		BigFloat( out, in )

inline uint32 loadLittleDWord(uint32* base, unsigned int dwordIndex)
{
	return LittleDWord(base[dwordIndex]);
}

inline void storeLittleDWord(uint32* base, unsigned int dwordIndex, uint32 dword)
{
	base[dwordIndex] = LittleDWord(dword);
}

inline uint32 zigZagEncode32(int32 n) noexcept
{
	// Note:  the right-shift must be arithmetic
	return(n << 1) ^ (n >> 31);
}

inline int32 zigZagDecode32(uint32 n) noexcept
{
	return(n >> 1) ^ -static_cast<int32>(n & 1);
}

inline uint64 zigZagEncode64(uint64 n) noexcept
{
	// Note:  the right-shift must be arithmetic
	return(n << 1) ^ (n >> 63);
}

inline __int64 zigZagDecode64(uint64 n) noexcept
{
	return(n >> 1) ^ -static_cast<__int64>(n & 1);
}

inline int bitByte(int bits) noexcept
{
	return (bits + 7) >> 3;
}

inline int bitForBitnum(int bitnum) noexcept
{
	return 1;
	//return getBitForBitnum(bitnum);
}

class bufferWrite
{
public:
	bufferWrite() noexcept;
	bufferWrite(void* data, int bytes, int maxBits = -1) noexcept;
	bufferWrite(const char* debugName, void* data, int bytes, int maxBits = -1) noexcept;

	void startWriting(void* data, int bytes, int startBit = 0, int bits = -1) noexcept;

	void reset() noexcept;

	unsigned char* getBasePointer() noexcept { return (unsigned char*)data; }

	void setAssertOnOverflow(bool assert) noexcept;

	const char* getDebugName() noexcept;
	void setDebugName(const char* debugName) noexcept;

	void seekToBit(int bitPos) noexcept;


	// Bit functions.
	void writeOneBit(int value) noexcept;
	void writeOneBitNoCheck(int value) noexcept;
	void writeOneBitAt(int bit, int value) noexcept;

	void writeUBitLong(unsigned int data, int numbits, bool checkRange = true) noexcept;
	void writeSBitLong(int data, int numbits) noexcept;


	void writeBitLong(uint data, int numbits, bool _signed) noexcept;


	bool writeBits(const void* in, int bits) noexcept;


	void writeUBitVar(unsigned int data) noexcept;

	void writeVarInt32(uint32 data) noexcept;
	void writeVarInt64(uint64 data) noexcept;
	void writeSignedVarInt32(int32 data) noexcept;
	void writeSignedVarInt64(int64 data) noexcept;
	int byteSizeVarInt32(uint32 data) noexcept;
	int byteSizeVarInt64(uint64 data) noexcept;
	int byteSizeSignedVarInt32(int32 data) noexcept;
	int byteSizeSignedVarInt64(int64 data) noexcept;


	bool writeBitsFromBuffer(class bufferRead* in, int bits) noexcept;

	void writeBitAngle(float angle, int numbits) noexcept;
	void writeBitCoord(const float f) noexcept;
	void writeBitCoordMP(const float f, bool integral, bool lowPrecision) noexcept;
	void writeBitFloat(float value) noexcept;
	void writeBitVec3Coord(const Vector& fa) noexcept;
	void writeBitNormal(float f) noexcept;
	void writeBitVec3Normal(const Vector& fa) noexcept;
	void writeBitAngles(const Vector& fa) noexcept;

	// Byte functions.
	void writeChar(int value) noexcept;
	void writeByte(int value) noexcept;
	void writeShort(int value) noexcept;
	void writeWord(int value) noexcept;
	void writeLong(long value) noexcept;
	void writeLongLong(int64 value) noexcept;
	void writeFloat(float value) noexcept;
	bool writeBytes(const void* buffer, int bytes) noexcept;

	// Returns false if it overflows the buffer.
	bool writeString(const char* string) noexcept;


	// Status.
	// How many bytes are filled in?
	int getNumBytesWritten() const noexcept;
	int getNumBitsWritten() const noexcept;
	int getMaxNumBits() noexcept;
	int getNumBitsLeft() noexcept;
	int getNumBytesLeft() noexcept;
	unsigned char* getData() noexcept;
	const unsigned char* getData() const noexcept;

	// Has the buffer overflowed?
	bool checkForOverflow(int bits) noexcept;
	inline bool isOverflowed() const noexcept { return overflow; }

	__forceinline void setOverflowFlag() { overflow = true; }

	unsigned long* data;
	int dataBytes;
	int dataBits;
	int curBit;

private:
	bool overflow;
	bool assertOnOverflow;
	const char* debugName;
};

__forceinline void bufferWrite::writeOneBitNoCheck(int value) noexcept
{
	if (value)
		data[curBit >> 3] |= (1 << (curBit & 7));
	else
		data[curBit >> 3] &= ~(1 << (curBit & 7));

	++curBit;
}

inline void	bufferWrite::writeOneBitAt(int bit, int value) noexcept
{
	if (bit >= dataBits)
	{
		setOverflowFlag();
		return;
	}

	if (value)
		data[bit >> 5] |= 1u << (bit & 31);
	else
		data[bit >> 5] &= ~(1u << (bit & 31));
}

__forceinline void bufferWrite::writeUBitLong(unsigned int curData, int numbits, bool checkRange) __restrict noexcept
{
	if (getNumBitsLeft() < numbits)
	{
		curBit = dataBits;
		return;
	}

	int curBitMasked = curBit & 31;
	int dWord = curBit >> 5;
	curBit += numbits;

	// Mask in a dword.

	uint32* __restrict out = &data[dWord];

	// Rotate data into dword alignment
	curData = (curData << curBitMasked) | (curData >> (32 - curBitMasked));

	// Calculate bitmasks for first and second word
	unsigned int temp = 1 << (numbits - 1);
	unsigned int mask1 = (temp * 2 - 1) << curBitMasked;
	unsigned int mask2 = (temp - 1) >> (31 - curBitMasked);

	// Only look beyond current word if necessary (avoid access violation)
	int i = mask2 & 1;
	uint32 dword1 = loadLittleDWord(out, 0);
	uint32 dword2 = loadLittleDWord(out, i);

	// Drop bits into place
	dword1 ^= (mask1 & (curData ^ dword1));
	dword2 ^= (mask2 & (curData ^ dword2));

	// Note reversed order of writes so that dword1 wins if mask2 == 0 && i == 0
	storeLittleDWord(out, i, dword2);
	storeLittleDWord(out, 0, dword1);
}

// writes an unsigned integer with variable bit length
__forceinline void bufferWrite::writeUBitVar(unsigned int data) noexcept
{
	int n = (data < 0x10u ? -1 : 0) + (data < 0x100u ? -1 : 0) + (data < 0x1000u ? -1 : 0);
	writeUBitLong(data * 4 + n + 3, 6 + n * 4 + 12);
	if (data >= 0x1000u)
		writeUBitLong(data >> 16, 16);
}

__forceinline void bufferWrite::writeBitFloat(float value) noexcept
{
	long intValue;
	intValue = *((long*)&value);
	writeUBitLong(intValue, 32);
}

inline int bufferWrite::getNumBytesWritten() const noexcept
{
	return bitByte(curBit);
}

inline int bufferWrite::getNumBitsWritten() const noexcept
{
	return curBit;
}

inline int bufferWrite::getMaxNumBits() noexcept
{
	return dataBits;
}

inline int bufferWrite::getNumBitsLeft() noexcept
{
	return dataBits - curBit;
}

inline int bufferWrite::getNumBytesLeft() noexcept
{
	return getNumBitsLeft() >> 3;
}

inline unsigned char* bufferWrite::getData() noexcept
{
	return (unsigned char*)data;
}

inline const unsigned char* bufferWrite::getData() const noexcept
{
	return (unsigned char*)data;
}

__forceinline bool bufferWrite::checkForOverflow(int bits) noexcept
{
	if (curBit + bits > dataBits)
		setOverflowFlag();

	return overflow;
}

class bufferRead
{
public:
	bufferRead() noexcept;
	bufferRead(const void* data, int bytes, int bits = -1) noexcept;
	bufferRead(const char* debugName, const void* data, int bytes, int bits = -1) noexcept;


	void startReading(const void* data, int bytes, int startBit = 0, int bits = -1) noexcept;
	void reset() noexcept;

	void setAssertOnOverflow(bool assert) noexcept;

	const char* getDebugName() const noexcept { return debugName; }
	void setDebugName(const char* name) noexcept;

	void exciseBits(int startbit, int bitstoremove) noexcept;

	// Bit functions.
	int readOneBit() noexcept;

	unsigned int checkReadUBitLong(int numbits) noexcept;
	int readOneBitNoCheck() noexcept;
	bool checkForOverflow(int bits) noexcept;

	const unsigned char* getBasePointer() noexcept { return data; }

	__forceinline int totalBytesAvailable() const noexcept { return dataBytes; }

	// Read a list of bits in.
	void readBits(void* out, int bits) noexcept;
	int readBitsClamped_ptr(void* out, size_t outSizeBytes, size_t bits) noexcept;

	template <typename T, size_t N>
	int readBitsClamped(T(&out)[N], size_t bits) noexcept { return readBitsClamped_ptr(out, N * sizeof(T), bits); }

	float readBitAngle(int numbits) noexcept;

	unsigned int readUBitLong(int numbits) __restrict noexcept;
	unsigned int readUBitLongNoInline(int numbits) __restrict noexcept;
	unsigned int peekUBitLong(int numbits) noexcept;
	int readSBitLong(int numbits) noexcept;

	// reads an unsigned integer with variable bit length
	unsigned int readUBitVar() noexcept;
	unsigned int readUBitVarInternal(int encodingType) noexcept;

	// reads a varint encoded integer
	uint32 readVarInt32() noexcept;
	uint64 readVarInt64() noexcept;
	int32 readSignedVarInt32() noexcept;
	__int64 readSignedVarInt64() noexcept;

	// You can read signed or unsigned data with this, just cast to a signed int if necessary.
	unsigned int readBitLong(int numbits, bool _signed) noexcept;

	float readBitCoord() noexcept;
	float readBitCoordMP(bool integral, bool lowPrecision) noexcept;
	float readBitFloat() noexcept;
	float readBitNormal() noexcept;
	void readBitVec3Coord(Vector& fa) noexcept;
	void readBitVec3Normal(Vector& fa) noexcept;
	void readBitAngles(Vector& fa) noexcept;

	// Faster for comparisons but do not fully decode float values
	unsigned int readBitCoordBits() noexcept;
	unsigned int readBitCoordMPBits(bool integral, bool lowPrecision) noexcept;

	// Byte functions (these still read data in bit-by-bit).
	__forceinline int readChar() noexcept { return (char)readUBitLong(8); }
	__forceinline int readByte() noexcept { return readUBitLong(8); }
	__forceinline int readShort() noexcept { return (short)readUBitLong(16); }
	__forceinline int readWord() noexcept { return readUBitLong(16); }
	__forceinline long readLong() noexcept { return readUBitLong(32); }
	__int64 readLongLong() noexcept;
	float readFloat() noexcept;
	bool readBytes(void* iut, int bytes) noexcept;
	bool readString(char* string, int bufferLength, bool line = false, int* outNumChars = NULL) noexcept;

	char* readAndAllocateString(bool* overflow = 0) noexcept;

	int compareBits(bufferRead* __restrict other, int bits) __restrict noexcept;

	// Status.
	int getNumBytesLeft() noexcept;
	int getNumBytesRead() noexcept;
	int getNumBitsLeft() noexcept;
	int getNumBitsRead() const noexcept;

	inline bool isOverflowed() const { return overflow; }

	inline bool seek(int bit) noexcept;
	inline bool seekRelative(int bitDelta) noexcept;

	void setOverflowFlag() noexcept;

	unsigned char* __restrict data;
	uint32 dataBytes;
	uint32 dataBits;
	uint32 curBit;

private:
	bool overflow;
	bool assertOnOverflow;
	const char* debugName;
};

inline int bufferRead::getNumBytesRead() noexcept
{
	return bitByte(curBit);
}

inline int bufferRead::getNumBitsLeft() noexcept
{
	return dataBits - curBit;
}

inline int bufferRead::getNumBytesLeft() noexcept
{
	return getNumBitsLeft() >> 3;
}

inline int bufferRead::getNumBitsRead() const noexcept
{
	return curBit;
}

inline bool bufferRead::seek(int bit) noexcept
{
	if (bit < 0 || bit > static_cast<int>(dataBits))
	{
		setOverflowFlag();
		curBit = dataBits;
		return false;
	}
	else
	{
		curBit = bit;
		return true;
	}
}

// Seek to an offset from the current position.
inline bool	bufferRead::seekRelative(int bitDelta) noexcept
{
	return seek(curBit + bitDelta);
}

inline bool bufferRead::checkForOverflow(int bits) noexcept
{
	if (curBit + bits > dataBits)
		setOverflowFlag();

	return overflow;
}

inline int bufferRead::readOneBitNoCheck() noexcept
{
	unsigned char value = data[curBit >> 3] >> (curBit & 7);
	++curBit;
	return value & 1;
}

inline int bufferRead::readOneBit() noexcept
{
	if (getNumBitsLeft() <= 0)
	{
		setOverflowFlag();
		return 0;
	}
	return readOneBitNoCheck();
}

inline float bufferRead::readBitFloat() noexcept
{
	union { uint32 u; float f; } c = { readUBitLong(32) };
	return c.f;
}

__forceinline unsigned int bufferRead::readUBitVar() noexcept
{
	// six bits: low 2 bits for encoding + first 4 bits of value
	unsigned int sixbits = readUBitLong(6);
	unsigned int encoding = sixbits & 3;
	if (encoding)
	{
		// this function will seek back four bits and read the full value
		return readUBitVarInternal(encoding);
	}
	return sixbits >> 2;
}

__forceinline unsigned int bufferRead::readUBitLong(int numbits) __restrict noexcept
{
	if (getNumBitsLeft() < numbits)
	{
		curBit = dataBits;
		setOverflowFlag();
		return 0;
	}

	unsigned int iStartBit = curBit & 31u;
	int iLastBit = curBit + numbits - 1;
	unsigned int iWordOffset1 = curBit >> 5;
	unsigned int iWordOffset2 = iLastBit >> 5;
	curBit += numbits;

	extern uint32 extraMasks[33];
	unsigned int bitmask = extraMasks[numbits];

	unsigned int dw1 = loadLittleDWord((uint32* __restrict)data, iWordOffset1) >> iStartBit;
	unsigned int dw2 = loadLittleDWord((uint32* __restrict)data, iWordOffset2) << (32 - iStartBit);

	return (dw1 | dw2) & bitmask;
}

__forceinline int bufferRead::compareBits(bufferRead* __restrict other, int numbits) __restrict noexcept
{
	return (readUBitLong(numbits) != other->readUBitLong(numbits));
}