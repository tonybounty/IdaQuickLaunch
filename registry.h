#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <Windows.h>
#include <cassert>


// tstring is mutant char/wchar_t STL string, it depends on TCHAR who depends on UNICODE preprocessor definition
using tstring = std::basic_string<TCHAR>;

// Exception definition
struct WinApiException {
    tstring s_error_code;
    DWORD n_error_code;
    tstring msg;
    WinApiException(tstring msg = TEXT(""), DWORD explicit_error = 0) : msg{ msg } 
    {
        if (explicit_error != 0) {
            this->n_error_code = explicit_error;
        }
        else {
            this->n_error_code = GetLastError();
        }
#ifdef UNICODE
            this->s_error_code = std::to_wstring(this->n_error_code);
#else
            this->s_error_code = std::to_string(this->n_error_code);
#endif
    };

    const tstring what() const {
        return this->msg;
    }

    const tstring formated()  const {
        constexpr auto buffer_size = 255;
        TCHAR buffer[buffer_size]{ 0 };

        const auto res = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            this->n_error_code,
            0,
            buffer,
            255,
            NULL);

        return tstring{ buffer };
    }
};
// Exception


namespace winapi {
    
    // if UNICODE is set, wrapper function call Reg*W API ortherwise it call ANSI Reg*A API 
    class Registry
    {
    public:
        [[nodiscard]] static const tstring ReadString(const HKEY hkey, const tstring& keypath);
        [[nodiscard]] static const DWORD ReadDword(const HKEY hkey, const tstring& keypath);
        static void WriteDword(const HKEY hkey, const tstring& keypath, DWORD value);
        static void WriteString(const HKEY hkey, const tstring& keypath, const tstring& value);
        [[nodiscard]] static const bool IsPathExist(const HKEY hkey, const tstring& keypath);
        static void CreateSubKey(const HKEY hkey, const tstring& keypath);
        static void DeleteEntry(const HKEY hkey, const tstring& keypath);
        [[nodiscard]] static const std::tuple<tstring, tstring> SplitSubkeyName(const tstring& keypath);
        [[nodiscard]] static std::vector<tstring> ListValues(HKEY hkey, const tstring& keypath);
        [[nodiscard]] static std::vector<tstring> ListSubkey(HKEY hkey, const tstring& keypath);
    };

}

