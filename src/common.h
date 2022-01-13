#pragma once
#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "cmath"

using u64 = uint64_t;
using i64 = int64_t;
using u32 = uint32_t;
using i32 = int32_t;
using u16 = uint16_t;
using i16 = int16_t;
using u8 = uint8_t;
using i8 = int8_t;

using f32 = float;
using f64 = double;

using byte = uint8_t;
using bool8 = int8_t;
using bool32 = int32_t;


#define Assert(expression) if (!(expression)) { \
    printf("Assertion Failed\nExpression: %s | Location: %s(%d)", #expression, __FILE__, __LINE__); abort();}

#define ForCount(int_name, count) for (u32 int_name = 0; int_name < count; int_name++)  // for arrays

#define ArraySize(c_array) ((u32)(sizeof(c_array)/sizeof(*c_array))) 
