/* @file lib/stdint.h
 * @author matthewj S2008
 * @brief Standard integer type definitions
 */

#ifndef LIB_STDINT_H
#define LIB_STDINT_H

#ifndef ASSEMBLER

typedef unsigned char u8;
typedef unsigned char uint8;
typedef unsigned char uint8_t;

typedef unsigned short u16;
typedef unsigned short uint16_t;

typedef unsigned int u32;
typedef unsigned int uint32_t;

typedef unsigned long long uint64_t;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed char int8_t;

typedef signed short s16;
typedef signed short int16_t;

typedef signed int s32;
typedef signed int int32_t;
typedef signed long long int64_t;


#endif /* !ASSEMBLER */

#endif /* !LIB_STDINT_H */
