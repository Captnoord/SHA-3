
#include "stdafx.h"


// Endianness conversion inlines - These will be optimised out on platforms where
//  it's not necessary, and will collapse to only the swapping code on 
//  other big endian platforms.

inline bool littleEndian()
{
	uint16_t tn = 0xff00;
	if(((uint8_t*)&tn)[0] == 0x00)
		return true;
	else 
		return false;
}

inline uint16_t NativeToLittle(uint16_t in)
{
	if(littleEndian())
		return in;
	else
	{
		uint16_t var = 0;
		((uint8_t*)&var)[0] = ((uint8_t*)&in)[1];
		((uint8_t*)&var)[1] = ((uint8_t*)&in)[0];

		return var;
	}
}

inline uint32_t NativeToLittle(uint32_t in)
{
	if(littleEndian())
		return in;
	else
	{
		uint32_t var = 0;
		((uint16_t*)&var)[0] = NativeToLittle(((uint16_t*)&in)[1]);
		((uint16_t*)&var)[1] = NativeToLittle(((uint16_t*)&in)[0]);

		return var;	
	}
}

inline uint64_t NativeToLittle(uint64_t in)
{
	if(littleEndian())
		return in;
	else
	{
		uint64_t var = 0;
		((uint32_t*)&var)[0] = NativeToLittle(((uint32_t*)&in)[1]);
		((uint32_t*)&var)[1] = NativeToLittle(((uint32_t*)&in)[0]);

		return var;
	
	}
}

inline uint64_t LittleToNative(uint64_t in)
{
	if(littleEndian())
		return in;
	else
	{
		uint64_t var = 0;
		((uint32_t*)&var)[0] = NativeToLittle(((uint32_t*)&in)[1]);
		((uint32_t*)&var)[1] = NativeToLittle(((uint32_t*)&in)[0]);

		return var;
	
	}
}

// Constants of the Keccak algorithm.

uint64_t RC[] = {
	0x0000000000000001L, 0x0000000000008082L, 0x800000000000808aL,
	0x8000000080008000L, 0x000000000000808bL, 0x0000000080000001L,
	0x8000000080008081L, 0x8000000000008009L, 0x000000000000008aL,
	0x0000000000000088L, 0x0000000080008009L, 0x000000008000000aL,
	0x000000008000808bL, 0x800000000000008bL, 0x8000000000008089L,
	0x8000000000008003L, 0x8000000000008002L, 0x8000000000000080L,
	0x000000000000800aL, 0x800000008000000aL, 0x8000000080008081L,
	0x8000000000008080L, 0x0000000080000001L, 0x8000000080008008L
};

int R[] = {
	0, 1, 62, 28, 27, 36, 44, 6, 55, 20, 3, 10, 43,
	25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14
};


// State structure
struct keccakState
{
	uint64_t *A;
	unsigned int blockLen;
	uint8_t *buffer;
	unsigned int bufferLen;
	int length;
};


// Function to create the state structure for keccak application, of size length
//   (where length is the number of bits in the hash divided by 8. 
//   (eg 64 for SHA-3-512)
struct keccakState *keccakCreate(int length)
{
	struct keccakState *state = new keccakState;
	memset(state, 0, sizeof(keccakState));

	state->A = new uint64_t[25];
	memset(state->A, 0, 25*sizeof(uint64_t));
	state->blockLen = 200 - 2 * (length/8);
	state->buffer = new uint8_t[state->blockLen];
	memset(state->buffer, 0, state->blockLen*sizeof(uint8_t));
	state->bufferLen = 0;
	state->length = length;
	return state;
}

inline int index(int x);
inline int index(int x, int y);
void keccakProcessBuffer(struct keccakState *state);
void keccakUpdate(uint8_t *input, int off, int len);
unsigned char *keccakDigest(keccakState *state);
unsigned char *sha3Digest(keccakState *state);
void keccakAddPadding(keccakState *state);
void keccakf(keccakState *state);
void sha3AddPadding(keccakState *state);

void keccakReset(keccakState *state)
{
	for (unsigned int i = 0; i < 25; i++) 
	{
		state->A[i] = 0L;
	}
	state->bufferLen = 0;
}

// keccakUpdate - Functions to pack input data into a block

//  One byte input at a time - process buffer if it's empty
void keccakUpdate(uint8_t input, keccakState *state)
{
	state->buffer[state->bufferLen] = input;
	if (++(state->bufferLen) == state->blockLen) 
	{
		keccakProcessBuffer(state);
	}
}

//  Process a larger buffer with varying amounts of data in it
void keccakUpdate(const uint8_t *input, int off, int len, keccakState *state)
{
	uint8_t *buffer = state->buffer;
	while (len > 0) 
	{
		int cpLen = 0;
		if((state->blockLen - state->bufferLen) > len)
		{
			cpLen = len;
		}
		else
		{
			cpLen = state->blockLen - state->bufferLen;
		}

		for(unsigned int i=0 ; i!=cpLen ; i++)
		{
			buffer[state->bufferLen+i] = input[off+i];
		}
		state->bufferLen += cpLen;
		off += cpLen;
		len -= cpLen;
		if (state->bufferLen == state->blockLen) 
		{
			keccakProcessBuffer(state);
		}
	}
}


// keccakDigest - called once all data has been few to the keccakUpdate functions
//  Pads the structure (in case the input is not a multiple of the block length)
//  returns the hash result in a char array (not null terminated)
unsigned char *keccakDigest(keccakState *state)
{
	uint64_t *A = state->A;
	keccakAddPadding(state);
	keccakProcessBuffer(state);
	uint64_t *tmp = new uint64_t[state->length];
	for (unsigned int i = 0; i < state->length ; i += 8) 
	{
		tmp[i >> 3] = NativeToLittle(A[i >> 3]);
	}
	keccakReset(state);
	return (unsigned char*)tmp;
}

// sha3Digest - called once all data has been few to the keccakUpdate functions
//  Pads the structure (in case the input is not a multiple of the block length)
//  returns the hash result in a char array (not null terminated)
unsigned char *sha3Digest(keccakState *state)
{
	uint64_t *A = state->A;
	sha3AddPadding(state);
	keccakProcessBuffer(state);
	uint64_t *tmp = new uint64_t[state->length];
	for (unsigned int i = 0; i < state->length ; i += 8) 
	{
		tmp[i >> 3] = NativeToLittle(A[i >> 3]);
	}
	keccakReset(state);
	return (unsigned char*)tmp;
}

void sha3AddPadding(keccakState *state)
{
	unsigned int bufferLen = state->bufferLen;
	uint8_t *buffer = state->buffer;
	if (state->bufferLen + 1 == state->blockLen) 
	{
		buffer[bufferLen] = (uint8_t) 0x86;
	} 
	else 
	{
		buffer[bufferLen] = (uint8_t) 0x06;
		for (unsigned int i = bufferLen + 1; i < state->blockLen - 1; i++) 
		{
			buffer[i] = 0;
		}
		buffer[state->blockLen - 1] = (uint8_t) 0x80;
	}
}

void keccakAddPadding(keccakState *state)
{
	unsigned int bufferLen = state->bufferLen;
	uint8_t *buffer = state->buffer;
	if (state->bufferLen + 1 == state->blockLen) 
	{
		buffer[bufferLen] = (uint8_t) 0x81;
	} 
	else 
	{
		buffer[bufferLen] = (uint8_t) 0x01;
		for (unsigned int i = bufferLen + 1; i < state->blockLen - 1; i++) 
		{
			buffer[i] = 0;
		}
		buffer[state->blockLen - 1] = (uint8_t) 0x80;
	}
}

void keccakProcessBuffer(struct keccakState *state)
{
	uint64_t *A = state->A;
	for (int i = 0; i < state->blockLen/8 ; i++) 
	{
		A[i] ^= LittleToNative(((uint64_t*)state->buffer)[i]);
	}
	keccakf(state);
	state->bufferLen = 0;
}


// As we're not using assembly, we can't use the native rotation instructions
//  replace it with a small inline
static inline uint64_t rotateLeft(uint64_t x, int n)
{
	return ((x << n) | (x >> (64 - n)));
}


inline int index(int x)
{
	return x < 0 ? index(x + 5) : x % 5;
}

inline int index(int x, int y)
{
	return index(x) + 5 * index(y);
}


// Hash function proper. 
void keccakf(keccakState *state)
{
	uint64_t *A = state->A;
	uint64_t *B = new uint64_t[25];
	uint64_t *C = new uint64_t[5];
	uint64_t *D = new uint64_t[5];
	for (int n = 0; n < 24; n++) 
	{
		for (int x = 0; x < 5; x++) 
		{
			C[x] = A[index(x, 0)] ^ A[index(x, 1)] ^ A[index(x, 2)] ^ A[index(x, 3)] ^ A[index(x, 4)];
		}
		for (int x = 0; x < 5; x++) 
		{
			D[x] = C[index(x - 1)] ^ rotateLeft(C[index(x + 1)], 1);
			for (int y = 0; y < 5; y++) 
			{
				A[index(x, y)] ^= D[x];
			}
		}
		for (int x = 0; x < 5; x++) 
		{
			for (int y = 0; y < 5; y++) 
			{
				int i = index(x, y);
				B[index(y, x * 2 + 3 * y)] = rotateLeft(A[i], R[i]);
			}
		}
		for (int x = 0; x < 5; x++) 
		{
			for (int y = 0; y < 5; y++) 
			{
				int i = index(x, y);
				A[i] = B[i] ^ (~B[index(x + 1, y)] & B[index(x + 2, y)]);
			}
		}
		A[0] ^= RC[n];
	}
}

// Test driver. Reads in test.txt and outputs the SHA-3-512 hash.

int _tmain(int argc, _TCHAR* argv[])
{
	unsigned int hashSize = 256;
	keccakState *st = keccakCreate(hashSize);

	FILE *fHand = fopen("test.txt", "rb");
	if(!fHand)
	{
		puts("Unable to open input file. \n");
		return 0;
	}
	fseek(fHand, 0, SEEK_END);
	unsigned int fileSize = ftell(fHand);
	fseek(fHand, 0, SEEK_SET);
	char *msg = new char[fileSize];
	fread(msg, 1, fileSize, fHand);
	fclose(fHand);

	keccakUpdate((uint8_t*)msg, 0, 0, st);

	unsigned char *op = sha3Digest(st);

	printf("Output: ");
	for(unsigned int i=0 ; i!=(hashSize/8) ; i++)
	{
		printf("%.2x", *(op++));
	}
	printf("\n");
	return 0;
}
