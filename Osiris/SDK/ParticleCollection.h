#pragma once

#include "Pad.h"

template<class T> struct UtlReference {
	UtlReference* next;
	UtlReference* prev;
	T* object;
};

enum AttributeDataType {
	ATTRDATATYPE_NONE = -1,
	ATTRDATATYPE_FLOAT = 0,
	ATTRDATATYPE_4V,
	ATTRDATATYPE_INT,
	ATTRDATATYPE_POINTER,

	ATTRDATATYPE_COUNT,
};

#define MAX_PARTICLE_ATTRIBUTES 24

#define DEFPARTICLE_ATTRIBUTE( name, bit, datatype )			\
	const int PARTICLE_ATTRIBUTE_##name##_MASK = (1 << bit);	\
	const int PARTICLE_ATTRIBUTE_##name = bit;					\
	const AttributeDataType PARTICLE_ATTRIBUTE_##name##_DATATYPE = datatype;

DEFPARTICLE_ATTRIBUTE(TINT_RGB, 6, ATTRDATATYPE_4V);

DEFPARTICLE_ATTRIBUTE(ALPHA, 7, ATTRDATATYPE_FLOAT);

struct ParticleAttributeAddressTable {
	float* attributes[MAX_PARTICLE_ATTRIBUTES];
	size_t floatStrides[MAX_PARTICLE_ATTRIBUTES];

	FORCEINLINE float* FloatAttributePtr(int attribute, int particleNumber) const {
		int block_ofs = particleNumber / 4;
		return attributes[attribute] +
			floatStrides[attribute] * block_ofs +
			(particleNumber & 3);
	}

};

struct UtlStringSimple {
	char* buffer;
	int capacity;
	int growSize;
	int length;
};

class ParticleSystemDefinition {
	PAD(308);
public:
	UtlStringSimple name;
};

class ParticleCollection {
	PAD(48);//0
public:
	int activeParticles;//48
private:
	PAD(12);//52
public:
	UtlReference<ParticleSystemDefinition> def;//64
private:
	PAD(60);//80
public:
	ParticleCollection* parent;//136
private:
	PAD(84);//140
public:
	ParticleAttributeAddressTable particleAttributes;//224
};