// MyXml.h

#ifndef ZIP7_INC_MY_XML_H
#define ZIP7_INC_MY_XML_H

#include "MyString.h"

struct CXmlProp
{
  AString Name;
  AString Value;
};

class CXmlItem
{
public:
  AString Name;
  bool IsTag;
  CObjectVector<CXmlProp> Props;
  CObjectVector<CXmlItem> SubItems;
  
  const char * ParseItem(const char *s, int numAllowedLevels);

  bool IsTagged(const char *tag) const throw();
  int FindProp(const char *propName) const throw();
  AString GetPropVal(const char *propName) const;
  AString GetSubString() const;
  const AString * GetSubStringPtr() const throw();
  int FindSubTag(const char *tag) const throw();
  const CXmlItem *FindSubTag_GetPtr(const char *tag) const throw();
  AString GetSubStringForTag(const char *tag) const;
  void AppendTo(AString &s) const;
};

struct CXml
{
  CXmlItem Root;

  bool Parse(const char *s);
  // void AppendTo(AString &s) const;
};

void z7_xml_DecodeString(AString &s);

#endif
