#include "BitBuffer.h"

unsigned long littleBits[32];
unsigned long bitWriteMasks[32][33];
unsigned long extraMasks[33];

bufferWrite::bufferWrite() noexcept
{
	data = NULL;
	dataBytes = 0;
	dataBits = -1;
	curBit = 0;
	overflow = false;
	assertOnOverflow = true;
	debugName = NULL;
}

bufferWrite::bufferWrite(const char* debugName, void* data, int bytes, int bits) noexcept
{
	assertOnOverflow = true;
	this->debugName = debugName;
	startWriting(data, bytes, 0, bits);
}

bufferWrite::bufferWrite(void* data, int bytes, int maxBits) noexcept
{
	assertOnOverflow = true;
	debugName = NULL;
	startWriting(data, bytes, 0, maxBits);
}

void bufferWrite::startWriting(void* data, int bytes, int startBit, int maxBits) noexcept
{
	bytes &= ~3;

	this->data = (unsigned long*)data;
	dataBytes = bytes;

	if (maxBits == -1)
		dataBits = bytes << 3;
	else
		dataBits = maxBits;

	curBit = startBit;
	overflow = false;
}

void bufferWrite::reset() noexcept
{
	curBit = 0;
	overflow = false;
}

void bufferWrite::setAssertOnOverflow(bool assert) noexcept
{
	assertOnOverflow = assert;
}

const char* bufferWrite::getDebugName() noexcept
{
	return debugName;
}

void bufferWrite::setDebugName(const char* debugName) noexcept
{
	this->debugName = debugName;
}

void bufferWrite::seekToBit(int bitPos) noexcept
{
	curBit = bitPos;
}

void bufferWrite::writeOneBit(int value) noexcept
{
	if (curBit >= dataBits)
		return;
	writeOneBitNoCheck(value);
}

void bufferWrite::writeSBitLong(int data, int numbits) noexcept
{
	// Force the sign-extension bit to be correct even in the case of overflow.
	int value = data;
	int preserveBits = (0x7FFFFFFF >> (32 - numbits));
	int signExtension = (value >> 31) & ~preserveBits;
	value &= preserveBits;
	value |= signExtension;

	writeUBitLong(value, numbits, false);
}

void bufferWrite::writeBitLong(uint data, int numbits, bool _signed) noexcept
{
	if (_signed)
		writeSBitLong(static_cast<int>(data), numbits);
	else
		writeUBitLong(data, numbits);
}

bool bufferWrite::writeBits(const void* inData, int bits) noexcept
{
	unsigned char* out = (unsigned char*)inData;
	int bitsLeft = bits;

	if ((curBit + bits) > dataBits)
		return false;

	// Align output to dword boundary
	while (((unsigned long)out & 3) != 0 && bitsLeft >= 8)
	{

		writeUBitLong(*out, 8, false);
		++out;
		bitsLeft -= 8;
	}

	if ((bitsLeft >= 32) && (curBit & 7) == 0)
	{
		// current bit is byte aligned, do block copy
		int numbytes = bitsLeft >> 3;
		int numbits = numbytes << 3;

		Q_memcpy((char*)data + (curBit >> 3), out, numbytes);
		out += numbytes;
		bitsLeft -= numbits;
		curBit += numbits;
	}

	if (bitsLeft >= 32)
	{
		unsigned long bitsRight = (curBit & 31);
		unsigned long bitsLeft = 32 - bitsRight;
		extern uint32 bitWriteMasks[32][33];
		unsigned long bitMaskLeft = bitWriteMasks[bitsRight][32];
		unsigned long bitMaskRight = bitWriteMasks[0][bitsRight];

		unsigned long* data = &this->data[curBit >> 5];

		// Read dwords.
		while (bitsLeft >= 32)
		{
			unsigned long curData = *(unsigned long*)out;
			out += sizeof(unsigned long);

			*data &= bitMaskLeft;
			*data |= curData << bitsRight;

			data++;

			if (bitsLeft < 32)
			{
				curData >>= bitsLeft;
				*data &= bitMaskRight;
				*data |= curData;
			}

			bitsLeft -= 32;
			curBit += 32;
		}
	}


	// write remaining bytes
	while (bitsLeft >= 8)
	{
		writeUBitLong(*out, 8, false);
		++out;
		bitsLeft -= 8;
	}

	// write remaining bits
	if (bitsLeft)
		writeUBitLong(*out, bitsLeft, false);

	return !isOverflowed();
}

void bufferWrite::writeVarInt32(uint32 data) noexcept
{
	// Check if align and we have room, slow path if not
	if ((curBit & 7) == 0 && (curBit + maxVarint32Bytes * 8) <= dataBits)
	{
		uint8* target = ((uint8*)data) + (curBit >> 3);

		target[0] = static_cast<uint8>(data | 0x80);
		if (data >= (1 << 7))
		{
			target[1] = static_cast<uint8>((data >> 7) | 0x80);
			if (data >= (1 << 14))
			{
				target[2] = static_cast<uint8>((data >> 14) | 0x80);
				if (data >= (1 << 21))
				{
					target[3] = static_cast<uint8>((data >> 21) | 0x80);
					if (data >= (1 << 28))
					{
						target[4] = static_cast<uint8>(data >> 28);
						curBit += 5 * 8;
						return;
					}
					else
					{
						target[3] &= 0x7F;
						curBit += 4 * 8;
						return;
					}
				}
				else
				{
					target[2] &= 0x7F;
					curBit += 3 * 8;
					return;
				}
			}
			else
			{
				target[1] &= 0x7F;
				curBit += 2 * 8;
				return;
			}
		}
		else
		{
			target[0] &= 0x7F;
			curBit += 1 * 8;
			return;
		}
	}
	else // Slow path
	{
		while (data > 0x7F)
		{
			writeUBitLong((data & 0x7F) | 0x80, 8);
			data >>= 7;
		}
		writeUBitLong(data & 0x7F, 8);
	}
}

void bufferWrite::writeVarInt64(uint64 data) noexcept
{
	if ((curBit & 7) == 0 && (curBit + maxVarintBytes * 8) <= dataBits)
	{
		uint8* target = ((uint8*)data) + (curBit >> 3);

		// Splitting into 32-bit pieces gives better performance on 32-bit
		// processors.
		uint32 part0 = static_cast<uint32>(data);
		uint32 part1 = static_cast<uint32>(data >> 28);
		uint32 part2 = static_cast<uint32>(data >> 56);

		int size;

		// Here we can't really optimize for small numbers, since the data is
		// split into three parts.  Cheking for numbers < 128, for instance,
		// would require three comparisons, since you'd have to make sure part1
		// and part2 are zero.  However, if the caller is using 64-bit integers,
		// it is likely that they expect the numbers to often be very large, so
		// we probably don't want to optimize for small numbers anyway.  Thus,
		// we end up with a hardcoded binary search tree...
		if (part2 == 0)
		{
			if (part1 == 0)
			{
				if (part0 < (1 << 14))
				{
					if (part0 < (1 << 7))
					{
						size = 1; goto size1;
					}
					else
					{
						size = 2; goto size2;
					}
				}
				else
				{
					if (part0 < (1 << 21))
					{
						size = 3; goto size3;
					}
					else
					{
						size = 4; goto size4;
					}
				}
			}
			else
			{
				if (part1 < (1 << 14))
				{
					if (part1 < (1 << 7))
					{
						size = 5; goto size5;
					}
					else
					{
						size = 6; goto size6;
					}
				}
				else
				{
					if (part1 < (1 << 21))
					{
						size = 7; goto size7;
					}
					else
					{
						size = 8; goto size8;
					}
				}
			}
		}
		else
		{
			if (part2 < (1 << 7))
			{
				size = 9; goto size9;
			}
			else
			{
				size = 10; goto size10;
			}
		}

		//AssertFatalMsg(false, "Can't get here.");

	size10: target[9] = static_cast<uint8>((part2 >> 7) | 0x80);
	size9: target[8] = static_cast<uint8>((part2) | 0x80);
	size8: target[7] = static_cast<uint8>((part1 >> 21) | 0x80);
	size7: target[6] = static_cast<uint8>((part1 >> 14) | 0x80);
	size6: target[5] = static_cast<uint8>((part1 >> 7) | 0x80);
	size5: target[4] = static_cast<uint8>((part1) | 0x80);
	size4: target[3] = static_cast<uint8>((part0 >> 21) | 0x80);
	size3: target[2] = static_cast<uint8>((part0 >> 14) | 0x80);
	size2: target[1] = static_cast<uint8>((part0 >> 7) | 0x80);
	size1: target[0] = static_cast<uint8>((part0) | 0x80);

		target[size - 1] &= 0x7F;
		curBit += size * 8;
	}
	else // slow path
	{
		while (data > 0x7F)
		{
			writeUBitLong((data & 0x7F) | 0x80, 8);
			data >>= 7;
		}
		writeUBitLong(data & 0x7F, 8);
	}
}

void bufferWrite::writeSignedVarInt32(int32 data) noexcept
{
	writeVarInt32(zigZagEncode32(data));
}

void bufferWrite::writeSignedVarInt64(int64 data) noexcept
{
	writeVarInt64(zigZagEncode64(data));
}

int bufferWrite::byteSizeVarInt32(uint32 data) noexcept
{
	int size = 1;
	while (data > 0x7F) {
		size++;
		data >>= 7;
	}
	return size;
}

int	bufferWrite::byteSizeVarInt64(uint64 data) noexcept
{
	int size = 1;
	while (data > 0x7F) {
		size++;
		data >>= 7;
	}
	return size;
}

int bufferWrite::byteSizeSignedVarInt32(int32 data) noexcept
{
	return byteSizeVarInt32(zigZagEncode32(data));
}

int bufferWrite::byteSizeSignedVarInt64(int64 data) noexcept
{
	return byteSizeVarInt64(zigZagEncode64(data));
}

bool bufferWrite::writeBitsFromBuffer(bufferRead* in, int bits) noexcept
{
	while (bits > 32)
	{
		writeUBitLong(in->readUBitLong(32), 32);
		bits -= 32;
	}

	writeUBitLong(in->readUBitLong(bits), bits);
	return !isOverflowed() && !in->isOverflowed();
}

void bufferWrite::writeBitAngle(float angle, int numbits) noexcept
{
	int d;
	uint mask;
	uint shift;

	shift = bitForBitnum(numbits);
	mask = shift - 1;

	d = static_cast<int>((angle / 360.0) * shift);
	d &= mask;

	writeUBitLong(static_cast<uint>(d), numbits);
}

void bufferWrite::writeBitCoord(const float f) noexcept
{
	int signbit = (f <= -COORD_RESOLUTION);
	int intval = static_cast<int>(abs(f));
	int fractval = abs(static_cast<int>((f * COORD_DENOMINATOR))) & (COORD_DENOMINATOR - 1);


	// Send the bit flags that indicate whether we have an integer part and/or a fraction part.
	writeOneBit(intval);
	writeOneBit(fractval);

	if (intval || fractval)
	{
		// Send the sign bit
		writeOneBit(signbit);

		// Send the integer if we have one.
		if (intval)
		{
			// Adjust the integers from [1..MAX_COORD_VALUE] to [0..MAX_COORD_VALUE-1]
			intval--;
			writeUBitLong(static_cast<uint>(intval), COORD_INTEGER_BITS);
		}

		// Send the fraction if we have one
		if (fractval)
		{
			writeUBitLong(static_cast<uint>(fractval), COORD_FRACTIONAL_BITS);
		}
	}
}

void bufferWrite::writeBitCoordMP(const float f, bool integral, bool lowPrecision) noexcept
{
	int signbit = (f <= -(lowPrecision ? COORD_RESOLUTION_LOWPRECISION : COORD_RESOLUTION));
	int intval = static_cast<int>(abs(f));
	int fractval = lowPrecision ?
		(abs(static_cast<int>((f * COORD_DENOMINATOR_LOWPRECISION))) & (COORD_DENOMINATOR_LOWPRECISION - 1)) :
		(abs(static_cast<int>((f * COORD_DENOMINATOR))) & (COORD_DENOMINATOR - 1));

	bool    bInBounds = intval < (1 << COORD_INTEGER_BITS_MP);

	uint bits, numbits;

	if (integral)
	{
		// Integer encoding: in-bounds bit, nonzero bit, optional sign bit + integer value bits
		if (intval)
		{
			// Adjust the integers from [1..MAX_COORD_VALUE] to [0..MAX_COORD_VALUE-1]
			--intval;
			bits = intval * 8 + signbit * 4 + 2 + bInBounds;
			numbits = 3 + (bInBounds ? COORD_INTEGER_BITS_MP : COORD_INTEGER_BITS);
		}
		else
		{
			bits = bInBounds;
			numbits = 2;
		}
	}
	else
	{
		// Float encoding: in-bounds bit, integer bit, sign bit, fraction value bits, optional integer value bits
		if (intval)
		{
			// Adjust the integers from [1..MAX_COORD_VALUE] to [0..MAX_COORD_VALUE-1]
			--intval;
			bits = intval * 8 + signbit * 4 + 2 + bInBounds;
			bits += bInBounds ? (fractval << (3 + COORD_INTEGER_BITS_MP)) : (fractval << (3 + COORD_INTEGER_BITS));
			numbits = 3 + (bInBounds ? COORD_INTEGER_BITS_MP : COORD_INTEGER_BITS)
				+ (lowPrecision ? COORD_FRACTIONAL_BITS_MP_LOWPRECISION : COORD_FRACTIONAL_BITS);
		}
		else
		{
			bits = fractval * 8 + signbit * 4 + 0 + bInBounds;
			numbits = 3 + (lowPrecision ? COORD_FRACTIONAL_BITS_MP_LOWPRECISION : COORD_FRACTIONAL_BITS);
		}
	}

	writeUBitLong(bits, numbits);
}

void bufferWrite::writeBitVec3Coord(const Vector& fa) noexcept
{
	int xflag, yflag, zflag;

	xflag = (fa.x >= COORD_RESOLUTION) || (fa.x <= -COORD_RESOLUTION);
	yflag = (fa.y >= COORD_RESOLUTION) || (fa.y <= -COORD_RESOLUTION);
	zflag = (fa.z >= COORD_RESOLUTION) || (fa.z <= -COORD_RESOLUTION);

	writeOneBit(xflag);
	writeOneBit(yflag);
	writeOneBit(zflag);

	if (xflag)
		writeBitCoord(fa.x);
	if (yflag)
		writeBitCoord(fa.y);
	if (zflag)
		writeBitCoord(fa.z);
}

void bufferWrite::writeBitNormal(float f) noexcept
{
	int	signbit = (f <= -NORMAL_RESOLUTION);

	// NOTE: Since +/-1 are valid values for a normal, I'm going to encode that as all ones
	uint fractval = abs(static_cast<int>(f * NORMAL_DENOMINATOR));

	// clamp..
	if (fractval > NORMAL_DENOMINATOR)
		fractval = NORMAL_DENOMINATOR;

	// Send the sign bit
	writeOneBit(signbit);

	// Send the fractional component
	writeUBitLong(fractval, NORMAL_FRACTIONAL_BITS);
}

void bufferWrite::writeBitVec3Normal(const Vector& fa) noexcept
{
	int xflag, yflag;

	xflag = (fa.x >= NORMAL_RESOLUTION) || (fa.x <= -NORMAL_RESOLUTION);
	yflag = (fa.y >= NORMAL_RESOLUTION) || (fa.y <= -NORMAL_RESOLUTION);

	writeOneBit(xflag);
	writeOneBit(yflag);

	if (xflag)
		writeBitNormal(fa.x);
	if (yflag)
		writeBitNormal(fa.y);

	// Write z sign bit
	int	signbit = (fa.z <= -NORMAL_RESOLUTION);
	writeOneBit(signbit);
}

void bufferWrite::writeBitAngles(const Vector& fa) noexcept
{
	Vector tmp{ fa.x, fa.y, fa.z };
	writeBitVec3Coord(tmp);
}

void bufferWrite::writeChar(int value) noexcept
{
	writeSBitLong(value, sizeof(char) << 3);
}

void bufferWrite::writeByte(int value) noexcept
{
	writeUBitLong(value, sizeof(unsigned char) << 3);
}

void bufferWrite::writeShort(int value) noexcept
{
	writeSBitLong(value, sizeof(short) << 3);
}

void bufferWrite::writeWord(int value) noexcept
{
	writeUBitLong(value, sizeof(unsigned short) << 3);
}

void bufferWrite::writeLong(long value) noexcept
{
	writeSBitLong(value, sizeof(long) << 3);
}

void bufferWrite::writeLongLong(int64 value) noexcept
{
	uint* longs = (uint*)&value;

	// Insert the two DWORDS according to network endian
	const short endianIndex = 0x0100;
	std::byte* idx = (std::byte*)&endianIndex;
	writeUBitLong(longs[(*((int*)idx))++], sizeof(long) << 3);
	writeUBitLong(longs[*((int*)idx)], sizeof(long) << 3);
}

void bufferWrite::writeFloat(float value) noexcept
{
	// Pre-swap the float, since WriteBits writes raw data
	LittleFloat(&value, &value);

	writeBits(&value, sizeof(value) << 3);
}

bool bufferWrite::writeBytes(const void* buffer, int bytes) noexcept
{
	return writeBits(buffer, bytes << 3);
}

bool bufferWrite::writeString(const char* string) noexcept
{
	if (string)
	{
		do
		{
			writeChar(*string);
			++string;
		} while (*(string - 1) != 0);
	}
	else
	{
		writeChar(0);
	}

	return !isOverflowed();
}

bufferRead::bufferRead() noexcept
{
	data = NULL;
	dataBytes = 0;
	dataBits = -1;
	curBit = 0;
	overflow = false;
	assertOnOverflow = true;
	debugName = NULL;
}

bufferRead::bufferRead(const void* data, int bytes, int bits) noexcept
{
	assertOnOverflow = true;
	startReading(data, bytes, 0, bits);
}

bufferRead::bufferRead(const char* debugName, const void* data, int bytes, int bits) noexcept
{
	assertOnOverflow = true;
	this->debugName = debugName;
	startReading(data, bytes, 0, bits);
}

void bufferRead::startReading(const void* data, int bytes, int startBit, int bits) noexcept
{
	this->data = (unsigned char*)data;
	dataBytes = bytes;

	if (bits == -1)
		dataBits = dataBytes << 3;
	else
		dataBits = bits;

	curBit = startBit;
	overflow = false;
}

void bufferRead::reset() noexcept
{
	curBit = 0;
	overflow = false;
}

void bufferRead::setAssertOnOverflow(bool assert) noexcept
{
	assertOnOverflow = assert;
}

void bufferRead::setDebugName(const char* name) noexcept
{
	debugName = name;
}

void bufferRead::setOverflowFlag() noexcept
{
	overflow = true;
}

uint bufferRead::checkReadUBitLong(int numbits) noexcept
{
	// Ok, just read bits out.
	int i, bitValue;
	uint r = 0;

	for (i = 0; i < numbits; i++)
	{
		bitValue = readOneBitNoCheck();
		r |= bitValue << i;
	}
	curBit -= numbits;

	return r;
}

void bufferRead::readBits(void* outData, int bits) noexcept
{
	unsigned char* out = (unsigned char*)outData;
	int bitsLeft = bits;

	// align output to dword boundary
	while (((size_t)out & 3) != 0 && bitsLeft >= 8)
	{
		*out = (unsigned char)readUBitLong(8);
		++out;
		bitsLeft -= 8;
	}

	// read dwords
	while (bitsLeft >= 32)
	{
		*((unsigned long*)out) = readUBitLong(32);
		out += sizeof(unsigned long);
		bitsLeft -= 32;
	}

	// read remaining bytes
	while (bitsLeft >= 8)
	{
		*out = readUBitLong(8);
		++out;
		bitsLeft -= 8;
	}

	// read remaining bits
	if (bitsLeft)
		*out = readUBitLong(bitsLeft);
}

int bufferRead::readBitsClamped_ptr(void* outData, size_t outSizeBytes, size_t bits) noexcept
{
	size_t outSizeBits = outSizeBytes * 8;
	size_t readSizeBits = bits;
	int skippedBits = 0;
	if (readSizeBits > outSizeBits)
	{
		readSizeBits = outSizeBits;
		skippedBits = static_cast<int>(bits - outSizeBits);
	}

	readBits(outData, readSizeBits);
	seekRelative(skippedBits);

	// Return the number of bits actually read.
	return static_cast<int>(readSizeBits);
}

float bufferRead::readBitAngle(int numbits) noexcept
{
	float _return;
	int i;
	float shift;

	shift = static_cast<float>(bitForBitnum(numbits));

	i = readUBitLong(numbits);
	_return = (float)i * (360.0f / shift);

	return _return;
}

uint bufferRead::peekUBitLong(int numbits) noexcept
{
	uint r;
	int i, bitValue;

	bufferRead savebf;

	savebf = *this;  // Save current state info

	r = 0;
	for (i = 0; i < numbits; i++)
	{
		bitValue = readOneBit();

		// Append to current stream
		if (bitValue)
		{
			r |= bitForBitnum(i);
		}
	}

	*this = savebf;
	return r;
}

uint bufferRead::readUBitLongNoInline(int numbits) noexcept
{
	return readUBitLong(numbits);
}

uint bufferRead::readUBitVarInternal(int encodingType) noexcept
{
	curBit -= 4;
	// int bits = { 4, 8, 12, 32 }[ encodingType ];
	int bits = 4 + encodingType * 4 + (((2 - encodingType) >> 31) & 16);
	return readUBitLong(bits);
}

// Append numbits least significant bits from data to the current bit stream
int bufferRead::readSBitLong(int numbits) noexcept
{
	uint r = readUBitLong(numbits);
	uint s = 1 << (numbits - 1);
	if (r >= s)
	{
		// sign-extend by removing sign bit and then subtracting sign bit again
		r = r - s - s;
	}
	return r;
}

uint32 bufferRead::readVarInt32() noexcept
{
	uint32 result = 0;
	int count = 0;
	uint32 b;

	do
	{
		if (count == maxVarint32Bytes)
		{
			return result;
		}
		b = readUBitLong(8);
		result |= (b & 0x7F) << (7 * count);
		++count;
	} while (b & 0x80);

	return result;
}

uint64 bufferRead::readVarInt64() noexcept
{
	uint64 result = 0;
	int count = 0;
	uint64 b;

	do
	{
		if (count == maxVarintBytes)
		{
			return result;
		}
		b = readUBitLong(8);
		result |= static_cast<uint64>(b & 0x7F) << (7 * count);
		++count;
	} while (b & 0x80);

	return result;
}

int32 bufferRead::readSignedVarInt32() noexcept
{
	uint32 value = readVarInt32();
	return zigZagDecode32(value);
}

int64 bufferRead::readSignedVarInt64() noexcept
{
	uint64 value = readVarInt64();
	return zigZagDecode64(value);
}

uint bufferRead::readBitLong(int numbits, bool _signed) noexcept
{
	if (_signed)
		return static_cast<uint>(readSBitLong(numbits));
	else
		return readUBitLong(numbits);
}

// Basic Coordinate Routines (these contain bit-field size AND fixed point scaling constants)
float bufferRead::readBitCoord(void) noexcept
{
	int intval = 0, fractval = 0, signbit = 0;
	float value = 0.0f;


	// Read the required integer and fraction flags
	intval = readOneBit();
	fractval = readOneBit();

	// If we got either parse them, otherwise it's a zero.
	if (intval || fractval)
	{
		// Read the sign bit
		signbit = readOneBit();

		// If there's an integer, read it in
		if (intval)
		{
			// Adjust the integers from [0..MAX_COORD_VALUE-1] to [1..MAX_COORD_VALUE]
			intval = readUBitLong(COORD_INTEGER_BITS) + 1;
		}

		// If there's a fraction, read it in
		if (fractval)
		{
			fractval = readUBitLong(COORD_FRACTIONAL_BITS);
		}

		// Calculate the correct floating point value
		value = intval + (static_cast<float>(fractval) * COORD_RESOLUTION);

		// Fixup the sign if negative.
		if (signbit)
			value = -value;
	}

	return value;
}

float bufferRead::readBitCoordMP(bool integral, bool lowPrecision) noexcept
{
	// BitCoordMP float encoding: inbounds bit, integer bit, sign bit, optional int bits, float bits
	// BitCoordMP integer encoding: inbounds bit, integer bit, optional sign bit, optional int bits.
	// int bits are always encoded as (value - 1) since zero is handled by the integer bit

	// With integer-only encoding, the presence of the third bit depends on the second
	int flags = readUBitLong(3 - integral);
	enum { INBOUNDS = 1, INTVAL = 2, SIGN = 4 };

	if (integral)
	{
		if (flags & INTVAL)
		{
			// Read the third bit and the integer portion together at once
			uint bits = readUBitLong((flags & INBOUNDS) ? COORD_INTEGER_BITS_MP + 1 : COORD_INTEGER_BITS + 1);
			// Remap from [0,N] to [1,N+1]
			int intval = (bits >> 1) + 1;
			return (bits & 1) ? static_cast<float>(-intval) : static_cast<float>(intval);
		}
		return 0.f;
	}

	static const float mul_table[4] =
	{
		1.f / (1 << COORD_FRACTIONAL_BITS),
		-1.f / (1 << COORD_FRACTIONAL_BITS),
		1.f / (1 << COORD_FRACTIONAL_BITS_MP_LOWPRECISION),
		-1.f / (1 << COORD_FRACTIONAL_BITS_MP_LOWPRECISION)
	};
	//equivalent to: float multiply = mul_table[ ((flags & SIGN) ? 1 : 0) + bLowPrecision*2 ];
	float multiply = *(float*)((uintptr_t)&mul_table[0] + (flags & 4) + lowPrecision * 8);

	static const unsigned char numbits_table[8] =
	{
		COORD_FRACTIONAL_BITS,
		COORD_FRACTIONAL_BITS,
		COORD_FRACTIONAL_BITS + COORD_INTEGER_BITS,
		COORD_FRACTIONAL_BITS + COORD_INTEGER_BITS_MP,
		COORD_FRACTIONAL_BITS_MP_LOWPRECISION,
		COORD_FRACTIONAL_BITS_MP_LOWPRECISION,
		COORD_FRACTIONAL_BITS_MP_LOWPRECISION + COORD_INTEGER_BITS,
		COORD_FRACTIONAL_BITS_MP_LOWPRECISION + COORD_INTEGER_BITS_MP
	};
	uint bits = readUBitLong(numbits_table[(flags & (INBOUNDS | INTVAL)) + lowPrecision * 4]);

	if (flags & INTVAL)
	{
		// Shuffle the bits to remap the integer portion from [0,N] to [1,N+1]
		// and then paste in front of the fractional parts so we only need one
		// int-to-float conversion.

		uint fracbitsMP = bits >> COORD_INTEGER_BITS_MP;
		uint fracbits = bits >> COORD_INTEGER_BITS;

		uint intmaskMP = ((1 << COORD_INTEGER_BITS_MP) - 1);
		uint intmask = ((1 << COORD_INTEGER_BITS) - 1);

		uint selectNotMP = (flags & INBOUNDS) - 1;

		fracbits -= fracbitsMP;
		fracbits &= selectNotMP;
		fracbits += fracbitsMP;

		intmask -= intmaskMP;
		intmask &= selectNotMP;
		intmask += intmaskMP;

		uint intpart = (bits & intmask) + 1;
		uint intbitsLow = intpart << COORD_FRACTIONAL_BITS_MP_LOWPRECISION;
		uint intbits = intpart << COORD_FRACTIONAL_BITS;
		uint selectNotLow = (uint)lowPrecision - 1;

		intbits -= intbitsLow;
		intbits &= selectNotLow;
		intbits += intbitsLow;

		bits = fracbits | intbits;
	}

	return static_cast<int>(bits) * multiply;
}

uint bufferRead::readBitCoordBits() noexcept
{
	uint flags = readUBitLong(2);
	if (flags == 0)
		return 0;

	static const int numbits_table[3] =
	{
		COORD_INTEGER_BITS + 1,
		COORD_FRACTIONAL_BITS + 1,
		COORD_INTEGER_BITS + COORD_FRACTIONAL_BITS + 1
	};
	return readUBitLong(numbits_table[flags - 1]) * 4 + flags;
}

uint bufferRead::readBitCoordMPBits(bool integral, bool lowPrecision) noexcept
{
	uint flags = readUBitLong(2);
	enum { INBOUNDS = 1, INTVAL = 2 };
	int numbits = 0;

	if (integral)
	{
		if (flags & INTVAL)
		{
			numbits = (flags & INBOUNDS) ? (1 + COORD_INTEGER_BITS_MP) : (1 + COORD_INTEGER_BITS);
		}
		else
		{
			return flags; // no extra bits
		}
	}
	else
	{
		static const unsigned char numbits_table[8] =
		{
			1 + COORD_FRACTIONAL_BITS,
			1 + COORD_FRACTIONAL_BITS,
			1 + COORD_FRACTIONAL_BITS + COORD_INTEGER_BITS,
			1 + COORD_FRACTIONAL_BITS + COORD_INTEGER_BITS_MP,
			1 + COORD_FRACTIONAL_BITS_MP_LOWPRECISION,
			1 + COORD_FRACTIONAL_BITS_MP_LOWPRECISION,
			1 + COORD_FRACTIONAL_BITS_MP_LOWPRECISION + COORD_INTEGER_BITS,
			1 + COORD_FRACTIONAL_BITS_MP_LOWPRECISION + COORD_INTEGER_BITS_MP
		};
		numbits = numbits_table[flags + lowPrecision * 4];
	}

	return flags + readUBitLong(numbits) * 4;
}

void bufferRead::readBitVec3Coord(Vector& fa) noexcept
{
	int xflag, yflag, zflag;

	fa = { 0, 0, 0 };

	xflag = readOneBit();
	yflag = readOneBit();
	zflag = readOneBit();

	if (xflag)
		fa.x = readBitCoord();
	if (yflag)
		fa.y = readBitCoord();
	if (zflag)
		fa.z = readBitCoord();
}

float bufferRead::readBitNormal() noexcept
{
	// Read the sign bit
	int	signbit = readOneBit();

	// Read the fractional part
	uint fractval = readUBitLong(NORMAL_FRACTIONAL_BITS);

	// Calculate the correct floating point value
	float value = static_cast<float>(fractval) * NORMAL_RESOLUTION;

	// Fixup the sign if negative.
	if (signbit)
		value = -value;

	return value;
}

void bufferRead::readBitVec3Normal(Vector& fa) noexcept
{
	int xflag = readOneBit();
	int yflag = readOneBit();

	if (xflag)
		fa.x = readBitNormal();
	else
		fa.x = 0.0f;

	if (yflag)
		fa.y = readBitNormal();
	else
		fa.y = 0.0f;

	// The first two imply the third (but not its sign)
	int znegative = readOneBit();

	float fafafbfb = fa.x * fa.x + fa.y * fa.y;
	if (fafafbfb < 1.0f)
		fa.z = sqrt(1.0f - fafafbfb);
	else
		fa.z = 0.0f;

	if (znegative)
		fa.z = -fa.z;
}

void bufferRead::readBitAngles(Vector& fa) noexcept
{
	Vector tmp;
	readBitVec3Coord(tmp);
	fa = { tmp.x, tmp.y, tmp.z };
}

int64 bufferRead::readLongLong() noexcept
{
	int64 retval;
	uint* longs = (uint*)&retval;

	// Read the two DWORDs according to network endian
	const short endianIndex = 0x0100;
	std::byte* idx = (std::byte*)&endianIndex;
	longs[(*(int*)idx)++] = readUBitLong(sizeof(long) << 3);
	longs[(*(int*)idx)] = readUBitLong(sizeof(long) << 3);

	return retval;
}

float bufferRead::readFloat() noexcept
{
	float ret;
	readBits(&ret, 32);

	// Swap the float, since ReadBits reads raw data
	LittleFloat(&ret, &ret);
	return ret;
}

bool bufferRead::readBytes(void* out, int bytes) noexcept
{
	readBits(out, bytes << 3);
	return !isOverflowed();
}

bool bufferRead::readString(char* string, int bufferLength, bool line, int* outNumChars) noexcept
{
	bool tooSmall = false;
	int _char = 0;
	while (1)
	{
		char val = readChar();
		if (val == 0)
			break;
		else if (line && val == '\n')
			break;

		if (_char < (bufferLength - 1))
		{
			string[_char] = val;
			++_char;
		}
		else
		{
			tooSmall = true;
		}
	}

	// Make sure it's null-terminated.
	//Assert(iChar < maxLen);
	string[_char] = 0;

	if (outNumChars)
		*outNumChars = _char;

	return !isOverflowed() && !tooSmall;
}


char* bufferRead::readAndAllocateString(bool* overflow) noexcept
{
	char str[2048];

	int nChars;
	bool bOverflow = !readString(str, sizeof(str), false, &nChars);
	if (overflow)
		*overflow = bOverflow;

	// Now copy into the output and return it;
	char* pRet = new char[nChars + 1];
	for (int i = 0; i <= nChars; i++)
		pRet[i] = str[i];

	return pRet;
}

void bufferRead::exciseBits(int startbit, int bitstoremove) noexcept
{
	int endbit = startbit + bitstoremove;
	int remaining_to_end = dataBits - endbit;

	bufferWrite temp;
	temp.startWriting((void*)data, dataBits << 3, startbit);

	seek(endbit);

	for (int i = 0; i < remaining_to_end; i++)
	{
		temp.writeOneBit(readOneBit());
	}

	seek(startbit);

	dataBits -= bitstoremove;
	dataBytes = dataBits >> 3;
}