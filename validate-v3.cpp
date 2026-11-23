#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// TODO: make this .c
#ifdef _MSC_VER
#include <intrin.h>
//#pragma comment(linker, "/STACK:134217728,4096") // Reserve 128 MiB, commit 4 KiB
static u64 deposit64(u64 source, u64 mask)
{
  return _pdep_u64(source, mask);
}
static u64 extract64(u64 source, u64 mask)
{
  return _pext_u64(source, mask);
}
#endif

#define BITS64(B7, B6, B5, B4, B3, B2, B1, B0) 0b##B7##B6##B5##B4##B3##B2##B1##B0

u64 testNumber = BITS64(00000000,
                        00000000,
                        00000000,
                        00000000,
                        00000000,
                        00000000,
                        00000000,
                        00000000);

#if 0
....... ....... ....... ....... ....... ....... .......
.x..... ..x.... ...x... ....x.. .....x. ......x .......
..x.... ...x... ....x.. .....x. ......x ....... .......
...x... ....x.. .....x. ......x ....... ....... .......
....... ....... ....... ....... ....... ....... .......
....... ....... ....... ....... ....... ....... .......

....... x...... .x..... ..x.... ...x... ....x.. .....x.
....... ....... ....... ....... ....... ....... .......
.x..... ..x.... ...x... ....x.. .....x. ......x .......
..x.... ...x... ....x.. .....x. ......x ....... .......
...x... ....x.. .....x. ......x ....... ....... .......
....... ....... ....... ....... ....... ....... .......

....... ....... x...... .x..... ..x.... ...x... ....x..
....... x...... .x..... ..x.... ...x... ....x.. .....x.
....... ....... ....... ....... ....... ....... .......
.x..... ..x.... ...x... ....x.. .....x. ......x .......
..x.... ...x... ....x.. .....x. ......x ....... .......
...x... ....x.. .....x. ......x ....... ....... .......

....... ....... ....... x...... .x..... ..x.... ...x...
....... ....... x...... .x..... ..x.... ...x... ....x..
....... x...... .x..... ..x.... ...x... ....x.. .....x.
....... ....... ....... ....... ....... ....... .......
.x..... ..x.... ...x... ....x.. .....x. ......x .......
..x.... ...x... ....x.. .....x. ......x ....... .......

....... ....... ....... ....... ....... ....... .......
....... ....... ....... x...... .x..... ..x.... ...x...
....... ....... x...... .x..... ..x.... ...x... ....x..
....... x...... .x..... ..x.... ...x... ....x.. .....x.
....... ....... ....... ....... ....... ....... .......
.x..... ..x.... ...x... ....x.. .....x. ......x .......

....... ....... ....... ....... ....... ....... .......
....... ....... ....... ....... ....... ....... .......
....... ....... ....... x...... .x..... ..x.... ...x...
....... ....... x...... .x..... ..x.... ...x... ....x..
....... x...... .x..... ..x.... ...x... ....x.. .....x.
....... ....... ....... ....... ....... ....... .......
#endif

static u64 readTSC()
{
  return __rdtsc();
}

#if 0
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#define MEASURE_PERF 1

#define global static

#define PERF_COUNTERS(X)\
  X(AllToAll)\
  X(MakesFour)\
  X(QueryState)\
  X(QueryStateOld)\
  X(VerifyLeafs)\

enum
{
#define DECLARE_PERF_COUNTER(N) N,
  PERF_COUNTERS(DECLARE_PERF_COUNTER)
#undef DECLARE_PERF_COUNTER
};

#define PLUS_ONE(C) +1
#define NUM_PERF_COUNTERS (0 PERF_COUNTERS(PLUS_ONE))
struct PerformanceStatistics
{
  u64 ticks = 0;
  u64 hits = 0;
};
global PerformanceStatistics perfStats[NUM_PERF_COUNTERS];
global char const* perfZonesNames[NUM_PERF_COUNTERS] =
{
#define DECLARE_STRING(N) #N,
  PERF_COUNTERS(DECLARE_STRING)
};
const double cpuFrequencyEstimate = 3.5e+9;

#if MEASURE_PERF
#define PERF_ZONE(N) struct Defer##N { u64 start; ~Defer##N() { u64 end = readTSC(); perfStats[N].ticks += (end - start); perfStats[N].hits += 1; } } deferObject##N; deferObject##N.start = readTSC()
#else
#define PERF_ZONE(N)
#endif

#define ROWS 6
#define COLS 7

// TODO: use 2 bits
typedef u8 Player;
const Player Empty = 0;
const Player Red = 1;
const Player Yellow = 2;

static char char_from_player(Player player)
{
  switch (player)
  {
    case Empty: return ' ';
    case Red: return 'X';
    case Yellow: return 'O';
  }
  return '?';
}

// TODO: make these structs for type safety
typedef u8 Column; // Move - 1
typedef u8 Row;
typedef char Level;
typedef u8 Move; // Column + 1
typedef s16 BranchType;

struct Position
{
  char const* moves;
  s32 movesCount;
};

#if 0
#else
struct Board
{
  u64 data;
};

static Row col_height(Board board, Column x)
{
  static_assert(COLS == 7 && ROWS == 6);
  u64 columnOffset = x * 8ULL;
  u64 bits = board.data;
  u64 hLow = (bits >> columnOffset) & 0b11ULL; // lowest 2 bits in the Xth byte are low 2 bits of height
  u64 hHigh = (bits >> (7ULL*8ULL - 2ULL + x)) & 0b100; // Xth bit in the byte 7 is the high bit of height
  return hLow | hHigh;
}

static Player getPlayer(Board b, Column x, Row y)
{
  static_assert(COLS == 7 && ROWS == 6);
  Row h = col_height(b, x);
  u64 bits = b.data;
  const u64 columnStart = x * 8ULL + 2;
  if (y < h)
  {
    return 1 + ((bits >> (columnStart + y)) & 1); // 0 bit is Red, 1 bit is Yellow
  }
  return Empty;
  //return b._cells[y][x];
}

static u64 getHMask(Column x)
{
  static_assert(COLS == 7 && ROWS == 6);
  u64 columnOffset = x * 8;
  return (0b11ULL << columnOffset) | (1ULL << (7ULL * 8ULL + x));
}

static bool boardIsGood(Board b)
{
  static_assert(COLS == 7 && ROWS == 6);
  for (Column x = 0; x < COLS; ++x)
  {
    Row h = col_height(b, x);
    if (h > ROWS)
    {
      return false;
    }
    for (Row y = h; y < ROWS; ++y)
    {
      if (b.data & (1ULL << (x * 8ULL + 2 + y)))
      {
        return false;
      }
    }
  }
  if (b.data & 0x8000000000000000)
  {
    return false;
  }
  return true;
}

// TODO: never set empty, also get rid of y, and remove p - 1
static void setPlayer(Board *b, Column x, Row y, Player p)
{
  static_assert(COLS == 7 && ROWS == 6);
#if 0
  if (!boardIsGood(*b))
  {
    fprintf(stderr, "boardIsBad\n");
    exit(-1);
  }
#endif
  Row h = col_height(*b, x);
  if (x >= 7 || y >= 6 || (p == Empty && y + 1 != h) || (p != Empty && y != h) || (p == Empty && h == 0))
  {
    fprintf(stderr, "bad call to setPlayer h = %d, x = %d, y = %d, player = %c\n", h, x, y, char_from_player(p));
    exit(-1);
  }
  u64 oldBits = b->data;
  u64 newBits = b->data;
  const u64 hmask = getHMask(x);
  const u64 columnStart = x * 8ULL + 2;
  newBits &= ~hmask;
  u64 newH = 0xCDCDCDCDCDCDCDCD;
  switch (p)
  {
    case Empty:
    {
      newH = (h - 1) & 0b111ULL;
      newBits &= ~(1ULL << (columnStart + (h - 1))); // to guarantee we have only zeros above height
      newBits |= deposit64(newH, hmask);
    } break;
    case Red:
    case Yellow:
    {
      newH = (h + 1) & 0b111ULL;
      newBits |= deposit64(newH, hmask);
      u64 bit = (p - 1) & 0b1ULL;
      newBits |= (bit << (columnStart + (newH - 1)));
    } break;
  }
  b->data = newBits;
#if 0
  if (!boardIsGood(*b))
  {
    fprintf(stderr, "boardIsBad\n");
    exit(-1);
  }
#endif
  //b->_cells[y][x] = p;
}

static bool is_full_board(Board board)
{
  u64 bits = board.data;
  return (bits & 0b01111111'00000011'00000011'00000011'00000011'00000011'00000011'00000011)
              == 0b01111111'00000010'00000010'00000010'00000010'00000010'00000010'00000010;
}
#endif

static const Level LEVEL_CHARS[] = "0123456789";

// TODO: diagram is a known constant, and there are merely 384 of them (not including mirrored), we can remove an argument and template everything by a diagram
struct Diagram
{
  Level cells[ROWS][COLS];
};

const BranchType TypeMove = 0;
const BranchType TypeSteadyStateIndex = 1;
struct BranchMove
{
  BranchType type; // Move or State
  s16 value; // move is a digit char, from '0' to '9', steady state index is from 0 to count of steady states
};

struct Branch
{
  char const* moves;
  s32 movesCount;
  BranchMove state;
};

static Position get_position(Branch branch)
{
  return {branch.moves, branch.movesCount};
}

static bool is_steady_state_entry(BranchMove state)
{
  return state.type == TypeSteadyStateIndex;
}

// TODO: simpler code for this... maybe just one fread is enough
void loadFile(char const* path, char const* bufferFile, const int bufferFileMaxCount, int* fileCount)
{
  FILE* f = fopen(path, "rb");
  if (!f)
  {
    fprintf(stderr, "can't open file %hs\n", path);
    exit(-1);
  }
  int resultFileCount = 0;
  while (!feof(f))
  {
    size_t count = fread((void*)(bufferFile + resultFileCount), sizeof(char), bufferFileMaxCount - resultFileCount, f);
    if (ferror(f))
    {
      fprintf(stderr, "fread of file %hs errored\n", path);
      fclose(f);
      exit(-1);
    }
    resultFileCount += count;
  }
  fclose(f);
  if (resultFileCount == bufferFileMaxCount)
  {
    fprintf(stderr, "file %hs is too big\n", path);
    exit(-1);
  }
  *fileCount = resultFileCount;
}

#define MAX_BRANCHES 1000
#define MAX_STEADY_STATES 1000
#define OUGHT_TO_BE_ENOUGH 640000

struct Files
{
  int branchesFileCount = 0;
  char branchesFile[OUGHT_TO_BE_ENOUGH];
  int steadyStatesFileCount = 0;
  char steadyStatesFile[OUGHT_TO_BE_ENOUGH];
};

struct ConnectFourSolution
{
  s32 branchesCount = 0;
  Branch branches[MAX_BRANCHES];
  s32 steadyStatesCount = 0;
  Diagram steadyStates[MAX_STEADY_STATES];
};
global Files originalFiles;
global ConnectFourSolution originalSolution;

static void addBranch(ConnectFourSolution& solution, char const* moves, int movesCount, BranchType branchType, s16 branchValue)
{
  if (solution.branchesCount >= MAX_BRANCHES)
  {
    fprintf(stderr, "too many branches\n");
    exit(-1);
  }
  if (branchType == TypeMove)
  {
    if (branchValue < '1' || '7' < branchValue)
    {
      fprintf(stderr, "trying to add bad move %c (which is %d)\n", (char)branchValue, branchValue);
      exit(-1);
    }
  }
  solution.branches[solution.branchesCount].moves = moves;
  solution.branches[solution.branchesCount].movesCount = movesCount;
  solution.branches[solution.branchesCount].state.type = branchType;
  solution.branches[solution.branchesCount].state.value = branchValue;
  solution.branchesCount += 1;
}

static void parseBranches(ConnectFourSolution& solution, char *branchesFile, int branchesFileCount)
{
  s32 positionStart = -1;
  s32 positionCount = 0;
  enum { LookingForPosition, FillingPosition, LookingForMove, FillingState } parseState = LookingForPosition;
  s32 steadyStateIndex = -1;
  char prevc = '\n';
  for (s32 i = 0; i < branchesFileCount; ++i)
  {
    char c = branchesFile[i];
    if (c == '-')
    {
      fprintf(stderr, "branches json file can't contain minuses\n");
      exit(-1);
    }
    switch (parseState)
    {
      case LookingForPosition:
      {
        if (c == '"' && prevc == ' ')
        {
          positionStart = i + 1;
          positionCount = 0;
          parseState = FillingPosition;
        }
      } break;
      case FillingPosition:
      {
        bool isMoveDigit = '1' <= c && c <= '7';
        if (isMoveDigit)
        {
          positionCount += 1;
        }
        else
        {
          parseState = LookingForMove;
        }
      } break;
      case LookingForMove:
      {
        bool isDigit = '0' <= c && c <= '9';
        if (isDigit)
        {
          if (prevc == '"')
          {
            addBranch(solution, &branchesFile[positionStart], positionCount, TypeMove, c);
            parseState = LookingForPosition;
          }
          else
          {
            steadyStateIndex = c - '0';
            parseState = FillingState;
          }
        }
      } break;
      case FillingState:
      {
        bool isDigit = '0' <= c && c <= '9';
        if (isDigit)
        {
          steadyStateIndex *= 10;
          steadyStateIndex += (c - '0'); // TODO: check overflow?
        }
        else
        {
          addBranch(solution, &branchesFile[positionStart], positionCount, TypeSteadyStateIndex, steadyStateIndex);
          parseState = LookingForPosition;
        }
      } break;
    }
    prevc = c;
  }
  if (parseState != LookingForPosition)
  {
    fprintf(stderr, "parsing failed parseState = %d\n", parseState);
    exit(-1);
  }
}

static void parseSteadyStates(ConnectFourSolution& solution, char *steadyStatesFile, int steadyStatesFileCount)
{
  Column x = 0;
  Row yt = 0;
  s32 digitsCollected = 0;
  for (s32 i = 0; i < steadyStatesFileCount; ++i)
  {
    char c = steadyStatesFile[i];
    if (c < '0' || '9' < c)
    {
      continue;
    }
    solution.steadyStates[solution.steadyStatesCount].cells[yt][x] = c;
    digitsCollected += 1;
    if (digitsCollected == ROWS * COLS)
    {
      digitsCollected = 0;
      solution.steadyStatesCount += 1;
    }
    // I think this would be a cool function
    x += 1;
    if (x == COLS)
    {
      x = 0;
      yt += 1;
      if (yt == ROWS)
      {
        yt = 0;
      }
    }
  }
  if (digitsCollected || x || yt || solution.steadyStatesCount == 0)
  {
    fprintf(stderr, "error parsing steady stated (digits count in the file is not a multiple of ROWS * COLS), x = %d, yt = %d, digitsCollected = %d\n", x, yt, digitsCollected);
    exit(-1);
  }
}

static void loadBranches(ConnectFourSolution& solution, Files& files, char const* path)
{
  loadFile(path, files.branchesFile, sizeof files.branchesFile, &files.branchesFileCount);
  parseBranches(solution, files.branchesFile, files.branchesFileCount);
}

static void loadSteadyStates(ConnectFourSolution& solution, Files& files, char const* path)
{
  loadFile(path, files.steadyStatesFile, sizeof files.steadyStatesFile, &files.steadyStatesFileCount);
  parseSteadyStates(solution, files.steadyStatesFile, files.steadyStatesFileCount);
}

static void print_board(Board board, char const* indent)
{
  for (Row yPlus = ROWS; yPlus != 0; --yPlus)
  {
    static char row[COLS];
    Row y = yPlus - 1;
    for (Column x = 0; x < COLS; ++x)
    {
      row[x] = char_from_player(getPlayer(board, x, y));
    }
    printf("%hs|%.*hs|\n", indent, COLS, row);
  }
  printf("%hs.-------.\n", indent);
}

static Board board_from_position(Position position, bool mirrored = false)
{
  DEBUG_PRINT("board_from_position:\n");
  Board board = {};
  for (int i = 0; i < position.movesCount; ++i)
  {
    Column x = position.moves[i] - '1';
    if (mirrored)
    {
      x = 6 - x;
    }
    DEBUG_PRINT("  i=%d x=%d\n", i, x);
    // TODO: ignore this?
    auto h = col_height(board, x);
    if (x >= COLS || h >= ROWS)
    {
      fprintf(stderr, "illegal position %.*s\n", position.movesCount, position.moves);
      exit(1);
    }
    setPlayer(&board, x, h, (i & 1) + 1);
  }
  return board;
}

static bool makes_four_if_put_player_old(Board board, Column x, Row y, Player player)
{
  PERF_ZONE(MakesFour);
  DEBUG_PRINT("makes_four %d %d %c:\n", x, y, char_from_player(player));
  const s8 deltas[][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
  for (auto [dx, dy] : deltas)
  {
    //printf(" %+d %+d\n", dx, dy);
    u8 count = 1;
    const static s8 signs[] = {1, -1};
    for (s8 sign : signs)
    {
      //printf("  %+d\n", sign);
      u8 nx = x + sign * dx;
      u8 ny = y + sign * dy;
      while (nx < COLS && ny < ROWS && getPlayer(board, nx, ny) == player)
      {
        //printf("   %d %d %d\n", nx, ny, count);
        count += 1;
        nx += sign * dx;
        ny += sign * dy;
      }
      if (count >= 4)
      {
        return true;
      }
    }
  }
  return false;
}

//#include "bit-tricks/generated.cpp"

//offset = 2, stride = 8
const u64 lookupConnectFourExtractMask[COLS][ROWS][4] =
{
  {
    {0x4040400,0x38,0x20100800,0x0},
    {0x8080800,0x74,0x40201000,0x400},
    {0x10101000,0xec,0x80402000,0x40800},
    {0x20202000,0xdc,0x804000,0x4081000},
    {0x40404000,0xb8,0x8000,0x8102000},
    {0x80808000,0x70,0x0,0x10204000}
  },
  {
    {0x404040004,0x3800,0x2010080000,0x8},
    {0x808080008,0x7400,0x4020100004,0x40010},
    {0x1010100010,0xec00,0x8040200008,0x4080020},
    {0x2020200020,0xdc00,0x80400010,0x408100040},
    {0x4040400040,0xb800,0x800020,0x810200080},
    {0x8080800080,0x7000,0x40,0x1020400000}
  },
  {
    {0x40404000404,0x380000,0x201008000000,0x810},
    {0x80808000808,0x740000,0x402010000400,0x4001020},
    {0x101010001010,0xec0000,0x804020000804,0x408002040},
    {0x202020002020,0xdc0000,0x8040001008,0x40810004080},
    {0x404040004040,0xb80000,0x80002010,0x81020008000},
    {0x808080008080,0x700000,0x4020,0x102040000000}
  },
  {
    {0x4040400040404,0x38000000,0x20100800000000,0x81020},
    {0x8080800080808,0x74000000,0x40201000040000,0x400102040},
    {0x10101000101010,0xec000000,0x80402000080400,0x40800204080},
    {0x20202000202020,0xdc000000,0x804000100804,0x4081000408000},
    {0x40404000404040,0xb8000000,0x8000201008,0x8102000800000},
    {0x80808000808080,0x70000000,0x402010,0x10204000000000}
  },
  {
    {0x4040004040400,0x3800000000,0x10080000000000,0x8102000},
    {0x8080008080800,0x7400000000,0x20100004000000,0x40010204000},
    {0x10100010101000,0xec00000000,0x40200008040000,0x4080020408000},
    {0x20200020202000,0xdc00000000,0x80400010080400,0x8100040800000},
    {0x40400040404000,0xb800000000,0x800020100800,0x10200080000000},
    {0x80800080808000,0x7000000000,0x40201000,0x20400000000000}
  },
  {
    {0x4000404040000,0x380000000000,0x8000000000000,0x810200000},
    {0x8000808080000,0x740000000000,0x10000400000000,0x4001020400000},
    {0x10001010100000,0xec0000000000,0x20000804000000,0x8002040800000},
    {0x20002020200000,0xdc0000000000,0x40001008040000,0x10004080000000},
    {0x40004040400000,0xb80000000000,0x80002010080000,0x20008000000000},
    {0x80008080800000,0x700000000000,0x4020100000,0x40000000000000}
  },
  {
    {0x40404000000,0x38000000000000,0x0,0x81020000000},
    {0x80808000000,0x74000000000000,0x40000000000,0x102040000000},
    {0x101010000000,0xec000000000000,0x80400000000,0x204080000000},
    {0x202020000000,0xdc000000000000,0x100804000000,0x408000000000},
    {0x404040000000,0xb8000000000000,0x201008000000,0x800000000000},
    {0x808080000000,0x70000000000000,0x402010000000,0x0}
  },
};
const u64 lookupThreeInARow = 0b1111111110000000110000001000000011110000100000001100000010000000;
static void debugBoard(u64 bits, char const* name)
{
  char str[64 + 8 - 1 + 8 + 1];
  int newlines = 0;
  int spaces = 0;
  int chars = 0;
  for (u64 i = 7; i < 8; --i)
  {
    u8 b = (bits >> (i*8)) & 0xFF;
    for (u64 bit = 7; bit < 8; --bit)
    {
      bool one = b & (1 << bit);
      str[(chars++) + newlines + spaces] = one ? '1' : '0';
      if (bit == 2 && i != 7)
      {
        str[chars + newlines + (spaces++)] = ' ';
      }
    }
    str[chars + (newlines++) + spaces] = '\n';
  }
  str[chars + newlines + spaces] = 0;
  fprintf(stderr, "%hs:\n%hs", name, str);
}
static bool makes_four_if_put_player(Board board, Column x, Row y, Player player)
{
  PERF_ZONE(MakesFour);
  // prepare:
  u64 bits = board.data ^ (player == Red ? 0xFFFFFFFFFFFFFFFF : 0); // mabe do player - 2, or store the player as Red = -1ULL, Yellow = 0ULL
  u64 heightsMask = 0;
  // TODO: cache heightsMask
  for (Column x = 0; x < COLS; ++x)
  {
    heightsMask |= (((1ULL << col_height(board, x)) - 1) << 2ULL) << (8 * x);
  }
  bits &= heightsMask;
  bool result = false;
  for (u64 dirMask : lookupConnectFourExtractMask[x][y])
  {
    u64 extractedBits = extract64(bits, dirMask);
    if ((lookupThreeInARow >> extractedBits) & 1)
    {
      result = true;
      break;
    }
  }
#if 0
  bool old = makes_four_if_put_player_old(board, x, y, player);
  if (old != result)
  {
    //__debugbreak();
    fprintf(stderr, "error on board = 0x%llx, x = %d, y = %d, player = %d, old = %d, new = %d, heigthsMask = 0x%llx\n", board.data, x, y, player, old, result, heightsMask);
    debugBoard(bits, "bits");
    debugBoard(board.data, "board");
    debugBoard(heightsMask, "heightsMask");
    for (int i = 0; i < 4; ++i)
    {
      u64 dirMask = lookupConnectFourExtractMask[x][y][i];
      debugBoard(dirMask, "dirMask");
      fprintf(stderr, "0x%llx\n", dirMask);
      u64 extractedBits = extract64(bits, dirMask);
      debugBoard(extractedBits, "extracted");
      fprintf(stderr, "shifted = 0x%llx\n", (lookupThreeInARow >> extractedBits));
    }
    for (Column x = 0; x < COLS; ++x)
    {
      auto h = col_height(board, x);
      fprintf(stderr, "h[%d] = %d\n", x, h);
    }
    exit(-1);
  }
#endif
  return result;
}

static bool board_has_four(Board board)
{
  for (Row y = 0; y < ROWS; ++y)
  {
    for (Column x = 0; x < COLS; ++x)
    {
      auto player = getPlayer(board, x, y);
      if (player != Empty && makes_four_if_put_player(board, x, y, player))
      {
        return true;
      }
    }
  }
  return false;
}

static bool wins(Column x, Player player, Board board)
{
  Row y = col_height(board, x);
  if (y >= ROWS)
  {
    return false;
  }
  bool won = makes_four_if_put_player(board, x, y, player);
  return won;
}

struct Playable
{
  Move moves[COLS];
  u8 movesCount;
};

// TODO: replace with get_one_playable_by_level that will early return 0 if more than one move is playable
static Playable playable(Level level_char, Board board, Diagram diagram)
{
  DEBUG_PRINT("playable:\n");
  Playable found = {};
  for (Column x = 0; x < COLS; ++x)
  {
    Row y = col_height(board, x);
    if (y >= ROWS)
    {
      continue;
    }
    int yt = ROWS - 1 - y;
    if (diagram.cells[yt][x] == level_char)
    {
      DEBUG_PRINT("  add move %d\n", x + 1);
      found.moves[found.movesCount++] = x + 1;
    }
  }
  return found;
}

static Move query_steady_state(Board board, Diagram diagram)
{
  PERF_ZONE(QueryState);
  static const Player players[2] = {Red, Yellow};
  for (Player player : players)
  {
    for (Column x = 0; x < COLS; ++x)
    {
      Row y = col_height(board, x);
      if (y >= ROWS)
      {
        continue;
      }
      bool won = makes_four_if_put_player(board, x, y, player);
      if (won)
      {
        return x + 1;
      }
    }
  }
  s64 counts[10] = {};
  Move moves[10] = {};
  for (Column x = 0; x < COLS; ++x)
  {
    Row y = col_height(board, x);
    if (y >= ROWS)
    {
      continue;
    }
    int yt = ROWS - 1 - y;
    Level level = diagram.cells[yt][x];
    auto index = level - '0';
    counts[index] += 1;
    moves[index] = x + 1;
  }
  for (int i = 0; i < 10; ++i)
  {
    if (counts[i] == 1)
    {
      return moves[i];
    }
  }
  // we use 0 instead of None analogue, because 0 is an invalid move
  return 0;
}

static Move query_steady_state_old(Board board, Diagram diagram)
{
  PERF_ZONE(QueryStateOld);
  static const Player players[2] = {Red, Yellow};
  for (Player player : players)
  {
    for (Column x = 0; x < COLS; ++x)
    {
      if (wins(x, player, board))
      {
        return x + 1;
      }
    }
  }
  s64 counts[10] = {};
  Move moves[10] = {};
  for (Column x = 0; x < COLS; ++x)
  {
    Row y = col_height(board, x);
    if (y >= ROWS)
    {
      continue;
    }
    int yt = ROWS - 1 - y;
    Level level = diagram.cells[yt][x];
    auto index = level - '0';
    counts[index] += 1;
    moves[index] = x + 1;
  }
  for (int i = 0; i < 10; ++i)
  {
    if (counts[i] == 1)
    {
      return moves[i];
    }
  }
  // we use 0 instead of None analogue, because 0 is an invalid move
  return 0;
}

// A set of boards for which red_turn returns true
#define MAX_MEMO (1 << 20)
struct Memo
{
  Diagram diagram; // memo for this diagram
  int boardsCount = 0;
  struct { bool occupied; Board board; } table[MAX_MEMO]; // TODO: a hash table
};
static u32 jenkins(const u8* key, int length)
{
    int i = 0;
    u32 hash = 0;
    while (i != length) {
        hash += key[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}
static u32 hash(Board board) {
    return jenkins((const u8*)&board, sizeof board) & (MAX_MEMO - 1);
}
static bool in_memo(Board board, Memo* memo)
{
  u32 h = hash(board);
  u32 i = h;
  while (memo->table[i].occupied)
  {
    if (memcmp(&memo->table[i].board, &board, sizeof(Board)) == 0) // TODO: use == after we compress the board
    {
      return true;
    }
    i += 1;
    i &= (MAX_MEMO - 1);
  }
  return false;
}
// only call this when you know it is not in the memo
static void add_to_memo(Board board, Memo* memo)
{
  u32 h = hash(board);
  u32 i = h;
  while (memo->table[i].occupied)
  {
    i += 1;
    i &= (MAX_MEMO - 1);
  }
  memo->table[i].occupied = true;
  memo->table[i].board = board;
  memo->boardsCount += 1;
}

global Memo memo;
static bool red_turn(Board board)
{
  Board key = board;
  if (in_memo(key, &memo))
  {
    return true;
  }
  Move move = query_steady_state(board, memo.diagram);
  if (move == 0)
  {
    return false;
  }
  Column x = move - 1;
  Row y = col_height(board, x);
  if (y >= ROWS)
  {
    return false;
  }
  setPlayer(&board, x, y, Red);
  {
    if (makes_four_if_put_player(board, x, y, Red))
    {
      add_to_memo(key, &memo);
      return true;
    }
    if (is_full_board(board))
    {
      return false;
    }
    // TODO: col_height speed...
    for (Column yx = 0; yx < COLS; ++yx)
    {
      Column yy = col_height(board, yx);
      if (yy >= ROWS)
      {
        continue;
      }
      setPlayer(&board, yx, yy, Yellow);
      {
        if (makes_four_if_put_player(board, yx, yy, Yellow))
        {
          return false;
        }
        if (is_full_board(board))
        {
          return false;
        }
        // TODO: get rid of recursion
        if (!red_turn(board))
        {
          return false;
        }
      }
      setPlayer(&board, yx, yy, Empty);
    }
  }
  //setPlayer(&board, x, y, Empty); // we pass by value so we don't need this yet, maybe I'll have to pass by a pointer to test before even optimizing
  add_to_memo(key, &memo);
  return true;
}

static bool verify_leaf(Diagram diagram, Board board)
{
  // TODO: don't use static for multithreaded version, just allocate memo or use thread local storage, or create a lots of memos for all threads outside
  memset(&memo, 0, sizeof memo);
  memo.diagram = diagram;
  if (board_has_four(board))
  {
    printf("!!!!!!!!!!!!! leaf has four !!!!!!!!!!!!!!!!\n");
    return false;
  }
  return red_turn(board);
}

#define MAX_POSITIONS 700
struct BranchIndexes
{
  s16 branchIndexesCount = 0;
  s16 branchIndexes[MAX_POSITIONS];
};

struct SteadyStateToAllBranchesItSolves
{
  BranchIndexes table[MAX_STEADY_STATES];
};

struct SteadyStates
{
  s16 statesCount = 0;
  s16 states[MAX_STEADY_STATES];
};

#define TRY_NON_LEAF_BRANCHES 0
// NOTE: we use globals here because we don't want to pass them to red_turn cause it is recursive (and we try to reduce stack size of exponential search)
// TODO: tru passing them anyway
static void collectAllPositionsForAllSteadyStates(SteadyStateToAllBranchesItSolves& allToAll, SteadyStateToAllBranchesItSolves& allToMirrored)
{
  int mirroredAdditions = 0;
  for (s16 steadyStateIndex = 0; steadyStateIndex < originalSolution.steadyStatesCount; ++steadyStateIndex)
  {
    PERF_ZONE(AllToAll);
    memset(&memo, 0, sizeof memo);
    memo.diagram = originalSolution.steadyStates[steadyStateIndex];
    auto s = &allToAll.table[steadyStateIndex];
    printf("checking steadyState %d:", steadyStateIndex);
    for (s16 branchIndex = 0; branchIndex < originalSolution.branchesCount; ++branchIndex)
    {
      static bool falsetrue[2] = {false, true};
      for (bool mirrored : falsetrue)
      {
        Branch b = originalSolution.branches[branchIndex];
#if !TRY_NON_LEAF_BRANCHES
        if (b.state.type != TypeSteadyStateIndex)
        {
          continue;
        }
#endif
        Position position = get_position(b);
        Board board = board_from_position(position, mirrored);
        bool redWins = red_turn(board);
        if (redWins)
        {
          if (s->branchIndexesCount == _countof(s->branchIndexes))
          {
            fprintf(stderr, "this steady state solves too many positions\n");
            exit(-1);
          }
          mirroredAdditions += (mirrored ? 1 : 0);
          s->branchIndexes[s->branchIndexesCount++] = branchIndex;
          if (mirrored)
          {
            auto m = &allToMirrored.table[steadyStateIndex];
            m->branchIndexes[m->branchIndexesCount++] = branchIndex;
          }
          printf(" %d", branchIndex);
          break;
        }
      }
    }
    printf(" (total %d)\n", s->branchIndexesCount);
  }
  printf("mirrored additions = %d\n", mirroredAdditions);
}

struct Replacements
{
  s16 newSteadyStates[MAX_BRANCHES]; // size is branches count
};

// fills replacements, allToAll map is volatile
static void setCoverGreedy(Replacements& replacements, s16 branchesCount, SteadyStates& usefulSteadyStates, s16 steadyStatesCount, const SteadyStateToAllBranchesItSolves& allToAllOriginal)
{
  static SteadyStateToAllBranchesItSolves allToAll;
  memcpy(&allToAll, &allToAllOriginal, sizeof allToAll);
  for (s16 i = 0; i < branchesCount; ++i) replacements.newSteadyStates[i] = -1;
  usefulSteadyStates.statesCount = 0;
  for (;;)
  {
    s16 maxPositions = 0;
    s16 maxPositionsIndex = -1;
    for (s16 steadyStateIndex = 0; steadyStateIndex < originalSolution.steadyStatesCount; ++steadyStateIndex)
    {
      auto s = &allToAll.table[steadyStateIndex];
      if (s->branchIndexesCount > maxPositions)
      {
        maxPositionsIndex = steadyStateIndex;
        maxPositions = s->branchIndexesCount;
      }
    }
    if (maxPositionsIndex == -1)
    {
      break;
    }
    {
      auto m = &allToAll.table[maxPositionsIndex];
      usefulSteadyStates.states[usefulSteadyStates.statesCount++] = maxPositionsIndex;
      for (s16 i = 0; i < m->branchIndexesCount; ++i)
      {
        s16 branchIndex = m->branchIndexes[i];
        if (replacements.newSteadyStates[branchIndex] != -1)
        {
          fprintf(stderr, "trying to replace replacement\n");
          exit(-1);
        }
        replacements.newSteadyStates[branchIndex] = maxPositionsIndex;
      }
    }
    for (s16 steadyStateIndex = 0; steadyStateIndex < steadyStatesCount; ++steadyStateIndex)
    {
      auto s = &allToAll.table[steadyStateIndex];
      for (s16 i = 0; i < s->branchIndexesCount;)
      {
        s16 branchIndex = s->branchIndexes[i];
        if (replacements.newSteadyStates[branchIndex] != -1)
        {
          s->branchIndexes[i] = s->branchIndexes[--s->branchIndexesCount];
          s->branchIndexes[s->branchIndexesCount] = 0;
        }
        else
        {
          i += 1;
        }
      }
    }
  }
}

struct SteadyStateIdsRemappings
{
  s16 oldFromNewStateId[MAX_STEADY_STATES];
  s16 newStatesCount = 0;
  s16 newFromOldStateId[MAX_STEADY_STATES];
  s16 oldStatesCount = 0;
};

static void insertionSort(s16 *a, s16 size)
{
  for (s16 i = 1; i < size; ++i)
  {
    s16 j; // will be right before insert position
    for (j = i - 1; j >= 0; --j)
    {
      if (a[j] < a[i])
      {
        break;
      }
    }
    if (j + 1 != i)
    {
      // a'[j+1] = a[i]
      // a'[j+2..i] = a[j+1..i-1]
      s16 ith = a[i];
      for (s16 k = i; k > j + 1; --k)
      {
        a[k] = a[k - 1];
      }
      a[j + 1] = ith;
    }
  }
}

static void fillRemappings(SteadyStateIdsRemappings& remap, SteadyStates& usefulSteadyStates, s16 steadyStatesCount)
{
  for (s16 i = 0; i < usefulSteadyStates.statesCount; ++i)
  {
    remap.oldFromNewStateId[i] = usefulSteadyStates.states[i];
  }
  remap.newStatesCount = usefulSteadyStates.statesCount;
  insertionSort(remap.oldFromNewStateId, remap.newStatesCount);
  remap.oldStatesCount = steadyStatesCount;
  for (s16 oldId = 0; oldId < remap.oldStatesCount; ++oldId)
  {
    remap.newFromOldStateId[oldId] = -1;
  }
  for (s16 newId = 0; newId < remap.newStatesCount; ++newId)
  {
    s16 oldId = remap.oldFromNewStateId[newId];
    remap.newFromOldStateId[oldId] = newId;
  }
}

// buffer is for mirrored
static void fillSolutionFromRemappingsAndMirrored(ConnectFourSolution& newSolution, const ConnectFourSolution& oldSolution, const SteadyStateIdsRemappings& remap, const Replacements& replacements, const SteadyStateToAllBranchesItSolves& allToMirrored)
{
  static char extraMemory[OUGHT_TO_BE_ENOUGH];
  static int extraMemoryCount;
  newSolution.branchesCount = oldSolution.branchesCount;
  for (s32 branchIndex = 0; branchIndex < oldSolution.branchesCount; ++branchIndex)
  {
    Branch b = oldSolution.branches[branchIndex];
    bool mustBeMirrored = false;
    if (b.state.type == TypeSteadyStateIndex)
    {
      s16 oldId = replacements.newSteadyStates[branchIndex];
      if (oldId == -1)
      {
        oldId = b.state.value;
      }
      auto m = &allToMirrored.table[oldId];
      for (s16 i = 0; i < m->branchIndexesCount; ++i)
      {
        if (branchIndex == m->branchIndexes[i])
        {
          mustBeMirrored = true;
          break;
        }
      }
      s16 newId = remap.newFromOldStateId[oldId];
      b.state.value = newId;
    }
    if (mustBeMirrored)
    {
      char* mirroredMoves = &extraMemory[extraMemoryCount];
      extraMemoryCount += b.movesCount;
      if (extraMemoryCount > _countof(extraMemory))
      {
        fprintf(stderr, "not enough extra memory\n");
        exit(-1);
      }
      for (int i = 0; i < b.movesCount; ++i)
      {
        s8 x = b.moves[i] - '1';
        mirroredMoves[i] = '1' + (6 - x);
      }
      b.moves = mirroredMoves;
    }
    newSolution.branches[branchIndex] = b;
  }
  newSolution.steadyStatesCount = remap.newStatesCount;
  for (s16 i = 0; i < remap.newStatesCount; ++i)
  {
    s16 oldId = remap.oldFromNewStateId[i];
    newSolution.steadyStates[i] = oldSolution.steadyStates[oldId];
  }
}

static void fillSolutionFromRemappings(ConnectFourSolution& newSolution, const ConnectFourSolution& oldSolution, const SteadyStateIdsRemappings& remap, const Replacements& replacements)
{
  newSolution.branchesCount = oldSolution.branchesCount;
  for (s32 branchIndex = 0; branchIndex < oldSolution.branchesCount; ++branchIndex)
  {
    Branch b = oldSolution.branches[branchIndex];
    if (b.state.type == TypeSteadyStateIndex)
    {
      s16 oldId = replacements.newSteadyStates[branchIndex];
      if (oldId == -1)
      {
        oldId = b.state.value;
      }
      s16 newId = remap.newFromOldStateId[oldId];
      b.state.value = newId;
    }
    newSolution.branches[branchIndex] = b;
  }
  newSolution.steadyStatesCount = remap.newStatesCount;
  for (s16 i = 0; i < remap.newStatesCount; ++i)
  {
    s16 oldId = remap.oldFromNewStateId[i];
    newSolution.steadyStates[i] = oldSolution.steadyStates[oldId];
  }
}

static void outBranches(char const* path, Branch *bs, s32 bsCount)
{
  FILE *f = fopen(path, "wb");
  if (!f)
  {
    fprintf(stderr, "can't open file %hs to write branches\n", path);
    exit(-1);
  }
  fprintf(f, "{\n");
  for (s32 branchIndex = 0; branchIndex < bsCount; ++branchIndex)
  {
    Branch b = bs[branchIndex];
    if (b.state.type == TypeMove)
    {
      fprintf(f, "  \"%.*hs\": \"%c\"", b.movesCount, b.moves, (char)b.state.value);
    }
    else
    {
      fprintf(f, "  \"%.*hs\": %d", b.movesCount, b.moves, b.state.value);
    }
    fprintf(f, "%hs\n", (branchIndex == bsCount - 1) ? "" : ",");
  }
  fprintf(f, "}\n");
  fclose(f);
}

static void outSteadyStates(char const* path, const Diagram *steadyStates, s16 steadyStatesCount)
{
  FILE *f = fopen(path, "wb");
  if (!f)
  {
    fprintf(stderr, "can't open file %hs to write new steady states\n", path);
    exit(-1);
  }
  fprintf(f, "[\n");
  for (s16 i = 0; i < steadyStatesCount; ++i)
  {
    Diagram diagram = steadyStates[i];
    fprintf(f, "  [\n");
    for (Row yt = 0; yt < ROWS; ++yt)
    {
      fprintf(f, "    \"%.*hs\"%hs\n", COLS, diagram.cells[yt], (yt == ROWS - 1 ? "" : ","));
    }
    fprintf(f, "  ]%hs\n", (i == steadyStatesCount - 1) ? "" : ",");
  }
  fprintf(f, "]\n");
  fclose(f);
}

static char const* _verify_entry(Position position, Diagram diagram)
{
  Board board = board_from_position(position);
  if (!verify_leaf(diagram, board))
  {
    return "diagram fails against some Yellow line";
  }
  return NULL;
}

struct Failure
{
  Position position;
  s16 steadyStateIndex;
  char const* problem;
};

#define MAX_FAILURES 1000
struct Failures
{
  s32 failuresCount = 0;
  Failure failures[MAX_FAILURES];
};

static void add_failure(Failures* f, Position position, s16 steadyStateIndex, char const* problem)
{
  f->failures[f->failuresCount] = {position, steadyStateIndex, problem};
  f->failuresCount += 1;
}

// branches and steady_states are globals, jobs is 1 cause we don't do multithreading
// we use globals here so red_turn can be optimized better
Failures check_steady_states_correct()
{
  Failures result = {};
  s32 maxMemo = 0;
  u64 fullStart = __rdtsc();
  for (s32 branchIndex = 0; branchIndex < originalSolution.branchesCount; ++branchIndex)
  {
    Branch b = originalSolution.branches[branchIndex];
    if (b.state.type != TypeSteadyStateIndex)
    {
      continue;
    }
    s16 steadyStateIndex = b.state.value;
    Position position = get_position(b);
    //printf("checking \"%.*hs\"...\n", position.movesCount, position.moves);
    u64 tscStart = __rdtsc();
    char const* problem = _verify_entry(position, originalSolution.steadyStates[steadyStateIndex]);
    u64 tscEnd = __rdtsc();
    if (memo.boardsCount > maxMemo)
    {
      maxMemo = memo.boardsCount;
    }
    //printf("  took ~%lf seconds, current memo is %d, max memo is %d, problem is %hs\n", (tscEnd - tscStart) / cpuFrequencyEstimate, memo.boardsCount, maxMemo, problem ? problem : "null");
    if (problem)
    {
      add_failure(&result, position, steadyStateIndex, problem);
    }
  }
  u64 fullEnd = __rdtsc();
  printf("total check time ~%lf seconds\n", (fullEnd - fullStart) / cpuFrequencyEstimate);
  return result;
}

void test0()
{
  Position position = {};
  //position.moves = "121212";
  position.moves = "767622";
  position.movesCount = 6;
  Board board = board_from_position(position);
  print_board(board, "  ");
  bool hasFour = board_has_four(board);
  printf("hasFour = %hs\n", hasFour ? "true" : "false");
  Diagram diagram =
  {
    {
      {'0','0','0','0','0','0','0'},
      {'0','0','0','0','0','0','0'},
      {'0','0','0','0','0','0','0'},
      {'0','0','0','0','0','0','0'},
      {'0','0','0','0','0','0','0'},
      {'1','0','0','0','0','0','0'},
    }
  };
  Move move = query_steady_state(board, diagram);
  printf("move = %d\n", move);
}

void printBranchesWithReplacements(const Replacements& replacements)
{
  printf("new branches with old steady state ids:\n");
  printf("{\n");
  for (s32 branchIndex = 0; branchIndex < originalSolution.branchesCount; ++branchIndex)
  {
    const Branch b = originalSolution.branches[branchIndex];
    const s16 newSteadyState = replacements.newSteadyStates[branchIndex];
    BranchMove state = b.state;
    if (newSteadyState != -1)
    {
      if (state.type != TypeSteadyStateIndex)
      {
        fprintf(stderr, "unexpected branch type change\n");
        exit(-1);
      }
      state.type = TypeSteadyStateIndex;
      state.value = newSteadyState;
    }
    if (state.type == TypeMove)
    {
      printf("  \"%.*hs\": \"%c\",\n", b.movesCount, b.moves, (char)state.value);
    }
    else
    {
      printf("  \"%.*hs\": %d,\n", b.movesCount, b.moves, state.value);
    }
  }
  printf("}\n");
}

int getTotalBranchesCovered(const SteadyStates& steadyStates, const SteadyStateToAllBranchesItSolves& allToAll)
{
  static bool branchIsCovered[MAX_BRANCHES] = {};
  memset(branchIsCovered, 0, sizeof branchIsCovered);
  for (s16 i = 0; i < steadyStates.statesCount; ++i)
  {
    s16 steadyStateIndex = steadyStates.states[i];
    auto m = &allToAll.table[steadyStateIndex];
    for (s16 j = 0; j < m->branchIndexesCount; ++j)
    {
      s16 branchIndex = m->branchIndexes[j];
      branchIsCovered[branchIndex] = true;
    }
  }
  int totalBranchesCovered = 0;
  for (s16 i = 0; i < MAX_BRANCHES; ++i)
  {
    if (branchIsCovered[i])
    {
      totalBranchesCovered += 1;
    }
  }
  return totalBranchesCovered;
}

void printPerformanceResults()
{
  for (int i = 0; i < NUM_PERF_COUNTERS; ++i)
  {
    u64 hits = perfStats[i].hits;
    if (hits == 0) continue;
    u64 ticks = perfStats[i].ticks;
    double totalTime = ticks / cpuFrequencyEstimate;
    double averageTime = totalTime / hits;
    char const* name = perfZonesNames[i];
    printf("%10.10hs: hits %10.llu, avg %15.1lf ns, total %5.3lf s\n", name, hits, averageTime * 1.0e+9, totalTime);
  }
}

#define SUBSETS_COUNT 384
void outAllToAllAsCode(char const* path, const SteadyStateToAllBranchesItSolves& allToAll, s16 steadyStatesCount)
{
  FILE* f = fopen(path, "wb");
  fprintf(f, "static Subset subsets[] =\n{\n");
  for (s16 i = 0; i < steadyStatesCount; ++i)
  {
    auto& list = allToAll.table[i];
    fprintf(f, "  {%d, {", list.branchIndexesCount);
    bool first = true;
    for (s16 i = 0; i < list.branchIndexesCount; ++i)
    {
      if (!first)
      {
        fprintf(f, ", ");
      }
      first = false;
      fprintf(f, "%d", list.branchIndexes[i]);
    }
    fprintf(f, "}},\n");
  }
  fprintf(f, "};\n");
  fclose(f);
}

int main()
{
  u64 mainStart = readTSC();
#if 1
  loadBranches(originalSolution, originalFiles, "branches.json");
  printf("%d branches loaded\n", originalSolution.branchesCount);
  loadSteadyStates(originalSolution, originalFiles, "steady_states.json");
  printf("%d steady states loaded\n", originalSolution.steadyStatesCount);
#else
  loadBranches(originalSolution, originalFiles, "new-branches-optimal.json");
  printf("%d branches loaded\n", originalSolution.branchesCount);
  loadSteadyStates(originalSolution, originalFiles, "new-steady-states-optimal.json");
  printf("%d steady states loaded\n", originalSolution.steadyStatesCount);
#endif
#if 1
  int longestBranch = 0;
  for (s16 i = 0; i < originalSolution.branchesCount; ++i)
  {
    Branch b = originalSolution.branches[i];
    if (b.state.type == TypeMove && b.movesCount > longestBranch)
    {
      longestBranch = b.movesCount;
    }
  }
  printf("longest branch size %d, free space %d\n", longestBranch, 42 - longestBranch);
#endif
  Failures failures = check_steady_states_correct();
  printf("%d failures:\n", failures.failuresCount);
  for (s32 i = 0; i < failures.failuresCount; ++i)
  {
    Failure f = failures.failures[i];
    printf("  position = \"%.*hs\", steadyStateIndex = %d, problem = %hs\n", f.position.movesCount, f.position.moves, f.steadyStateIndex, f.problem);
  }
#if 1
  static SteadyStateToAllBranchesItSolves allToAll;
  static SteadyStateToAllBranchesItSolves allToMirrored;
  collectAllPositionsForAllSteadyStates(allToAll, allToMirrored);
  outAllToAllAsCode("allToAll.c", allToAll, originalSolution.steadyStatesCount);
#endif
  u64 mainEnd = readTSC();
#if MEASURE_PERF
  printPerformanceResults();
#endif
  printf("main time = %g seconds\n", (mainEnd - mainStart) / cpuFrequencyEstimate);
#if 1
  // greedy solution (260):
  {
    static Replacements branchesReplacements;
    static SteadyStates steadyStatesCoverGreedy;
    setCoverGreedy(branchesReplacements, originalSolution.branchesCount, steadyStatesCoverGreedy, originalSolution.steadyStatesCount, allToAll);
    static SteadyStateIdsRemappings remap;
    fillRemappings(remap, steadyStatesCoverGreedy, originalSolution.steadyStatesCount);
    static ConnectFourSolution reducedSolution;
    fillSolutionFromRemappings(reducedSolution, originalSolution, remap, branchesReplacements);
    int totalBranchesCovered = getTotalBranchesCovered(steadyStatesCoverGreedy, allToAll);
    printf("total branches covered greedy %d\n", totalBranchesCovered);
    printf("new steady states count by greedy method %d\n", steadyStatesCoverGreedy.statesCount);
    outBranches("new-branches-greedy.json", reducedSolution.branches, reducedSolution.branchesCount);
    outSteadyStates("new-steady-states-greedy.json", reducedSolution.steadyStates, reducedSolution.steadyStatesCount);
  }
  // actual set cover solution (258):
  {
    static Replacements branchesReplacements =
    {
#include "set-covering/set-cover-element-to-subset.c"
    };
    static SteadyStates steadyStatesCover =
    {
#include "set-covering/set-cover-subsets.c"
    };
    static SteadyStateIdsRemappings remap;
    fillRemappings(remap, steadyStatesCover, originalSolution.steadyStatesCount);
    static ConnectFourSolution reducedSolution;
    fillSolutionFromRemappingsAndMirrored(reducedSolution, originalSolution, remap, branchesReplacements, allToMirrored);
    int totalBranchesCovered = getTotalBranchesCovered(steadyStatesCover, allToAll);
    printf("total branches covered optimal %d\n", totalBranchesCovered);
    printf("new steady states count by optimal method %d\n", steadyStatesCover.statesCount);
    outBranches("new-branches-mirrored.json", reducedSolution.branches, reducedSolution.branchesCount);
    outSteadyStates("new-steady-states-mirrored.json", reducedSolution.steadyStates, reducedSolution.steadyStatesCount);
  }
#endif
  return 0;
}
