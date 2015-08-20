#pragma once

#include "IUnknownInterfaceImpl.h"
#include <string>

#include "SDK/apiObjects.h"

class AIMPString : public IUnknownInterfaceImpl<IAIMPString> {
public:

    AIMPString(const std::wstring& string);
    AIMPString(std::wstring* string, bool weak_ref);
    AIMPString(std::wstring::value_type* raw_string, std::wstring::size_type count);

    virtual ~AIMPString();

    virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* ppvObj);
    
    virtual HRESULT WINAPI GetChar(int Index, WCHAR *Char);
    virtual WCHAR* WINAPI GetData();
    virtual int WINAPI GetLength();
    virtual int WINAPI GetHashCode();
    virtual HRESULT WINAPI SetChar(int Index, WCHAR Char);
    virtual HRESULT WINAPI SetData(WCHAR* Chars, int CharsCount);

    virtual HRESULT WINAPI Add(IAIMPString* S);
    virtual HRESULT WINAPI Add2(WCHAR* Chars, int CharsCount);

    virtual HRESULT WINAPI ChangeCase(int Mode);
    virtual HRESULT WINAPI Clone(IAIMPString **S);

    virtual HRESULT WINAPI Compare(IAIMPString* S, int* CompareResult, bool IgnoreCase);
    virtual HRESULT WINAPI Compare2(WCHAR* Chars, int CharsCount, int* CompareResult, bool IgnoreCase);

    virtual HRESULT WINAPI Delete(int Index, int Count);

    virtual HRESULT WINAPI Find(IAIMPString* S, int *Index, int Flags, int StartFromIndex);
    virtual HRESULT WINAPI Find2(WCHAR *Chars, int CharsCount, int *Index, int Flags, int StartFromIndex);

    virtual HRESULT WINAPI Insert(int Index, IAIMPString *S);
    virtual HRESULT WINAPI Insert2(int Index, WCHAR *Chars, int CharsCount);

    virtual HRESULT WINAPI Replace(IAIMPString *OldPattern, IAIMPString *NewPattern, int Flags);
    virtual HRESULT WINAPI Replace2(WCHAR *OldPatternChars, int OldPatternCharsCount,
                                    WCHAR *NewPatternChars, int NewPatternCharsCount, int Flags);

    virtual HRESULT WINAPI SubString(int Index, int Count, IAIMPString **S);

private:
    AIMPString(const AIMPString&);
    AIMPString& operator=(const AIMPString&);
    std::wstring* string_;
    bool weak_ref_;
};
