// LzOutWindow.cpp

#include "StdAfx.h"

#include "LzOutWindow.h"

void CLzOutWindow::Init(bool solid) throw()
{
  if (!solid)
    COutBuffer::Init();
  #ifdef Z7_NO_EXCEPTIONS
  ErrorCode = S_OK;
  #endif
}
