#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <intrin.h>

// TODO: make this .c
#ifdef _MSC_VER
#pragma comment(linker, "/STACK:134217728,4096") // Reserve 128 MiB, commit 4 KiB
#endif

#if 0
#define IntSizes(X)\
  X(8)\
  X(16)\
  X(32)\
  X(64)\

#define Signed(N) typedef int##N##_t s##N;
#define Unsigned(N) typedef uint##N##_t u##N;

IntSizes(Signed)
IntSizes(Unsigned)
#endif

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

// TODO: use 2 bits
typedef u8 Player;
const Player Empty = 0;
const Player Red = 1;
const Player Yellow = 2;

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

struct Board
{
  Player cells[ROWS][COLS];
};

struct Heights
{
  Row heights[COLS];
};

static const Level LEVEL_CHARS[] = "0123456789";

// TODO: diagram is a known constant, and there are merely 384 of them (no including mirrored), we can remove an argument and template everything by a diagram
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
void load_file(char const* path, char const* bufferFile, const int bufferFileMaxCount, int* fileCount)
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
int branchesFileCount = 0;
char branchesFile[OUGHT_TO_BE_ENOUGH];
int steadyStatesFileCount = 0;
char steadyStatesFile[OUGHT_TO_BE_ENOUGH];
s32 branchesCount = 0;
Branch branches[MAX_BRANCHES];
s32 steadyStatesCount = 0;
Diagram steadyStates[MAX_STEADY_STATES];

static void add_branch(char const* moves, int movesCount, BranchType branchType, s16 branchValue)
{
  if (branchesCount >= MAX_BRANCHES)
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
  branches[branchesCount].moves = moves;
  branches[branchesCount].movesCount = movesCount;
  branches[branchesCount].state.type = branchType;
  branches[branchesCount].state.value = branchValue;
  branchesCount += 1;
}

static void parse_branches()
{
  s32 positionStart = -1;
  s32 positionCount = 0;
  enum { LookingForPosition, FillingPosition, LookingForMove, FillingState } parseState = LookingForPosition;
  s32 steadyStateIndex = -1;
  char prevc = '\n';
  for (s32 i = 0; i < branchesFileCount; ++i)
  {
    char c = branchesFile[i];
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
            add_branch(&branchesFile[positionStart], positionCount, TypeMove, c);
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
          add_branch(&branchesFile[positionStart], positionCount, TypeSteadyStateIndex, steadyStateIndex);
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

static void parse_steady_states()
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
    steadyStates[steadyStatesCount].cells[yt][x] = c;
    digitsCollected += 1;
    if (digitsCollected == ROWS * COLS)
    {
      digitsCollected = 0;
      steadyStatesCount += 1;
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
  if (digitsCollected || x || yt || steadyStatesCount == 0)
  {
    fprintf(stderr, "error parsing steady stated (digits count in the file is not a multiple of ROWS * COLS), x = %d, yt = %d, digitsCollected = %d\n", x, yt, digitsCollected);
    exit(-1);
  }
}

static void load_branches(char const* path)
{
  load_file(path, branchesFile, sizeof branchesFile, &branchesFileCount);
  parse_branches();
}

static void load_steady_states(char const* path)
{
  load_file(path, steadyStatesFile, sizeof steadyStatesFile, &steadyStatesFileCount);
  parse_steady_states();
}

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

#if 0
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

static void print_board(Board board, char const* indent)
{
  for (Row yPlus = ROWS; yPlus != 0; --yPlus)
  {
    static char row[COLS];
    Row y = yPlus - 1;
    for (Column x = 0; x < COLS; ++x)
    {
      row[x] = char_from_player(board.cells[y][x]);
    }
    printf("%hs|%.*hs|\n", indent, COLS, row);
  }
  printf("%hs.-------.\n", indent);
}

static Board board_from_position(Position position)
{
  DEBUG_PRINT("board_from_position:\n");
  Board board = {};
  Heights h = {};
  for (int i = 0; i < position.movesCount; ++i)
  {
    Column x = position.moves[i] - '1';
    DEBUG_PRINT("  i=%d x=%d\n", i, x);
    // TODO: ignore this?
    if (x >= COLS || h.heights[x] >= ROWS)
    {
      fprintf(stderr, "illegal position %.*s\n", position.movesCount, position.moves);
      exit(1);
    }
    board.cells[h.heights[x]][x] = (i & 1) + 1;
    h.heights[x] += 1;
  }
  return board;
}

static bool makes_four(Board board, Column x, Row y, Player player)
{
  DEBUG_PRINT("makes_four %d %d %c:\n", x, y, char_from_player(player));
  const static s8 deltas[][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
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
      while (nx < COLS && ny < ROWS && board.cells[ny][nx] == player)
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

static bool board_has_four(Board board)
{
  for (Row y = 0; y < ROWS; ++y)
  {
    for (Column x = 0; x < COLS; ++x)
    {
      auto player = board.cells[y][x];
      if (player != Empty && makes_four(board, x, y, player))
      {
        return true;
      }
    }
  }
  return false;
}

static Row col_height(Board board, Column x)
{
  for (Row y = 0; y < ROWS; ++y)
  {
    if (board.cells[y][x] == Empty)
    {
      return y;
    }
  }
  return ROWS;
}

// TODO: struct for heights
static bool wins(Column x, Player player, Board *board, Heights h)
{
  Row y = h.heights[x];
  if (y >= ROWS)
  {
    return false;
  }
  // NOTE: looks like we don't need a pointer to board here, let's hope compiler will inline everything for us,
  //       but we actually don't need to change board, makes_four doesn't look at the (x,y) cell
  board->cells[y][x] = player;
  bool won = makes_four(*board, x, y, player);
  board->cells[y][x] = Empty;
  return won;
}

struct Playable
{
  Move moves[COLS];
  u8 movesCount;
};

// TODO: replace with get_one_playable_by_level that will early return 0 if more than one move is playable
static Playable playable(Level level_char, Heights h, Diagram diagram)
{
  DEBUG_PRINT("playable:\n");
  Playable found = {};
  for (Column x = 0; x < COLS; ++x)
  {
    Row y = h.heights[x];
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
  Heights h = {};
  for (Column x = 0; x < COLS; ++x)
  {
    h.heights[x] = col_height(board, x);
  }
  static const Player players[2] = {Red, Yellow};
  for (Player player : players)
  {
    for (Column x = 0; x < COLS; ++x)
    {
      // TODO: don't use pointer to board here
      if (wins(x, player, &board, h))
      {
        return x + 1;
      }
    }
  }
  for (Level level_char : LEVEL_CHARS)
  {
    Playable valid = playable(level_char, h, diagram);
    if (valid.movesCount == 1)
    {
      return valid.moves[0];
    }
  }
  // we use 0 instead of None analogue, because 0 is an invalid move
  return 0;
}

// A set of boards for which red_turn returns true
#define MAX_MEMO (1 << 20)
#if 0
struct Memo
{
  int boardsCount = 0;
  Board boards[MAX_MEMO]; // TODO: a hash table
};
static bool in_memo(Board board, Memo* memo)
{
  // start from the end because we memorize later states earlier
  for (int i = memo->boardsCount - 1; i >= 0; --i)
  {
    if (memcmp(&memo->boards[i], &board, sizeof(Board)) == 0) // TODO: use == after we compress the board
    {
      return true;
    }
  }
  return false;
}
static void add_to_memo(Board board, Memo* memo)
{
  if (memo->boardsCount == MAX_MEMO)
  {
    fprintf(stderr, "out of memo\n");
    exit(-1);
  }
  memo->boards[memo->boardsCount++] = board;
  //printf("memoCount = %d\n", memo->boardsCount);
}
#else
struct Memo
{
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
#endif

static bool is_draw_state(Board board)
{
  Column c;
  for (c = 0; c < COLS; ++c)
  {
    if (col_height(board, c) >= ROWS)
    {
      continue;
    }
    break;
  }
  return c == COLS;
}

static bool red_turn(Memo* memo, Diagram diagram, Board board)
{
  Board key = board;
  if (in_memo(key, memo))
  {
    return true;
  }
  Move move = query_steady_state(board, diagram);
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
  board.cells[y][x] = Red;
  {
    if (makes_four(board, x, y, Red))
    {
      add_to_memo(key, memo);
      return true;
    }
    if (is_draw_state(board))
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
      board.cells[yy][yx] = Yellow;
      {
        if (makes_four(board, yx, yy, Yellow))
        {
          return false;
        }
        if (is_draw_state(board))
        {
          return false;
        }
        // TODO: get rid of recursion
        if (!red_turn(memo, diagram, board))
        {
          return false;
        }
      }
      board.cells[yy][yx] = Empty;
    }
  }
  board.cells[y][x] = Empty; // we pass by value so we don't need this yet, maybe I'll have to pass by a pointer to test before even optimizing
  add_to_memo(key, memo);
  return true;
}

static Memo memo;
static bool verify_leaf(Diagram diagram, Board board)
{
  // TODO: don't use static for multithreaded version, just allocate memo or use thread local storage, or create a lots of memos for all threads outside
  memset(&memo, 0, sizeof memo);
  if (board_has_four(board))
  {
    return false;
  }
  return red_turn(&memo, diagram, board);
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

const double cpuFrequencyEstimate = 3.5e+9;
// branches and steady_states are globals, jobs is 1 cause we don't do multithreading
Failures check_steady_states_correct()
{
  Failures result = {};
  s32 maxMemo = 0;
  u64 fullStart = __rdtsc();
  for (s32 branchIndex = 0; branchIndex < branchesCount; ++branchIndex)
  {
    Branch b = branches[branchIndex];
    if (b.state.type != TypeSteadyStateIndex)
    {
      continue;
    }
    s16 steadyStateIndex = b.state.value;
    Position position = get_position(b);
    printf("checking \"%.*hs\"...\n", position.movesCount, position.moves);
    u64 tscStart = __rdtsc();
    char const* problem = _verify_entry(position, steadyStates[steadyStateIndex]);
    u64 tscEnd = __rdtsc();
    if (memo.boardsCount > maxMemo)
    {
      maxMemo = memo.boardsCount;
    }
    printf("  took ~%lf seconds, current memo is %d, max memo is %d, problem is %hs\n", (tscEnd - tscStart) / cpuFrequencyEstimate, memo.boardsCount, maxMemo, problem ? problem : "null");
    if (problem)
    {
      add_failure(&result, position, steadyStateIndex, problem);
    }
  }
  u64 fullEnd = __rdtsc();
  printf("total time ~%lf seconds\n", (fullEnd - fullStart) / cpuFrequencyEstimate);
  return result;
}

#if 0
static bool starts_with(char const* s, int sCount, char const* start, int startCount)
{
  int i;
  for (i = 0; i < sCount && i < startCount; ++i)
  {
    if (s[i] != start[i])
    {
      return false;
    }
  }
  return i == startCount;
}
// TODO: think how to make this code small and still simple from CPU instructions point of view
static void parse_branches_old()
{
  const char lineStart[] = "  \"";
  const int lineStartCount = (sizeof lineStart) - 1;
  const char lineMiddle[] = "\": ";
  const int lineMiddleCount = (sizeof lineMiddle) - 1;
  char const* fileEnd = branchesFile + branchesFileCount;
  for (int i = 0; i < branchesFileCount;)
  {
    if (branchesFile[i] != '\n')
    {
      i += 1;
      continue;
    }
    char const* currentLineStart = &branchesFile[i + 1];
    if (!starts_with(currentLineStart, branchesFileCount - i, lineStart, lineStartCount))
    {
      i += 1;
      continue;
    }
    char const* positionStart = currentLineStart + lineStartCount;
    int currentLineCount = fileEnd - currentLineStart;
    char const* positionEnd = NULL;
    char const* stateStart = NULL;
    char const* stateEnd = NULL;
    bool stateIsSet = false;
    int j;
    for (j = lineStartCount; j < currentLineCount; ++j)
    {
      if (currentLineStart[j] == '\n')
      {
        fprintf(stderr, "end of line \"%.*hs\" reached\n", j, currentLineStart);
        exit(-1);
      }
      if (!starts_with(&currentLineStart[j], currentLineCount, lineMiddle, lineMiddleCount))
      {
        continue;
      }
      positionEnd = &currentLineStart[j];
      stateStart = positionEnd + lineMiddleCount;
      break;
    }
    if (!positionEnd || !stateStart || j >= currentLineCount)
    {
      fprintf(stderr, "line \"%.*hs\" starts with '%.*hs' but does not have '%.*hs' in the middle\n", j, currentLineStart, lineStartCount, lineStart, lineMiddleCount, lineMiddle);
    }
    int stateStartCount = fileEnd - stateStart;
    if (stateStartCount <= 1)
    {
      fprintf(stderr, "state started but file ended...\n");
      exit(-1);
    }
    BranchMove state;
    if (stateStart[0] == '\"')
    {
      state.type = TypeMove;
      state.value = stateStart[1];
      stateEnd = stateStart + ((sizeof "\"0\"") - 1);
    }
    else if ('0' <= stateStart[0] && stateStart[0] <= '9')
    {
      s16 index = 0;
      int indexCount;
      for (indexCount = 0; indexCount < stateStartCount; ++indexCount)
      {
        char d = stateStart[indexCount];
        if ('0' <= d && d <= '9')
        {
          index *= 10;
          index += d - '0';
        }
        else
        {
          stateEnd = stateStart + indexCount;
          break;
        }
      }
      if (indexCount == stateStartCount)
      {
        fprintf(stderr, "file end reachd while parsing state\n");
        exit(-1);
      }
      state.type = TypeSteadyStateIndex;
      state.value = index;
    }
    else
    {
      fprintf(stderr, "bad branch state\n");
      exit(-1);
    }
    if (!stateEnd)
    {
      fprintf(stderr, "where is state end?\n");
      exit(-1);
    }
    {
      int k;
      for (k = 0; k < fileEnd - stateEnd; ++k)
      {
        if (stateEnd[k] == '\n')
        {
          i += 1 + (stateEnd - currentLineStart) + k;
          break;
        }
      }
      if (k == fileEnd - stateEnd)
      {
        fprintf(stderr, "end of file while searching for end of line...\n");
        exit(-1);
      }
    }
    add_branch(positionStart, positionEnd - positionStart, state.type, state.value);
  }
}
#endif

int main()
{
  Position position = {};
  //position.moves = "1212121";
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
  load_branches("branches.json");
  printf("%d branches loaded\n", branchesCount);
  load_steady_states("steady_states.json");
  printf("%d steady states loaded\n", steadyStatesCount);
  Failures failures = check_steady_states_correct();
  printf("%d failures:\n", failures.failuresCount);
  for (s32 i = 0; i < failures.failuresCount; ++i)
  {
    Failure f = failures.failures[i];
    printf("  position = \"%.*hs\", steadyStateIndex = %d, problem = %hs\n", f.position.movesCount, f.position.moves, f.steadyStateIndex, f.problem);
  }
  return 0;
}
