// Crypto/Rar20Crypto.h

#ifndef ZIP7_INC_CRYPTO_RAR20_CRYPTO_H
#define ZIP7_INC_CRYPTO_RAR20_CRYPTO_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

namespace NCrypto {
namespace NRar2 {

/* ICompressFilter::Init() does nothing for this filter.
   Call SetPassword() to initialize filter. */

class CData
{
  Byte SubstTable[256];
  UInt32 Keys[4];
  
  UInt32 SubstLong(UInt32 t) const
  {
    return (UInt32)SubstTable[(unsigned)t         & 255]
        | ((UInt32)SubstTable[(unsigned)(t >>  8) & 255] << 8)
        | ((UInt32)SubstTable[(unsigned)(t >> 16) & 255] << 16)
        | ((UInt32)SubstTable[(unsigned)(t >> 24)      ] << 24);
  }
  void UpdateKeys(const Byte *data);
  void CryptBlock(Byte *buf, bool encrypt);
public:
  ~CData() { Wipe(); }
  void Wipe()
  {
    Z7_memset_0_ARRAY(SubstTable);
    Z7_memset_0_ARRAY(Keys);
  }

  void EncryptBlock(Byte *buf) { CryptBlock(buf, true); }
  void DecryptBlock(Byte *buf) { CryptBlock(buf, false); }
  void SetPassword(const Byte *password, unsigned passwordLen);
};

class CDecoder Z7_final:
  public ICompressFilter,
  public CMyUnknownImp,
  public CData
{
  Z7_COM_UNKNOWN_IMP_0
  Z7_IFACE_COM7_IMP(ICompressFilter)
};

}}

#endif
