// ProgressMt.h

#ifndef ZIP7_INC_PROGRESSMT_H
#define ZIP7_INC_PROGRESSMT_H

#include "../../Common/MyCom.h"
#include "../../Common/MyVector.h"
#include "../../Windows/Synchronization.h"

#include "../ICoder.h"
#include "../IProgress.h"

class CMtCompressProgressMixer
{
  CMyComPtr<ICompressProgressInfo> _progress;
  CRecordVector<UInt64> InSizes;
  CRecordVector<UInt64> OutSizes;
  UInt64 TotalInSize;
  UInt64 TotalOutSize;
public:
  NWindows::NSynchronization::CCriticalSection CriticalSection;
  void Init(unsigned numItems, ICompressProgressInfo *progress);
  void Reinit(unsigned index);
  HRESULT SetRatioInfo(unsigned index, const UInt64 *inSize, const UInt64 *outSize);
};


Z7_CLASS_IMP_NOQIB_1(
  CMtCompressProgress
  , ICompressProgressInfo
)
  unsigned _index;
  CMtCompressProgressMixer *_progress;
public:
  void Init(CMtCompressProgressMixer *progress, unsigned index)
  {
    _progress = progress;
    _index = index;
  }
  void Reinit() { _progress->Reinit(_index); }
};

#endif
