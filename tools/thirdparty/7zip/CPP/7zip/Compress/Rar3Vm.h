// Rar3Vm.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

#ifndef ZIP7_INC_COMPRESS_RAR3_VM_H
#define ZIP7_INC_COMPRESS_RAR3_VM_H

#include "../../../C/CpuArch.h"

#include "../../Common/MyVector.h"

#define Z7_RARVM_STANDARD_FILTERS
// #define Z7_RARVM_VM_ENABLE

namespace NCompress {
namespace NRar3 {

class CMemBitDecoder
{
  const Byte *_data;
  UInt32 _bitSize;
  UInt32 _bitPos;
public:
  void Init(const Byte *data, UInt32 byteSize)
  {
    _data = data;
    _bitSize = (byteSize << 3);
    _bitPos = 0;
  }
  UInt32 ReadBits(unsigned numBits);
  UInt32 ReadBit();
  bool Avail() const { return (_bitPos < _bitSize); }

  UInt32 ReadEncodedUInt32();
};

namespace NVm {

inline UInt32 GetValue32(const void *addr) { return GetUi32(addr); }
inline void SetValue32(void *addr, UInt32 value) { SetUi32(addr, value) }

const unsigned kNumRegBits = 3;
const UInt32 kNumRegs = 1 << kNumRegBits;
const UInt32 kNumGpRegs = kNumRegs - 1;

const UInt32 kSpaceSize = 0x40000;
const UInt32 kSpaceMask = kSpaceSize - 1;
const UInt32 kGlobalOffset = 0x3C000;
const UInt32 kGlobalSize = 0x2000;
const UInt32 kFixedGlobalSize = 64;

namespace NGlobalOffset
{
  const UInt32 kBlockSize = 0x1C;
  const UInt32 kBlockPos  = 0x20;
  const UInt32 kExecCount = 0x2C;
  const UInt32 kGlobalMemOutSize = 0x30;
}


#ifdef Z7_RARVM_VM_ENABLE

enum ECommand
{
  CMD_MOV,  CMD_CMP,  CMD_ADD,  CMD_SUB,  CMD_JZ,   CMD_JNZ,  CMD_INC,  CMD_DEC,
  CMD_JMP,  CMD_XOR,  CMD_AND,  CMD_OR,   CMD_TEST, CMD_JS,   CMD_JNS,  CMD_JB,
  CMD_JBE,  CMD_JA,   CMD_JAE,  CMD_PUSH, CMD_POP,  CMD_CALL, CMD_RET,  CMD_NOT,
  CMD_SHL,  CMD_SHR,  CMD_SAR,  CMD_NEG,  CMD_PUSHA,CMD_POPA, CMD_PUSHF,CMD_POPF,
  CMD_MOVZX,CMD_MOVSX,CMD_XCHG, CMD_MUL,  CMD_DIV,  CMD_ADC,  CMD_SBB,  CMD_PRINT,

  CMD_MOVB, CMD_CMPB, CMD_ADDB, CMD_SUBB, CMD_INCB, CMD_DECB,
  CMD_XORB, CMD_ANDB, CMD_ORB,  CMD_TESTB,CMD_NEGB,
  CMD_SHLB, CMD_SHRB, CMD_SARB, CMD_MULB
};

enum EOpType {OP_TYPE_REG, OP_TYPE_INT, OP_TYPE_REGMEM, OP_TYPE_NONE};

// Addr in COperand object can link (point) to CVm object!!!

struct COperand
{
  EOpType Type;
  UInt32 Data;
  UInt32 Base;
  COperand(): Type(OP_TYPE_NONE), Data(0), Base(0) {}
};

struct CCommand
{
  ECommand OpCode;
  bool ByteMode;
  COperand Op1, Op2;
};

#endif


struct CBlockRef
{
  UInt32 Offset;
  UInt32 Size;
};


class CProgram
{
  #ifdef Z7_RARVM_VM_ENABLE
  void ReadProgram(const Byte *code, UInt32 codeSize);
public:
  CRecordVector<CCommand> Commands;
  #endif
  
public:
  #ifdef Z7_RARVM_STANDARD_FILTERS
  int StandardFilterIndex;
  #endif
  
  bool IsSupported;
  CRecordVector<Byte> StaticData;

  bool PrepareProgram(const Byte *code, UInt32 codeSize);
};


struct CProgramInitState
{
  UInt32 InitR[kNumGpRegs];
  CRecordVector<Byte> GlobalData;

  void AllocateEmptyFixedGlobal()
  {
    GlobalData.ClearAndSetSize(NVm::kFixedGlobalSize);
    memset(&GlobalData[0], 0, NVm::kFixedGlobalSize);
  }
};


class CVm
{
  static UInt32 GetValue(bool byteMode, const void *addr)
  {
    if (byteMode)
      return(*(const Byte *)addr);
    else
      return GetUi32(addr);
  }

  static void SetValue(bool byteMode, void *addr, UInt32 value)
  {
    if (byteMode)
      *(Byte *)addr = (Byte)value;
    else
      SetUi32(addr, value)
  }

  UInt32 GetFixedGlobalValue32(UInt32 globalOffset) { return GetValue(false, &Mem[kGlobalOffset + globalOffset]); }

  void SetBlockSize(UInt32 v) { SetValue(&Mem[kGlobalOffset + NGlobalOffset::kBlockSize], v); }
  void SetBlockPos(UInt32 v) { SetValue(&Mem[kGlobalOffset + NGlobalOffset::kBlockPos], v); }
public:
  static void SetValue(void *addr, UInt32 value) { SetValue(false, addr, value); }

private:

  #ifdef Z7_RARVM_VM_ENABLE
  UInt32 GetOperand32(const COperand *op) const;
  void SetOperand32(const COperand *op, UInt32 val);
  Byte GetOperand8(const COperand *op) const;
  void SetOperand8(const COperand *op, Byte val);
  UInt32 GetOperand(bool byteMode, const COperand *op) const;
  void SetOperand(bool byteMode, const COperand *op, UInt32 val);
  bool ExecuteCode(const CProgram *prg);
  #endif
  
  #ifdef Z7_RARVM_STANDARD_FILTERS
  bool ExecuteStandardFilter(unsigned filterIndex);
  #endif
  
  Byte *Mem;
  UInt32 R[kNumRegs + 1]; // R[kNumRegs] = 0 always (speed optimization)
  UInt32 Flags;

public:
  CVm();
  ~CVm();
  bool Create();
  void SetMemory(UInt32 pos, const Byte *data, UInt32 dataSize);
  bool Execute(CProgram *prg, const CProgramInitState *initState,
      CBlockRef &outBlockRef, CRecordVector<Byte> &outGlobalData);
  const Byte *GetDataPointer(UInt32 offset) const { return Mem + offset; }
};

#endif

}}}
