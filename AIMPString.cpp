#include "AIMPString.h"

AIMPString::AIMPString(std::wstring* string, bool weak_ref)
    :
    string_(weak_ref ? string : new std::wstring(*string)),
    weak_ref_(weak_ref)
{
//    assert(string);
}

AIMPString::AIMPString(std::wstring::value_type* raw_string, std::wstring::size_type count)
    :
    string_(new std::wstring(raw_string, count)),
    weak_ref_(false)
{
}

AIMPString::AIMPString(const std::wstring &string)
:
string_(new std::wstring(string)),
weak_ref_(false) {
    //AddRef();
}

AIMPString::AIMPString(const AIMPString& rhs)
    :
    string_(rhs.weak_ref_ ? rhs.string_ : new std::wstring(*rhs.string_)),
    weak_ref_(rhs.weak_ref_)
{
}

AIMPString::~AIMPString()
{
    if (!weak_ref_) {
        delete string_;
    }
}

HRESULT WINAPI AIMPString::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (!ppvObj) {
        return E_POINTER;
    }

    if (IID_IUnknown == riid) {
        *ppvObj = this;
        AddRef();
        return S_OK;
    } else if (IID_IAIMPString == riid) {
        *ppvObj = this;
        AddRef();
        return S_OK;                
    }

    return E_NOINTERFACE;
}

HRESULT WINAPI AIMPString::GetChar(int index, WCHAR *ch)
{
    if (0 <= index && static_cast<std::wstring::size_type>(index) < string_->length() && ch) {
        *ch = (*string_)[index];
        return S_OK;
    }
    return E_INVALIDARG;
}

WCHAR* WINAPI AIMPString::GetData()
{
    if (GetLength() <= 0)
        return nullptr;
    return &(*string_->begin());
}

int WINAPI AIMPString::GetLength()
{
    return string_->length();
}

int WINAPI AIMPString::GetHashCode()
{
//    assert(!__FUNCTION__" is not implemented");
    return 0;
}

HRESULT WINAPI AIMPString::SetChar(int index, WCHAR ch)
{
    if (0 <= index && static_cast<std::wstring::size_type>(index) < string_->length()) {
        (*string_)[index] = ch;
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT WINAPI AIMPString::SetData(WCHAR* chars, int charsCount)
{
    if (chars) {
        string_->assign(chars, charsCount);
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT WINAPI AIMPString::Add(IAIMPString* s)
{
    if (s) {
        string_->append(s->GetData(), s->GetLength());
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT WINAPI AIMPString::Add2(WCHAR* chars, int charsCount)
{
    if (chars) {
        string_->append(chars, charsCount);
        return S_OK;
    }
    return E_INVALIDARG;    
}

HRESULT WINAPI AIMPString::ChangeCase(int /*Mode*/)
{
    return E_NOTIMPL; ///!!! TODO: impement
}

HRESULT WINAPI AIMPString::Clone(IAIMPString **s)
{
    if (s) {
        *s = new AIMPString(*this);
        (*s)->AddRef();
        return S_OK;
    }
    return E_INVALIDARG;  
}

HRESULT WINAPI AIMPString::Compare(IAIMPString* /*S*/, int* /*CompareResult*/, bool /*IgnoreCase*/)
{
    return E_NOTIMPL; ///!!! TODO: impement
}

HRESULT WINAPI AIMPString::Compare2(WCHAR* /*Chars*/, int /*CharsCount*/, int* /*CompareResult*/, bool /*IgnoreCase*/)
{
    return E_NOTIMPL; ///!!! TODO: impement
}

HRESULT WINAPI AIMPString::Delete(int index, int count)
{
    if (count >= 0 && index >= 0 && static_cast<std::wstring::size_type>(index + count) <= string_->length()) {
        string_->erase(index, count);
        return S_OK;
    }
    return E_INVALIDARG; 
}

HRESULT WINAPI AIMPString::Find(IAIMPString* /*S*/, int* /*Index*/, int /*Flags*/, int /*StartFromIndex*/)
{
    return E_NOTIMPL; ///!!! TODO: impement
}

HRESULT WINAPI AIMPString::Find2(WCHAR* /*Chars*/, int /*CharsCount*/, int* /*Index*/, int /*Flags*/, int /*StartFromIndex*/)
{
    return E_NOTIMPL; ///!!! TODO: impement
}

HRESULT WINAPI AIMPString::Insert(int index, IAIMPString *s)
{
    if (s && 0 <= index && static_cast<std::wstring::size_type>(index) < string_->length()) {
        string_->insert(index, s->GetData(), s->GetLength());
        return S_OK;
    }
    return E_INVALIDARG; 
}

HRESULT WINAPI AIMPString::Insert2(int index, WCHAR *chars, int charsCount)
{
    if (chars && 0 <= index && static_cast<std::wstring::size_type>(index) < string_->length()) {
        string_->insert(index, chars, charsCount);
        return S_OK;
    }
    return E_INVALIDARG; 
}

HRESULT WINAPI AIMPString::Replace(IAIMPString* /*OldPattern*/, IAIMPString* /*NewPattern*/, int /*Flags*/)
{
    return E_NOTIMPL; ///!!! TODO: impement
}

HRESULT WINAPI AIMPString::Replace2(WCHAR* /*OldPatternChars*/, int /*OldPatternCharsCount*/,
                                    WCHAR* /*NewPatternChars*/, int /*NewPatternCharsCount*/, int /*Flags*/)
{
    return E_NOTIMPL; ///!!! TODO: impement
}

HRESULT WINAPI AIMPString::SubString(int index, int count, IAIMPString **s)
{
    if (s && count >= 0 && 0 <= index && static_cast<std::wstring::size_type>(index + count) <= string_->length()) {
        *s = new AIMPString(GetData() + index, count);
        (*s)->AddRef();
        return S_OK;
    }
    return E_INVALIDARG;  
}
