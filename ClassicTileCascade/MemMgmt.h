/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `MemMgmt.cpp` for details.
 */
#pragma once

// Type definitions of smart pointers, with custom deleters for various Win32 object types
// Also includes a utility class (CCoInitialize) for automatically calling CoInitialize and 
// CoUnitialize at entry/exit of scope
// Finally, includes a class that simplifies using std::basic_string as a buffer for C-style
// char* routines. Upon construction, this class takes a std::basic_string, optionally resizes to a buffer size.
// Also will optionally remove the null terminator and anything after it upon destruction

// Custom deleters
template<typename T, auto ( __stdcall* Fn)(T)>
struct MM_Deleter {
    void operator()(T handle)
    {
        if (handle) {
            (*Fn)(handle);
        }
    }

    using pointer = T;
};


struct FILE_deleter {
    void operator()(FILE* pFile)
    {
        if (pFile) {
            ::fclose(pFile);
        }
    }
};


// Type definitions of various smart pointers for use with Win32 object types
using SPHKEY = std::unique_ptr<HKEY, MM_Deleter<HKEY, ::RegCloseKey>>;
using SPHMENU = std::unique_ptr<HMENU, MM_Deleter<HMENU, ::DestroyMenu>>;
using SPHWND = std::unique_ptr<HWND, MM_Deleter<HWND, ::DestroyWindow>>;
using SPHICON = std::unique_ptr<HICON, MM_Deleter<HICON, ::DestroyIcon>>;
using SPFILE = std::unique_ptr<FILE, FILE_deleter>;
using SPHANDLE_EX = std::unique_ptr<HANDLE, MM_Deleter<HANDLE, ::CloseHandle>>;
using SPHMODULE = std::unique_ptr<HMODULE, MM_Deleter<HMODULE, ::FreeLibrary>>;

// Utility class (CCoInitialize) for automatically calling CoInitialize and 
// CoUnitialize at entry/exit of scope
struct CCoInitialize
{
    CCoInitialize()
    {
        ::CoInitialize(NULL);
    }

    ~CCoInitialize()
    {
        ::CoUninitialize();
    }
};


template<class T>
class basic_sz_buf
{
public:
    using value_type = T::value_type;
    using size_type = T::size_type;

    basic_sz_buf() = delete;
    basic_sz_buf(const basic_sz_buf&) = default;
    basic_sz_buf(basic_sz_buf&&) noexcept = default;
    basic_sz_buf& operator=(const basic_sz_buf&) = default;
    basic_sz_buf& operator=(basic_sz_buf&&) noexcept = default;

    basic_sz_buf(T& sz, const std::optional<size_type>& nSize = {}, bool bRemoveNulls = true)
        : m_sz(sz), m_bRemoveNulls(bRemoveNulls)
    {
        if (nSize) {
            m_sz.resize(*nSize);
        }
    }


    virtual ~basic_sz_buf()
    {
        if (m_bRemoveNulls) {
            remove_from_null();
        }
    }


    virtual value_type* data()
    {
        return m_sz.data();
    }

    virtual operator value_type*()
    {
        return data();
    }

    virtual T& get_string()
    {
        return m_sz;
    }

    virtual operator T& ()
    {
        return get_string();
    }

    virtual size_type remove_from_null()
    {
        size_type nNullChar = m_sz.find(static_cast<value_type>(0));
        if (nNullChar != T::npos) {
            m_sz.erase(nNullChar);
        }
        return nNullChar;
    }


    virtual bool get_remove_null() const
    {
        return m_bRemoveNulls;
    }

    virtual void set_remove_null(bool bRemoveNulls)
    {
        m_bRemoveNulls = bRemoveNulls;
    }

    virtual size_type size() const
    {
        return m_sz.size();
    }

protected:
    T& m_sz;
    bool m_bRemoveNulls;

};

using sz_buf = basic_sz_buf<std::string>;
using sz_wbuf = basic_sz_buf<std::wstring>;
