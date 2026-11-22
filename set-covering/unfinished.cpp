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
