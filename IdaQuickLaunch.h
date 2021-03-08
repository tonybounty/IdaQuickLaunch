#pragma once
#pragma comment(lib, "Shlwapi")


#include <Windows.h>
#include <Shlwapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <array>

#include "registry.h"

// Exceptions definition
struct BadCmdArgsException {};
struct Exception {
    tstring msg;
    Exception(tstring msg) : msg(msg) {};
    const tstring what() const {
        return this->msg;
    }
};
// Exceptions

class IdaQuickLaunch
{
public:
    void RunIda(const std::wstring& ida_path, const std::wstring& bin_path) const;
    void InstallRegistry(const std::wstring& ida_path) const;
    void UninstallRegistry() const;
    void ProcessCommandLine(LPCWSTR command_line) const;
    void DisplayMsgBox(const std::wstring& msgtxt, UINT type = MB_OK | MB_ICONSTOP) const;
    void DisplayMsgBoxUsages() const;
private:
    std::wstring GetCurrentExePath() const;
    bool Is32bitPE(const std::wstring filepath) const;
    std::wstring CheckAndFormatIDAPath(const std::wstring& folder_path, const std::wstring& idaexe_filename) const;
    const std::wstring LookupIdaPath() const;
    
    const std::wstring msgbox_title{ L"Ida Pro Quick Launch" };
    const std::wstring reg_str_contexual_menu{ L"Open with IDA Pro" };
    const std::wstring ida32{ L"ida.exe" };
    const std::wstring ida64{ L"ida64.exe" };
    const std::wstring reg_hexrays_subkey{ L"SOFTWARE\\Hex-Rays SA\\" };
    std::array<std::wstring, 3> reg_shellfile_subkeys = { 
        L"dllfile",
        L"exefile",
        L".bin"
    };
};
