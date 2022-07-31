#pragma once
#include <stack>

#include "Datamap.h"

typedef unsigned char byte;

typedef void (*FN_FIELD_COMPARE)(const char* classname, const char* fieldname, const char* fieldtype,
	bool networked, bool noterrorchecked, bool differs, bool withintolerance, const char* value);

enum
{
	SLOT_ORIGINALDATA = -1,
};

class PredictionCopy
{
public:
	typedef enum
	{
		DIFFERS = 0,
		IDENTICAL,
		WITHINTOLERANCE,
	} difftype_t;

	typedef enum
	{
		TRANSFERDATA_COPYONLY = 0,  // Data copying only (uses runs)
		TRANSFERDATA_ERRORCHECK_NOSPEW, // Checks for errors, returns after first error found
		TRANSFERDATA_ERRORCHECK_SPEW,   // checks for errors, reports all errors to console
		TRANSFERDATA_ERRORCHECK_DESCRIBE, // used by hud_pdump, dumps values, etc, for all fields
	} optype_t;

	PredictionCopy(int type, byte* dest, bool dest_packed, const byte* src, bool src_packed,
		optype_t opType, FN_FIELD_COMPARE func = NULL) noexcept;

	int	transferData(const char* operation, int entindex, datamap* dmap) noexcept;
private:
	optype_t opType;
	int type;
	void* dest;
	const void* src;
	int destOffsetIndex;
	int srcOffsetIndex;
	int errorCount;
	int entIndex;

	FN_FIELD_COMPARE fieldCompareFunc;

	const typedescription_t* watchField;
	char const* operation;

	std::stack< const typedescription_t* > fieldStack;
};