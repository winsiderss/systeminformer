// PropVariantUtils.cpp

#include "StdAfx.h"

#include "../Common/IntToString.h"

#include "PropVariantUtils.h"

using namespace NWindows;

static void AddHex(AString &s, UInt32 v)
{
  char sz[16];
  sz[0] = '0';
  sz[1] = 'x';
  ConvertUInt32ToHex(v, sz + 2);
  s += sz;
}


AString TypePairToString(const CUInt32PCharPair *pairs, unsigned num, UInt32 value)
{
  char sz[16];
  const char *p = NULL;
  for (unsigned i = 0; i < num; i++)
  {
    const CUInt32PCharPair &pair = pairs[i];
    if (pair.Value == value)
      p = pair.Name;
  }
  if (!p)
  {
    ConvertUInt32ToString(value, sz);
    p = sz;
  }
  return (AString)p;
}

void PairToProp(const CUInt32PCharPair *pairs, unsigned num, UInt32 value, NCOM::CPropVariant &prop)
{
  prop = TypePairToString(pairs, num, value);
}


AString TypeToString(const char * const table[], unsigned num, UInt32 value)
{
  char sz[16];
  const char *p = NULL;
  if (value < num)
    p = table[value];
  if (!p)
  {
    ConvertUInt32ToString(value, sz);
    p = sz;
  }
  return (AString)p;
}

void TypeToProp(const char * const table[], unsigned num, UInt32 value, NWindows::NCOM::CPropVariant &prop)
{
  char sz[16];
  const char *p = NULL;
  if (value < num)
    p = table[value];
  if (!p)
  {
    ConvertUInt32ToString(value, sz);
    p = sz;
  }
  prop = p;
}


AString FlagsToString(const char * const *names, unsigned num, UInt32 flags)
{
  AString s;
  for (unsigned i = 0; i < num; i++)
  {
    UInt32 flag = (UInt32)1 << i;
    if ((flags & flag) != 0)
    {
      const char *name = names[i];
      if (name && name[0] != 0)
      {
        s.Add_OptSpaced(name);
        flags &= ~flag;
      }
    }
  }
  if (flags != 0)
  {
    s.Add_Space_if_NotEmpty();
    AddHex(s, flags);
  }
  return s;
}

AString FlagsToString(const CUInt32PCharPair *pairs, unsigned num, UInt32 flags)
{
  AString s;
  for (unsigned i = 0; i < num; i++)
  {
    const CUInt32PCharPair &p = pairs[i];
    UInt32 flag = (UInt32)1 << (unsigned)p.Value;
    if ((flags & flag) != 0)
    {
      if (p.Name[0] != 0)
        s.Add_OptSpaced(p.Name);
    }
    flags &= ~flag;
  }
  if (flags != 0)
  {
    s.Add_Space_if_NotEmpty();
    AddHex(s, flags);
  }
  return s;
}

void FlagsToProp(const char * const *names, unsigned num, UInt32 flags, NCOM::CPropVariant &prop)
{
  prop = FlagsToString(names, num, flags);
}

void FlagsToProp(const CUInt32PCharPair *pairs, unsigned num, UInt32 flags, NCOM::CPropVariant &prop)
{
  prop = FlagsToString(pairs, num, flags);
}


static AString Flags64ToString(const CUInt32PCharPair *pairs, unsigned num, UInt64 flags)
{
  AString s;
  for (unsigned i = 0; i < num; i++)
  {
    const CUInt32PCharPair &p = pairs[i];
    UInt64 flag = (UInt64)1 << (unsigned)p.Value;
    if ((flags & flag) != 0)
    {
      if (p.Name[0] != 0)
        s.Add_OptSpaced(p.Name);
    }
    flags &= ~flag;
  }
  if (flags != 0)
  {
    {
      char sz[32];
      sz[0] = '0';
      sz[1] = 'x';
      ConvertUInt64ToHex(flags, sz + 2);
      s.Add_OptSpaced(sz);
    }
  }
  return s;
}

void Flags64ToProp(const CUInt32PCharPair *pairs, unsigned num, UInt64 flags, NCOM::CPropVariant &prop)
{
  prop = Flags64ToString(pairs, num, flags);
}
