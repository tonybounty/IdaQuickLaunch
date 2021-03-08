#include "registry.h"

const tstring winapi::Registry::ReadString(const HKEY hkey, const tstring& keypath)
{
    DWORD value_size = 0;

    auto [subkey, name] = SplitSubkeyName(keypath);
    
    auto err = RegGetValue(
        hkey,
        subkey.data(),
        name.data(),
        RRF_RT_REG_SZ,
        NULL,
        NULL,
        &value_size);
    if (err != ERROR_SUCCESS) {
        throw WinApiException(TEXT(""), err);
    } 

    auto value_data = std::make_unique<TCHAR[]>(value_size/sizeof(TCHAR));
    err = RegGetValue(
        hkey,
        subkey.data(),
        name.data(),
        RRF_RT_REG_SZ,
        NULL,
        value_data.get(),
        &value_size);

    if (err != ERROR_SUCCESS) {
        throw WinApiException(TEXT(""), err);
    }


    return tstring(value_data.get());
}

const DWORD winapi::Registry::ReadDword(const HKEY hkey, const tstring& keypath)
{
    DWORD value = 0;
    DWORD size = sizeof(value);
    auto [subkey, name] = SplitSubkeyName(keypath);
    
    const auto err = RegGetValue(
        hkey,
        subkey.data(),
        name.data(),
        RRF_RT_REG_DWORD,
        NULL,
        &value,
        &size);

    if (err != ERROR_SUCCESS) {
        throw WinApiException(TEXT(""), err);
    } 
    return value;
}

void winapi::Registry::WriteDword(const HKEY hkey, const tstring& keypath, DWORD value)
{
    auto [subkey, name] = SplitSubkeyName(keypath);
    const auto err = RegSetKeyValue(
        hkey,
        subkey.c_str(),
        name.c_str(),
        REG_DWORD,
        &value,
        sizeof(value));

    if (err != ERROR_SUCCESS) {
        throw WinApiException(TEXT(""), err);
    }
}

void winapi::Registry::WriteString(const HKEY hkey, const tstring& keypath, const tstring& value) 
{
    auto [subkey, name] = SplitSubkeyName(keypath);
    const auto err = RegSetKeyValue(
        hkey,
        subkey.c_str(),
        name.c_str(),
        REG_SZ,
        value.c_str(), 
        (value.size() * sizeof(TCHAR)) + sizeof(TCHAR));

    if (err != ERROR_SUCCESS) {
        throw WinApiException(TEXT(""), err);
    }

}

const bool winapi::Registry::IsPathExist(const HKEY hkey, const tstring& keypath)
{
    auto [subkey, name] = SplitSubkeyName(keypath);
    LSTATUS err = 1;
    bool res = false;
    HKEY h = 0;
    
    if (name.empty()) {
        err = RegOpenKeyEx(hkey, subkey.c_str(), 0, KEY_READ, &h);
        if (!err) {
            RegCloseKey(h);
        }
    }
    else {
        err = RegGetValue(hkey, subkey.c_str(), name.c_str(), RRF_RT_ANY, NULL, NULL, NULL);
    }

    if (err == ERROR_SUCCESS) {
        res = true;
    }

    return res;
}

void winapi::Registry::CreateSubKey(const HKEY hkey, const tstring& keypath)
{
    LSTATUS err = 1;
    HKEY h = 0;

    err = RegCreateKeyEx(
        hkey,
        keypath.c_str(),
        NULL,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &h,
        NULL);
    
    if (err != ERROR_SUCCESS) {
        throw WinApiException(TEXT(""), err);
    }
    else {
        RegCloseKey(h);
    }

}

void winapi::Registry::DeleteEntry(const HKEY hkey, const tstring& keypath)
{
    auto [subkey, name] = SplitSubkeyName(keypath);
    LSTATUS err = 1;

    if (name.empty()) {
        err = RegDeleteTree(hkey, subkey.c_str());
    }
    else {
        err = RegDeleteKeyValue(hkey, subkey.c_str(), name.c_str());
    }

    if (err != ERROR_SUCCESS) {
        throw WinApiException(TEXT(""), err);
    }
}

const std::tuple<tstring, tstring> winapi::Registry::SplitSubkeyName(const tstring& keypath)
{
    tstring name;
    tstring subkey;
    const auto last_bslash = keypath.rfind(TEXT('\\'));

    if (last_bslash != std::string::npos) {
        name = keypath.substr(last_bslash + 1, std::string::npos);
        subkey = keypath.substr(0, last_bslash);
    }
    else {
        name = tstring(keypath.c_str());
    }

    return { subkey, name };
}

std::vector<tstring> winapi::Registry::ListValues(HKEY hkey, const tstring& keypath)
{
    constexpr DWORD keyname_size = 255;
    std::vector<tstring> list;
    DWORD size = keyname_size;
    DWORD dwIndex = 0;
    TCHAR value_name[keyname_size]{ 0 };
    LSTATUS err = 1;
    HKEY h;

    err = RegOpenKeyEx(hkey, keypath.c_str(), 0, KEY_READ, &h);
    if (err != ERROR_SUCCESS) {
        throw WinApiException(TEXT(""), err);
    }

    err = RegEnumValue(h, dwIndex, value_name, &size, NULL, NULL, NULL, NULL);

    while (err == ERROR_SUCCESS || err != ERROR_NO_MORE_ITEMS) {
        list.push_back(value_name);
        ++dwIndex;
        size = keyname_size;
        err = RegEnumValue(h, dwIndex, value_name, &size, NULL, NULL, NULL, NULL);
    }

    RegCloseKey(h);
    return list;

}

std::vector<tstring> winapi::Registry::ListSubkey(HKEY hkey, const tstring& keypath)
{
    constexpr DWORD valuename_size = 16383;
    std::vector<tstring> list;
    DWORD size = valuename_size;
    DWORD dwIndex = 0;
    TCHAR key_name[16383]{ 0 };
    LSTATUS err = 1;
    HKEY h;

    err = RegOpenKeyEx(hkey, keypath.c_str(), 0, KEY_ENUMERATE_SUB_KEYS, &h);
    if (err != ERROR_SUCCESS) {
        throw WinApiException(TEXT(""), err);
    }

    err = RegEnumKeyEx(h, dwIndex, key_name, &size, NULL, NULL, NULL, NULL);

    while (err == ERROR_SUCCESS || err != ERROR_NO_MORE_ITEMS) {
        list.push_back(tstring(key_name));
        ++dwIndex;
        size = valuename_size;
        err = RegEnumKeyEx(h, dwIndex, key_name, &size, NULL, NULL, NULL, NULL);
    }

    RegCloseKey(h);
    return list;

}
