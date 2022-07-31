#include "../Memory.h"

#include "PredictionCopy.h"

PredictionCopy::PredictionCopy(int type, byte* dest, bool dest_packed, const byte* src, bool src_packed, optype_t opType, FN_FIELD_COMPARE func) noexcept
{
	this->opType = opType;
	this->type = type; 
	this->dest = dest;
	this->src = src;
	destOffsetIndex = dest_packed ? TD_OFFSET_PACKED : TD_OFFSET_NORMAL;
	srcOffsetIndex = src_packed ? TD_OFFSET_PACKED : TD_OFFSET_NORMAL;

	errorCount = 0;
	entIndex = -1;

	watchField = NULL;
	fieldCompareFunc = func;
}

int PredictionCopy::transferData(const char* operation, int entindex, datamap* dmap) noexcept
{
	return memory->transferData(this, operation, entindex, dmap);
}