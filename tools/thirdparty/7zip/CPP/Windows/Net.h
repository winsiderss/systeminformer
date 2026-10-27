// Windows/Net.h

#ifndef ZIP7_INC_WINDOWS_NET_H
#define ZIP7_INC_WINDOWS_NET_H

#include "../Common/MyString.h"
#include "../Common/MyWindows.h"

namespace NWindows {
namespace NNet {

struct CResourceBase
{
  DWORD Scope;
  DWORD Type;
  DWORD DisplayType;
  DWORD Usage;
  bool LocalNameIsDefined;
  bool RemoteNameIsDefined;
  bool CommentIsDefined;
  bool ProviderIsDefined;
};

struct CResource: public CResourceBase
{
  CSysString LocalName;
  CSysString RemoteName;
  CSysString Comment;
  CSysString Provider;
};

#ifdef _UNICODE
typedef CResource CResourceW;
#else
struct CResourceW: public CResourceBase
{
  UString LocalName;
  UString RemoteName;
  UString Comment;
  UString Provider;
};
#endif

class CEnum
{
  HANDLE _handle;
  bool _handleAllocated;
  DWORD Open(DWORD scope, DWORD type, DWORD usage, LPNETRESOURCE netResource);
  DWORD Next(LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize);
  #ifndef _UNICODE
  DWORD Open(DWORD scope, DWORD type, DWORD usage, LPNETRESOURCEW netResource);
  DWORD NextW(LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize);
  #endif
protected:
  bool IsHandleAllocated() const { return _handleAllocated; }
public:
  CEnum(): _handleAllocated(false) {}
  ~CEnum() {  Close(); }
  DWORD Close();
  DWORD Open(DWORD scope, DWORD type, DWORD usage, const CResource *resource);
  DWORD Next(CResource &resource);
  #ifndef _UNICODE
  DWORD Open(DWORD scope, DWORD type, DWORD usage, const CResourceW *resource);
  DWORD Next(CResourceW &resource);
  #endif
};

DWORD GetResourceParent(const CResource &resource, CResource &parentResource);
#ifndef _UNICODE
DWORD GetResourceParent(const CResourceW &resource, CResourceW &parentResource);
#endif

DWORD GetResourceInformation(const CResource &resource,
    CResource &destResource, CSysString &systemPathPart);
#ifndef _UNICODE
DWORD GetResourceInformation(const CResourceW &resource,
    CResourceW &destResource, UString &systemPathPart);
#endif

DWORD AddConnection2(const CResource &resource, LPCTSTR password, LPCTSTR userName, DWORD flags);
#ifndef _UNICODE
DWORD AddConnection2(const CResourceW &resource, LPCWSTR password, LPCWSTR userName, DWORD flags);
#endif

}}

#endif
