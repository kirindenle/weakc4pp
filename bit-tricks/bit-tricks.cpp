#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define ROWS 6
#define COLS 7

const u64 offset = 2;
const u64 stride = 8;
u64 coordsToBit(u64 x, u64 y)
{
  return 1ULL << (stride * x + offset + y);
}

void printMask(u64 mask)
{
  char str[COLS*ROWS + ROWS + 1];
  int i = 0;
  int newlines = 0;
  for (u64 y = ROWS - 1; y < ROWS; --y)
  {
    for (u64 x = 0; x < COLS; ++x)
    {
      bool bit = mask & coordsToBit(x, y);
      str[newlines + (i++)] = bit ? 'x' : '.';
    }
    str[(newlines++) + i] = '\n';
  }
  str[newlines + i] = 0;
  printf("%hs", str);
}

void generateConnectFourMasks()
{
  u64 masks[COLS][ROWS][4];
  const s8 deltas[][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
  printf("//offset = %llu, stride = %llu\n", offset, stride);
  printf("const u64 lookupConnectFourExtractMask[COLS][ROWS][4] =\n{\n");
  for (u64 x = 0; x < COLS; ++x)
  {
    printf("  {\n");
    for (u64 y = 0; y < ROWS; ++y)
    {
      printf("    {");
      int i = 0;
      for (auto [xDir, yDir] : deltas)
      {
        u64 mask = 0;
        const static s8 signs[] = {1, -1};
        for (s8 sign : signs)
        {
          u8 dx = sign * xDir;
          u8 dy = sign * yDir;
          u8 nx = x + dx;
          u8 ny = y + dy;
          for (int i = 1; nx < COLS && ny < ROWS && i <= 3; ++i, nx += dx, ny += dy)
          {
            mask |= coordsToBit(nx, ny);
          }
        }
        masks[x][y][i] = mask;
        printf("0x%llx%hs", mask, i == 3 ? "" : ",");
        //printf("x = %llu, y = %llu, (%d,%d):\n", x, y, xDir, yDir);
        //printMask(mask);
        i += 1;
      }
      printf("}%hs", y == ROWS - 1 ? "\n" : ",\n");
    }
    printf("  },\n");
  }
  printf("};\n");
}

void generateThreeInARowMask()
{
  u64 mask = 0;
  for (u64 i = 0; i < 64; ++i)
  {
    u64 bit = 1ULL << i;
    u64 threeInARow = 0b111;
    for (u64 j = 0; j < 4; ++j)
    {
      u64 current = threeInARow << j;
      if ((i & current) == current)
      {
        printf("// i = %d\n", (int)i);
        mask |= bit;
        break;
      }
    }
  }
  char buffer[65];
  _ui64toa(mask, buffer, 2);
  printf("const u64 lookupThreeInARow = 0b%hs;\n", buffer);
}

int main()
{
  generateConnectFourMasks();
  generateThreeInARowMask();
  return 0;
}
