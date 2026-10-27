// BZip2Crc.h

#ifndef ZIP7_INC_BZIP2_CRC_H
#define ZIP7_INC_BZIP2_CRC_H

#include "../../Common/MyTypes.h"

class CBZip2Crc
{
  UInt32 _value;
  static UInt32 Table[256];
public:
  static void InitTable();
  CBZip2Crc(UInt32 initVal = 0xFFFFFFFF): _value(initVal) {}
  void Init(UInt32 initVal = 0xFFFFFFFF) { _value = initVal; }
  void UpdateByte(Byte b) { _value = Table[(_value >> 24) ^ b] ^ (_value << 8); }
  void UpdateByte(unsigned b) { _value = Table[(_value >> 24) ^ b] ^ (_value << 8); }
  UInt32 GetDigest() const { return _value ^ 0xFFFFFFFF; }
};

class CBZip2CombinedCrc
{
  UInt32 _value;
public:
  CBZip2CombinedCrc(): _value(0) {}
  void Init() { _value = 0; }
  void Update(UInt32 v) { _value = ((_value << 1) | (_value >> 31)) ^ v; }
  UInt32 GetDigest() const { return _value ; }
};

#endif
