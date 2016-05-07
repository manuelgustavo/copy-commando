#pragma once

namespace crc32 {
	// //////////////////////////////////////////////////////////
	// Crc32.cpp
	// Copyright (c) 2011-2015 Stephan Brumme. All rights reserved.
	// Slicing-by-16 contributed by Bulat Ziganshin
	// see http://create.stephan-brumme.com/disclaimer.html
	//

	// g++ -o Crc32 Crc32.cpp -O3 -lrt -march=native -mtune=native

	// if running on an embedded system, you might consider shrinking the
	// big Crc32Lookup table:
	// - crc32_bitwise doesn't need it at all
	// - crc32_halfbyte has its own small lookup table
	// - crc32_1byte    needs only Crc32Lookup[0]
	// - crc32_4bytes   needs only Crc32Lookup[0..3]
	// - crc32_8bytes   needs only Crc32Lookup[0..7]
	// - crc32_4x8bytes needs only Crc32Lookup[0..7]
	// - crc32_16bytes  needs all of Crc32Lookup


#include <stdlib.h>

	// define endianess and some integer data types
#if defined(_MSC_VER) || defined(__MINGW32__)
	typedef unsigned __int8  uint8_t;
	typedef unsigned __int32 uint32_t;
	typedef   signed __int32  int32_t;

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER    __LITTLE_ENDIAN

#include <xmmintrin.h>
#ifdef __MINGW32__
#define PREFETCH(location) __builtin_prefetch(location)
#else
#define PREFETCH(location) _mm_prefetch(location, _MM_HINT_T0)
#endif
#else
	// uint8_t, uint32_t, in32_t
#include <stdint.h>
	// defines __BYTE_ORDER as __LITTLE_ENDIAN or __BIG_ENDIAN
#include <sys/param.h>

#ifdef __GNUC__
#define PREFETCH(location) __builtin_prefetch(location)
#else
#define PREFETCH(location) ;
#endif
#endif


	/// zlib's CRC32 polynomial
	const uint32_t Polynomial = 0xEDB88320;

	/// swap endianess
	static inline uint32_t swap(uint32_t x)
	{
#if defined(__GNUC__) || defined(__clang__)
		return __builtin_bswap32(x);
#else
		return (x >> 24) |
			((x >> 8) & 0x0000FF00) |
			((x << 8) & 0x00FF0000) |
			(x << 24);
#endif
	}


	/// Slicing-By-16
	const size_t MaxSlice = 16;
	/// forward declaration, table is at the end of this file
	extern const uint32_t Crc32Lookup[MaxSlice][256]; // extern is needed to keep compiler happey


													  /// compute CRC32 (bitwise algorithm)
	inline uint32_t crc32_bitwise(const void* data, size_t length, uint32_t previousCrc32 = 0)
	{
		uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
		const uint8_t* current = (const uint8_t*)data;

		while (length-- != 0)
		{
			crc ^= *current++;

			for (int j = 0; j < 8; j++)
			{
				// branch-free
				crc = (crc >> 1) ^ (-int32_t(crc & 1) & Polynomial);

				// branching, much slower:
				//if (crc & 1)
				//  crc = (crc >> 1) ^ Polynomial;
				//else
				//  crc =  crc >> 1;
			}
		}

		return ~crc; // same as crc ^ 0xFFFFFFFF
	}


	/// compute CRC32 (half-byte algoritm)
	inline uint32_t crc32_halfbyte(const void* data, size_t length, uint32_t previousCrc32 = 0)
	{
		uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
		const uint8_t* current = (const uint8_t*)data;

		/// look-up table for half-byte, same as crc32Lookup[0][16*i]
		static const uint32_t Crc32Lookup16[16] =
		{
			0x00000000,0x1DB71064,0x3B6E20C8,0x26D930AC,0x76DC4190,0x6B6B51F4,0x4DB26158,0x5005713C,
			0xEDB88320,0xF00F9344,0xD6D6A3E8,0xCB61B38C,0x9B64C2B0,0x86D3D2D4,0xA00AE278,0xBDBDF21C
		};

		while (length-- != 0)
		{
			crc = Crc32Lookup16[(crc ^  *current) & 0x0F] ^ (crc >> 4);
			crc = Crc32Lookup16[(crc ^ (*current >> 4)) & 0x0F] ^ (crc >> 4);
			current++;
		}

		return ~crc; // same as crc ^ 0xFFFFFFFF
	}


	/// compute CRC32 (standard algorithm)
	inline uint32_t crc32_1byte(const void* data, size_t length, uint32_t previousCrc32 = 0)
	{
		uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
		const uint8_t* current = (const uint8_t*)data;

		while (length-- > 0)
			crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *current++];

		return ~crc; // same as crc ^ 0xFFFFFFFF
	}


	/// compute CRC32 (Slicing-by-4 algorithm)
	inline uint32_t crc32_4bytes(const void* data, size_t length, uint32_t previousCrc32 = 0)
	{
		uint32_t  crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
		const uint32_t* current = (const uint32_t*)data;

		// process four bytes at once (Slicing-by-4)
		while (length >= 4)
		{
#if __BYTE_ORDER == __BIG_ENDIAN
			uint32_t one = *current++ ^ swap(crc);
			crc = Crc32Lookup[0][one & 0xFF] ^
				Crc32Lookup[1][(one >> 8) & 0xFF] ^
				Crc32Lookup[2][(one >> 16) & 0xFF] ^
				Crc32Lookup[3][(one >> 24) & 0xFF];
#else
			uint32_t one = *current++ ^ crc;
			crc = Crc32Lookup[0][(one >> 24) & 0xFF] ^
				Crc32Lookup[1][(one >> 16) & 0xFF] ^
				Crc32Lookup[2][(one >> 8) & 0xFF] ^
				Crc32Lookup[3][one & 0xFF];
#endif

			length -= 4;
		}

		const uint8_t* currentChar = (const uint8_t*)current;
		// remaining 1 to 3 bytes (standard algorithm)
		while (length-- != 0)
			crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

		return ~crc; // same as crc ^ 0xFFFFFFFF
	}


	/// compute CRC32 (Slicing-by-8 algorithm)
	inline uint32_t crc32_8bytes(const void* data, size_t length, uint32_t previousCrc32 = 0)
	{
		uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
		const uint32_t* current = (const uint32_t*)data;

		// process eight bytes at once (Slicing-by-8)
		while (length >= 8)
		{
#if __BYTE_ORDER == __BIG_ENDIAN
			uint32_t one = *current++ ^ swap(crc);
			uint32_t two = *current++;
			crc = Crc32Lookup[0][two & 0xFF] ^
				Crc32Lookup[1][(two >> 8) & 0xFF] ^
				Crc32Lookup[2][(two >> 16) & 0xFF] ^
				Crc32Lookup[3][(two >> 24) & 0xFF] ^
				Crc32Lookup[4][one & 0xFF] ^
				Crc32Lookup[5][(one >> 8) & 0xFF] ^
				Crc32Lookup[6][(one >> 16) & 0xFF] ^
				Crc32Lookup[7][(one >> 24) & 0xFF];
#else
			uint32_t one = *current++ ^ crc;
			uint32_t two = *current++;
			crc = Crc32Lookup[0][(two >> 24) & 0xFF] ^
				Crc32Lookup[1][(two >> 16) & 0xFF] ^
				Crc32Lookup[2][(two >> 8) & 0xFF] ^
				Crc32Lookup[3][two & 0xFF] ^
				Crc32Lookup[4][(one >> 24) & 0xFF] ^
				Crc32Lookup[5][(one >> 16) & 0xFF] ^
				Crc32Lookup[6][(one >> 8) & 0xFF] ^
				Crc32Lookup[7][one & 0xFF];
#endif

			length -= 8;
		}

		const uint8_t* currentChar = (const uint8_t*)current;
		// remaining 1 to 7 bytes (standard algorithm)
		while (length-- != 0)
			crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

		return ~crc; // same as crc ^ 0xFFFFFFFF
	}


	/// compute CRC32 (Slicing-by-8 algorithm), unroll inner loop 4 times
	inline uint32_t crc32_4x8bytes(const void* data, size_t length, uint32_t previousCrc32 = 0)
	{
		uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
		const uint32_t* current = (const uint32_t*)data;

		// enabling optimization (at least -O2) automatically unrolls the inner for-loop
		const size_t Unroll = 4;
		const size_t BytesAtOnce = 8 * Unroll;

		// process 4x eight bytes at once (Slicing-by-8)
		while (length >= BytesAtOnce)
		{
			for (size_t unrolling = 0; unrolling < Unroll; unrolling++)
			{
#if __BYTE_ORDER == __BIG_ENDIAN
				uint32_t one = *current++ ^ swap(crc);
				uint32_t two = *current++;
				crc = Crc32Lookup[0][two & 0xFF] ^
					Crc32Lookup[1][(two >> 8) & 0xFF] ^
					Crc32Lookup[2][(two >> 16) & 0xFF] ^
					Crc32Lookup[3][(two >> 24) & 0xFF] ^
					Crc32Lookup[4][one & 0xFF] ^
					Crc32Lookup[5][(one >> 8) & 0xFF] ^
					Crc32Lookup[6][(one >> 16) & 0xFF] ^
					Crc32Lookup[7][(one >> 24) & 0xFF];
#else
				uint32_t one = *current++ ^ crc;
				uint32_t two = *current++;
				crc = Crc32Lookup[0][(two >> 24) & 0xFF] ^
					Crc32Lookup[1][(two >> 16) & 0xFF] ^
					Crc32Lookup[2][(two >> 8) & 0xFF] ^
					Crc32Lookup[3][two & 0xFF] ^
					Crc32Lookup[4][(one >> 24) & 0xFF] ^
					Crc32Lookup[5][(one >> 16) & 0xFF] ^
					Crc32Lookup[6][(one >> 8) & 0xFF] ^
					Crc32Lookup[7][one & 0xFF];
#endif

			}

			length -= BytesAtOnce;
		}

		const uint8_t* currentChar = (const uint8_t*)current;
		// remaining 1 to 31 bytes (standard algorithm)
		while (length-- != 0)
			crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

		return ~crc; // same as crc ^ 0xFFFFFFFF
	}


	/// compute CRC32 (Slicing-by-16 algorithm)
	inline uint32_t crc32_16bytes(const void* data, size_t length, uint32_t previousCrc32 = 0)
	{
		uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
		const uint32_t* current = (const uint32_t*)data;

		// enabling optimization (at least -O2) automatically unrolls the inner for-loop
		const size_t Unroll = 4;
		const size_t BytesAtOnce = 16 * Unroll;

		while (length >= BytesAtOnce)
		{
			for (size_t unrolling = 0; unrolling < Unroll; unrolling++)
			{
#if __BYTE_ORDER == __BIG_ENDIAN
				uint32_t one = *current++ ^ swap(crc);
				uint32_t two = *current++;
				uint32_t three = *current++;
				uint32_t four = *current++;
				crc = Crc32Lookup[0][four & 0xFF] ^
					Crc32Lookup[1][(four >> 8) & 0xFF] ^
					Crc32Lookup[2][(four >> 16) & 0xFF] ^
					Crc32Lookup[3][(four >> 24) & 0xFF] ^
					Crc32Lookup[4][three & 0xFF] ^
					Crc32Lookup[5][(three >> 8) & 0xFF] ^
					Crc32Lookup[6][(three >> 16) & 0xFF] ^
					Crc32Lookup[7][(three >> 24) & 0xFF] ^
					Crc32Lookup[8][two & 0xFF] ^
					Crc32Lookup[9][(two >> 8) & 0xFF] ^
					Crc32Lookup[10][(two >> 16) & 0xFF] ^
					Crc32Lookup[11][(two >> 24) & 0xFF] ^
					Crc32Lookup[12][one & 0xFF] ^
					Crc32Lookup[13][(one >> 8) & 0xFF] ^
					Crc32Lookup[14][(one >> 16) & 0xFF] ^
					Crc32Lookup[15][(one >> 24) & 0xFF];
#else
				uint32_t one = *current++ ^ crc;
				uint32_t two = *current++;
				uint32_t three = *current++;
				uint32_t four = *current++;
				crc = Crc32Lookup[0][(four >> 24) & 0xFF] ^
					Crc32Lookup[1][(four >> 16) & 0xFF] ^
					Crc32Lookup[2][(four >> 8) & 0xFF] ^
					Crc32Lookup[3][four & 0xFF] ^
					Crc32Lookup[4][(three >> 24) & 0xFF] ^
					Crc32Lookup[5][(three >> 16) & 0xFF] ^
					Crc32Lookup[6][(three >> 8) & 0xFF] ^
					Crc32Lookup[7][three & 0xFF] ^
					Crc32Lookup[8][(two >> 24) & 0xFF] ^
					Crc32Lookup[9][(two >> 16) & 0xFF] ^
					Crc32Lookup[10][(two >> 8) & 0xFF] ^
					Crc32Lookup[11][two & 0xFF] ^
					Crc32Lookup[12][(one >> 24) & 0xFF] ^
					Crc32Lookup[13][(one >> 16) & 0xFF] ^
					Crc32Lookup[14][(one >> 8) & 0xFF] ^
					Crc32Lookup[15][one & 0xFF];
#endif
			}

			length -= BytesAtOnce;
		}

		const uint8_t* currentChar = (const uint8_t*)current;
		// remaining 1 to 63 bytes (standard algorithm)
		while (length-- != 0)
			crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

		return ~crc; // same as crc ^ 0xFFFFFFFF
	}


	/// compute CRC32 (Slicing-by-16 algorithm, prefetch upcoming data blocks)
	inline uint32_t crc32_16bytes_prefetch(const void* data, size_t length, uint32_t previousCrc32 = 0, size_t prefetchAhead = 256)
	{
		// CRC code is identical to crc32_16bytes (including unrolling), only added prefetching
		// 256 bytes look-ahead seems to be the sweet spot on Core i7 CPUs

		uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
		const uint32_t* current = (const uint32_t*)data;

		// enabling optimization (at least -O2) automatically unrolls the for-loop
		const size_t Unroll = 4;
		const size_t BytesAtOnce = 16 * Unroll;

		while (length >= BytesAtOnce + prefetchAhead)
		{
			PREFETCH(((const char*)current) + prefetchAhead);

			for (size_t unrolling = 0; unrolling < Unroll; unrolling++)
			{
#if __BYTE_ORDER == __BIG_ENDIAN
				uint32_t one = *current++ ^ swap(crc);
				uint32_t two = *current++;
				uint32_t three = *current++;
				uint32_t four = *current++;
				crc = Crc32Lookup[0][four & 0xFF] ^
					Crc32Lookup[1][(four >> 8) & 0xFF] ^
					Crc32Lookup[2][(four >> 16) & 0xFF] ^
					Crc32Lookup[3][(four >> 24) & 0xFF] ^
					Crc32Lookup[4][three & 0xFF] ^
					Crc32Lookup[5][(three >> 8) & 0xFF] ^
					Crc32Lookup[6][(three >> 16) & 0xFF] ^
					Crc32Lookup[7][(three >> 24) & 0xFF] ^
					Crc32Lookup[8][two & 0xFF] ^
					Crc32Lookup[9][(two >> 8) & 0xFF] ^
					Crc32Lookup[10][(two >> 16) & 0xFF] ^
					Crc32Lookup[11][(two >> 24) & 0xFF] ^
					Crc32Lookup[12][one & 0xFF] ^
					Crc32Lookup[13][(one >> 8) & 0xFF] ^
					Crc32Lookup[14][(one >> 16) & 0xFF] ^
					Crc32Lookup[15][(one >> 24) & 0xFF];
#else
				uint32_t one = *current++ ^ crc;
				uint32_t two = *current++;
				uint32_t three = *current++;
				uint32_t four = *current++;
				crc = Crc32Lookup[0][(four >> 24) & 0xFF] ^
					Crc32Lookup[1][(four >> 16) & 0xFF] ^
					Crc32Lookup[2][(four >> 8) & 0xFF] ^
					Crc32Lookup[3][four & 0xFF] ^
					Crc32Lookup[4][(three >> 24) & 0xFF] ^
					Crc32Lookup[5][(three >> 16) & 0xFF] ^
					Crc32Lookup[6][(three >> 8) & 0xFF] ^
					Crc32Lookup[7][three & 0xFF] ^
					Crc32Lookup[8][(two >> 24) & 0xFF] ^
					Crc32Lookup[9][(two >> 16) & 0xFF] ^
					Crc32Lookup[10][(two >> 8) & 0xFF] ^
					Crc32Lookup[11][two & 0xFF] ^
					Crc32Lookup[12][(one >> 24) & 0xFF] ^
					Crc32Lookup[13][(one >> 16) & 0xFF] ^
					Crc32Lookup[14][(one >> 8) & 0xFF] ^
					Crc32Lookup[15][one & 0xFF];
#endif
			}

			length -= BytesAtOnce;
		}

		const uint8_t* currentChar = (const uint8_t*)current;
		// remaining 1 to 63 bytes (standard algorithm)
		while (length-- != 0)
			crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

		return ~crc; // same as crc ^ 0xFFFFFFFF
	}


	/// compute CRC32 using the fastest algorithm for large datasets on modern CPUs
	inline uint32_t crc32_fast(const void* data, size_t length, uint32_t previousCrc32 = 0)
	{
		return crc32_16bytes(data, length, previousCrc32);
	}
}