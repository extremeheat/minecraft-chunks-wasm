#pragma once

using u64 = unsigned long long;
using u32 = unsigned int;
using u16 = unsigned short;
using u8 = unsigned char;
using i64 = long long;
using i32 = int;
using i16 = short;
using i8 = char;
using byte = u8;

#define FLOOR(V) (int)V - (V < (int)V)
#define CEIL(x) (int)(x) + ((x) > (int)(x));
#define ABS(x) (x < 0) ? -x : x
#define out

struct Vec3 {
   int x;
   int y;
   int z;

   int operator==(const Vec3 &other) {
      return x == other.x && y == other.y && z == other.z;
   }
};
