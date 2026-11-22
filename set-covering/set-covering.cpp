#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <stdlib.h>

#ifndef _countof
#define _countof(A) (sizeof (A)) / sizeof (A[0])
#endif

// TODO: put all globals in one big struct

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// TODO: try typesafe indexes here
#define SUBSETS_COUNT 384
#define MAX_SUBSET_SIZE 25
struct ElementIndex { s16 index; bool operator==(ElementIndex i) { return index == i.index; } };
struct alignas(64) Subset
{
  s16 elementsCount;
  ElementIndex elements[MAX_SUBSET_SIZE];
  // TODO: generalize
  s16 pad[32 - MAX_SUBSET_SIZE - 1]; // lets be 64 bytes
};
bool operator<=(const Subset &s1, const Subset &s2)
{
  for (s16 i = 0, j = 0; i < s1.elementsCount; ++i)
  {
    ElementIndex e = s1.elements[i];
    while (j < s2.elementsCount && s2.elements[j].index < e.index)
    {
      ++j;
    }
    if (j == s2.elementsCount || !(e == s2.elements[j]))
    {
      return false;
    }
    ++j;
  }
  return true;
}
#include "steadyStatesToBranches.c"
struct SubsetIndex { s16 index; };
static Subset* getSubset(SubsetIndex i)
{
  return &subsets[i.index];
}

// there are only 522 elements in original task, and there are at least 21 elements that can be removed due to having only one cover subset
#define SET_SIZE 654
static const s16 setSize = SET_SIZE; // const must be <= MAX_SET_SIZE

static SubsetIndex freeVars[SUBSETS_COUNT];
// TODO: compress free subsets
static Subset freeSubsets[SUBSETS_COUNT];
struct FreeVarIndex { s16 index; bool operator<(FreeVarIndex i) { return index < i.index; } bool operator==(FreeVarIndex i) { return index == i.index; } };
static FreeVarIndex freeVarsCount = {0};
SubsetIndex* getFreeVar(FreeVarIndex i)
{
  return &freeVars[i.index];
}
Subset *getFreeSubset(FreeVarIndex i)
{
  return &freeSubsets[i.index];
}

static s16 _coverCounts[SET_SIZE]; // how many free subsets cover this element, only of not covered elements
s16 *getCoverCount(ElementIndex i)
{
  return &_coverCounts[i.index];
}

FreeVarIndex chooseFreeVar()
{
  memset(&_coverCounts, 0, sizeof _coverCounts);
  for (FreeVarIndex i = {0}; i < freeVarsCount; ++i.index)
  {
    SubsetIndex subsetIndex = *getFreeVar(i);
    const Subset* s = getFreeSubset(i);
    for (s16 j = 0; j < s->elementsCount; ++j)
    {
      ElementIndex element = s->elements[j];
      *getCoverCount(element) += 1;
    }
  }
  s16 minCoverCount = SUBSETS_COUNT + 1;
  for (ElementIndex i = {0}; i.index < SET_SIZE; ++i.index)
  {
    s16 c = *getCoverCount(i);
    if (c != 0 && c < minCoverCount)
    {
      minCoverCount = c;
    }
  }
  if (minCoverCount == SUBSETS_COUNT + 1)
  {
    fprintf(stderr, "all free vars empty\n");
    exit(-1);
  }
  // searching for minimum rho_j, which is max subsetSize
  s16 maxSize = 0;
  SubsetIndex bestSubsetIndex = {-1};
  FreeVarIndex bestFreeVarIndex = {-1};
  for (FreeVarIndex i = {0}; i < freeVarsCount; ++i.index)
  {
    SubsetIndex subsetIndex = *getFreeVar(i);
    const Subset* s = getSubset(subsetIndex);
    if (maxSize < s->elementsCount)
    {
      for (s16 j = 0; j < s->elementsCount; ++j)
      {
        ElementIndex element = s->elements[j];
        if (*getCoverCount(element) == minCoverCount)
        {
          maxSize = s->elementsCount;
          bestSubsetIndex = subsetIndex;
          bestFreeVarIndex = i;
          break;
        }
      }
    }
  }
  // TODO: remove free var here?
  return bestFreeVarIndex;
}

static s16 REC;
static bool x[SUBSETS_COUNT];
bool* getX(SubsetIndex i)
{
  return &x[i.index];
}

struct Bitset
{
  u64 bits[(SET_SIZE + 63) >> 6] = {};
};
static bool inBitset(const Bitset& bitset, ElementIndex elementIndex)
{
  static_assert(_countof(bitset.bits) == 11);
  if (elementIndex.index < 0 || 11 * 64 <= elementIndex.index)
  {
    fprintf(stderr, "bad element index\n");
    exit(-1);
  }
  u64 i = elementIndex.index >> 6ULL; // div by 64
  u64 localBit = elementIndex.index & 63ULL; // mod 64
  return bitset.bits[i] & (1ULL << localBit);
}
bool isSubsetWithoutCovered(const Subset& s1, const Subset& s2, const Bitset& coveredElements)
{
  for (s16 i = 0, j = 0; i < s1.elementsCount; ++i)
  {
    ElementIndex e = s1.elements[i];
    if (inBitset(coveredElements, e))
    {
      continue;
    }
    while (j < s2.elementsCount && (inBitset(coveredElements, s2.elements[j]) || s2.elements[j].index < e.index))
    {
      ++j;
    }
    if (j == s2.elementsCount || !(e == s2.elements[j]))
    {
      return false;
    }
    ++j;
  }
  return true;
}
static void addToBitset(Bitset& bitset, ElementIndex elementIndex)
{
  bitset.bits[elementIndex.index >> 6ULL] |= (1ULL << (elementIndex.index & 63ULL));
}
static Bitset coveredSet;
static bool covered(ElementIndex elementIndex)
{
  return inBitset(coveredSet, elementIndex);
}
static void setCovered(ElementIndex elementIndex)
{
  addToBitset(coveredSet, elementIndex);
}

// TODO: initial filtering
static void initFreeVars()
{
  freeVarsCount.index = SUBSETS_COUNT;
  for (s16 i = 0; i < SUBSETS_COUNT; ++i)
  {
    freeVars[i].index = i;
    freeSubsets[i] = subsets[i];
  }
}

static SubsetIndex madeNonfree[SUBSETS_COUNT];
struct NonfreeIndex
{
  s16 index;
};
static NonfreeIndex madeNonfreeCount;
SubsetIndex *getMadeNonfree(NonfreeIndex i)
{
  return &madeNonfree[i.index];
}
struct StackEntry
{
  SubsetIndex chosen;
  NonfreeIndex madeNonfreeStart;
};
static s32 stackCount;
static StackEntry stack[SUBSETS_COUNT];

static void makeNonfree(SubsetIndex subsetIndex)
{
  *getMadeNonfree(madeNonfreeCount) = subsetIndex;
  madeNonfreeCount.index += 1;
}

static void subtractCovered(Subset *s)
{
  s16 newElementsCount = 0;
  for (s16 i = 0; i < s->elementsCount; ++i)
  {
    auto e = s->elements[i];
    if (!covered(e))
    {
      s->elements[newElementsCount++] = e;
    }
  }
  s->elementsCount = newElementsCount;
}

#if 0
static void memswap64(u64 *a, u64 *b, int count)
{
  while (count--)
  {
    u64 aOld = *a;
    u64 bOld = *b;
    *a++ = bOld;
    *b++ = aOld;
  }
}

static void swapSubsets(Subset *s1, Subset *s2)
{
  static_assert(sizeof(Subset) % 8 == 0);
  memswap64((u64*)s1, (u64*)s2, sizeof(Subset) / 8);
}
#endif

static void removeFreeVar(FreeVarIndex freeVarIndex)
{
  if (freeVarsCount.index == 1)
  {
    freeVarsCount = {0};
    return;
  }
  freeVarsCount.index -= 1;
  *getFreeSubset(freeVarIndex) = *getFreeSubset(freeVarsCount);
  for (FreeVarIndex i = {0}; i.index < freeVarsCount.index; ++i.index)
  {
    Subset *s = getFreeSubset(i);
    subtractCovered(s);
  }
}

static void solveMinCover()
{
  initFreeVars();
  for (;;)
  {
    auto freeVarIndex = chooseFreeVar();
    auto subsetIndex = *getFreeVar(freeVarIndex);
    *getX(subsetIndex) = true;
    const Subset* s = getFreeSubset(freeVarIndex);
    for (s16 i = 0; i < s->elementsCount; ++i)
    {
      setCovered(s->elements[i]);
    }
  }
}

void printBitset(const Bitset& bitset)
{
  char str[SET_SIZE + 1 + 64];
  int newlines = 0;
  int ones = 0;
  int zeros = 0;
  for (ElementIndex i = {0}; i.index < SET_SIZE; ++i.index)
  {
    bool bIn = inBitset(bitset, i);
    ones  += bIn ? 1 : 0;
    zeros += bIn ? 0 : 1;
    str[i.index + newlines] = bIn ? '1' : '0';
    if ((i.index & 63) == 63)
    {
      newlines += 1;
      str[i.index + newlines] = '\n';
    }
  }
  str[SET_SIZE + newlines] = 0;
  printf("%hs\nelements = %d, skipped elemets = %d\n", str, ones, zeros);
}

// table of any counters for each element
struct CountersForElements
{
  s16 table[SET_SIZE] = {}; // size is alwayse SET_SIZE
};
static s16& counter(CountersForElements& c, ElementIndex i)
{
  return c.table[i.index];
}
struct ElementToSubsetMap
{
  SubsetIndex table[SET_SIZE] = {};
};
static constexpr const SubsetIndex NoSubset = {-1};
static void initEmpty(ElementToSubsetMap& m)
{
  for (int i = 0; i < SET_SIZE; ++i) m.table[i] = NoSubset;
}
static SubsetIndex& mappedSubset(ElementToSubsetMap& m, ElementIndex i)
{
  return m.table[i.index];
}
struct FlagsForSubsets
{
  bool table[SUBSETS_COUNT];
};
static bool& flag(FlagsForSubsets& f, SubsetIndex i)
{
  return f.table[i.index];
}

static void getRedundantFirstMethod(FlagsForSubsets& redundant)
{
  memset(&redundant, 0, sizeof redundant);
  int redundantSubsets = 0;
  for (SubsetIndex i = {0}; i.index < SUBSETS_COUNT; ++i.index)
  {
    const Subset* si = getSubset(i);
    bool first = true;
    for(SubsetIndex j = {0}; j.index < SUBSETS_COUNT; ++j.index)
    {
      if (j.index == i.index || flag(redundant, j))
      {
        continue;
      }
      if (*si <= *getSubset(j))
      {
        if (first)
        {
          redundantSubsets += 1;
          flag(redundant, i) = true;
          printf("s%d <=", i.index);
        }
        first = false;
        printf(" s%d", j.index);
      }
    }
    if (!first)
    {
      printf("\n");
    }
  }
  printf("total redundant = %d\n", redundantSubsets);
}

static void simpleLogicIterations()
{
  FlagsForSubsets redundant = {};
  FlagsForSubsets taken = {};
  Bitset coveredElements = {};
  int totalCoveredElements = 0;
  int totalTakenSubsets = 0;
  int totalRedundant = 0;
  int iteration = 0;
  FlagsForSubsets redundantFirstMethod;
  getRedundantFirstMethod(redundantFirstMethod);
  ElementToSubsetMap replacements;
  initEmpty(replacements);
  for (;;)
  {
    int redundantThisIteration = 0;
    for (SubsetIndex i = {0}; i.index < SUBSETS_COUNT; ++i.index)
    {
      if (flag(taken, i) || flag(redundant, i))
      {
        continue;
      }
      const Subset* si = getSubset(i);
      for(SubsetIndex j = {0}; j.index < SUBSETS_COUNT; ++j.index)
      {
        if (flag(taken, j) || flag(redundant, j) || j.index == i.index)
        {
          continue;
        }
        if (isSubsetWithoutCovered(*si, *getSubset(j), coveredElements))
        {
          redundantThisIteration += 1;
          flag(redundant, i) = true;
          break;
        }
      }
    }
    // tmp
    if (iteration == 0)
    {
      for (SubsetIndex i = {0}; i.index < SUBSETS_COUNT; ++i.index)
      {
        bool bOld = flag(redundantFirstMethod, i);
        bool bNew = flag(redundant, i);
        if (bOld != bNew)
        {
          printf("differ old %d != new %d for s%d\n", bOld, bNew, i.index);
        }
      }
    }
    CountersForElements coverCounts = {};
    ElementToSubsetMap lastCountedSubset;
    initEmpty(lastCountedSubset);
    for (SubsetIndex i = {0}; i.index < SUBSETS_COUNT; ++i.index)
    {
      if (flag(redundant, i) || flag(taken, i))
      {
        continue;
      }
      const Subset *s = getSubset(i);
      for (s16 j = 0; j < s->elementsCount; ++j)
      {
        ElementIndex element = s->elements[j];
        if (inBitset(coveredElements, element))
        {
          continue;
        }
        counter(coverCounts, element) += 1;
        mappedSubset(lastCountedSubset, element) = i;
      }
    }
    int coveredThisIteration = 0;
    int takenThisIteration = 0;
    for (ElementIndex i = {0}; i.index < SET_SIZE; ++i.index)
    {
      if (counter(coverCounts, i) != 1)
      {
        continue;
      }
      SubsetIndex mustSubsetIndex = mappedSubset(lastCountedSubset, i);
      bool wasTaken = (flag(taken, mustSubsetIndex) == true);
      if (!wasTaken)
      {
        flag(taken, mustSubsetIndex) = true;
        takenThisIteration += 1;
      }
      const Subset* s = getSubset(mustSubsetIndex);
      for (s16 j = 0; j < s->elementsCount; ++j)
      {
        ElementIndex element = s->elements[j];
        if (inBitset(coveredElements, element))
        {
          continue;
        }
        coveredThisIteration += 1;
        addToBitset(coveredElements, element);
        mappedSubset(replacements, element) = mustSubsetIndex;
      }
    }
    if (coveredThisIteration == 0 && takenThisIteration == 0 && redundantThisIteration == 0)
    {
      break;
    }
    totalCoveredElements += coveredThisIteration;
    totalTakenSubsets += takenThisIteration;
    totalRedundant += redundantThisIteration;
    printf("%d) covered %d, taken %d, redundant %d\n", iteration++, totalCoveredElements, totalTakenSubsets, totalRedundant);
  }
  int takenDoubleCheck = 0;
  for (SubsetIndex i = {0}; i.index < SUBSETS_COUNT; ++i.index)
  {
    if (flag(taken, i))
    {
      takenDoubleCheck += 1;
    }
  }
  printf("taken double check = %d\n", takenDoubleCheck);
#if 1
  {
    FILE* idsFile = fopen("set-cover-subsets.c", "wb");
    fprintf(idsFile, "%d,\n", totalTakenSubsets);
    fprintf(idsFile, "{\n");
    for (SubsetIndex i = {0}; i.index < SUBSETS_COUNT; ++i.index)
    {
      if (flag(taken, i))
      {
        fprintf(idsFile, "  %d,\n", i.index);
      }
    }
    fprintf(idsFile, "}\n");
    fclose(idsFile);
    FILE* replacementsFile = fopen("set-cover-element-to-subset.c", "wb");
    for (ElementIndex i = {0}; i.index < SET_SIZE; ++i.index)
    {
      SubsetIndex subsetIndex = mappedSubset(replacements, i);
      fprintf(replacementsFile, "%d,\n", subsetIndex.index); // -1 means not covered, not all element indexes must be covered (cause some were not in the set of steady state branches)
    }
    fclose(replacementsFile);
  }
#endif
}

void printSubsetsThatMustBeTaken(FlagsForSubsets& redundant)
{
  CountersForElements coverCounts = {};
  ElementToSubsetMap lastCountedSubset;
  initEmpty(lastCountedSubset);
  for (SubsetIndex i = {0}; i.index < SUBSETS_COUNT; ++i.index)
  {
    if (flag(redundant, i))
    {
      continue;
    }
    const Subset *s = getSubset(i);
    for (s16 j = 0; j < s->elementsCount; ++j)
    {
      ElementIndex element = s->elements[j];
      counter(coverCounts, element) += 1;
      mappedSubset(lastCountedSubset, element) = i;
    }
  }
  FlagsForSubsets mustBeTaken = {};
  printf("must be taken:\n");
  for (ElementIndex i = {0}; i.index < SET_SIZE; ++i.index)
  {
    if (counter(coverCounts, i) == 1)
    {
      auto mustSubsetIndex = mappedSubset(lastCountedSubset, i);
      flag(mustBeTaken, mustSubsetIndex) = true;
    }
  }
  CountersForElements coveredByMustBeTaken = {};
  int totalMustBeTaken = 0;
  for (SubsetIndex i = {0}; i.index < SUBSETS_COUNT; ++i.index)
  {
    if (flag(mustBeTaken, i))
    {
      totalMustBeTaken += 1;
      const Subset *s = getSubset(i);
      printf("  s%d is unique for", i.index);
      for (s16 j = 0; j < s->elementsCount; ++j)
      {
        ElementIndex element = s->elements[j];
        counter(coveredByMustBeTaken, element) += 1;
        if (counter(coverCounts, element) == 1)
        {
          printf(" e%d", element.index);
        }
      }
      printf("\n");
    }
  }
  int totalCoveredByMustBeTaken = 0;
  for (ElementIndex i = {0}; i.index < SET_SIZE; ++i.index)
  {
    if (counter(coveredByMustBeTaken, i) > 0)
    {
      totalCoveredByMustBeTaken += 1;
    }
  }
  //printf("\n");
  printf("must be taken sets %d\n", totalMustBeTaken);
  printf("elements covered by must be taken sets %d\n", totalCoveredByMustBeTaken);
}

void printSubsetsAsCode()
{
  static_assert(_countof(subsets) == SUBSETS_COUNT);
  printf("static Subset subsets[] =\n{\n");
  for (auto& list : subsets)
  {
    printf("  {%d, {", list.elementsCount);
    bool first = true;
    for (s16 i = 0; i < list.elementsCount; ++i)
    {
      if (!first)
      {
        printf(", ");
      }
      first = false;
      printf("%d", list.elements[i].index);
    }
    printf("}},\n");
  }
  printf("};\n");
}

void printSubsets()
{
  static_assert(_countof(subsets) == SUBSETS_COUNT);
  printf("subsets:\n");
  int i = 0;
  for (auto& list : subsets)
  {
    printf("  s%d:", i++);
    for (s16 i = 0; i < list.elementsCount; ++i)
    {
      printf(" e%d", list.elements[i].index);
    }
    printf("\n");
  }
}

int main()
{
  //printSubsetsThatMustBeTaken(redundant);
  simpleLogicIterations();
  for (auto& s : subsets)
  {
    for (s16 i = 0; i < s.elementsCount; ++i)
    {
      setCovered(s.elements[i]);
    }
  }
  printSubsets();
  printBitset(coveredSet);
  return 0;
}

