#include "IdaQuickLaunch.h"

void IdaQuickLaunch::DisplayMsgBoxUsages() const
{
	auto current_path = this->GetCurrentExePath();
	std::wstring ws = L"command line usage: \n";
	ws += current_path.c_str();
	ws += L"  FLAG\n\n";
	ws += L"FLAGS:\n";
	ws += L"open file in IDA Pro:\n";
	ws += L"-run FILE_TO_OPEN [PATH_TO_IDA_FOLDER]\n\n";
	ws += L"add entry in explorer contextual menu:\n";
	ws += L"-install [PATH_TO_IDA_FOLDER]\n\n";
	ws += L"remove entry in explorer contextual menu:\n";
	ws += L"-uninstall";
	MessageBox(0, ws.c_str(), this->msgbox_title.c_str() , MB_OK | MB_ICONINFORMATION);
}

std::wstring IdaQuickLaunch::GetCurrentExePath() const
{
	LPWSTR* current_exe_path = nullptr;
	int useless = 0;
	current_exe_path = CommandLineToArgvW(L"", &useless); // empty str = current exe path

	if (current_exe_path == nullptr) {
		throw WinApiException(L"can't determine current executable path");
	}

	std::wstring current_path = std::wstring{ *current_exe_path };
	LocalFree(current_exe_path);
	return current_path;
}

void IdaQuickLaunch::ProcessCommandLine(LPCWSTR command_line) const
{
	int count = 0;
	std::function<void()> do_action;

	std::unique_ptr<LPWSTR, HLOCAL(__stdcall *)(HLOCAL)> 
		arguments(CommandLineToArgvW(command_line, &count), LocalFree);

	if (arguments.get() == nullptr) {
		arguments.release(); // prevent to call LocalFree on NULL ptr
		throw WinApiException(L"processing command line failed");
	}
	
	if (count < 1) {
		throw BadCmdArgsException();
	}

	std::wstring ida_path;
	std::wstring file_path;
	auto arg = arguments.get();

	if (wcscmp(arg[0], L"-install") == 0) {
		if (count >= 2) {
		do_action = std::bind(&IdaQuickLaunch::InstallRegistry, this, std::cref(arg[1]));
		}
		else {
		do_action = std::bind(&IdaQuickLaunch::InstallRegistry, this, this->LookupIdaPath());
		}
	}
	else if (wcscmp(arg[0], L"-uninstall") == 0) {
		do_action = std::bind(&IdaQuickLaunch::UninstallRegistry, this);
	}
	else if (wcscmp(arg[0], L"-run") == 0 && count >= 2) {
			file_path = std::wstring{arg[1]};
			if (count >= 3) {
				ida_path = std::wstring{ arg[2] };
			}
			else {
				ida_path = this->LookupIdaPath();
			}
			do_action = std::bind(&IdaQuickLaunch::RunIda, this, std::cref(ida_path), std::cref(file_path));
	}
	else {
		throw BadCmdArgsException();
	}
	
	assert(do_action != nullptr);
    do_action();
}

std::wstring IdaQuickLaunch::CheckAndFormatIDAPath(const std::wstring& folder_path, const std::wstring& idaexe_filename) const
{
	std::wstring file_path;
	if (folder_path.back() != L'\\') {
		file_path = folder_path + std::wstring(L"\\") + idaexe_filename;
	}
	else {
		file_path = folder_path + idaexe_filename;
	}

    if (!PathFileExistsW(file_path.c_str())) {
        throw WinApiException(std::wstring(L"file path not found : ") + file_path);
    }

	return file_path;
}

bool IdaQuickLaunch::Is32bitPE(const std::wstring filepath) const
{
	bool res = false;

	std::ifstream ifs{ filepath };

	if (!ifs.is_open()) {
		throw Exception{ L"open file failed : " + filepath };
	}

	DWORD img_nt_hdr = 0;
	char pe_str[2]{ 0 };
	WORD arch_type = 0;
	try {
		// IMAGE DOS HEADER
		ifs.seekg(0x3c);
		ifs.read(reinterpret_cast<char*>(&img_nt_hdr), 4);

		// Check if PEFile
		ifs.seekg(img_nt_hdr);
		ifs.read(reinterpret_cast<char*>(&pe_str), 2);

		// Check ARCH32/64
		ifs.seekg(img_nt_hdr + 4);
		ifs.read(reinterpret_cast<char*>(&arch_type), 2);
	}
	catch (std::exception e) {
		throw Exception{ L"not a PE file" };
	}

	if (pe_str[0] != 'P' || pe_str[1] != 'E') {
		throw Exception{ L"not a PE File" };
	}

	if (arch_type == 0x014c) {
		res = true;
	}
	else if (arch_type == 0x8664) {
		res = false;
	}
	else {
		throw Exception{ L"not an arch 32/64 bits" };
	}

	return res;
}

void IdaQuickLaunch::DisplayMsgBox(const std::wstring& msgtxt, UINT type /*= MB_OK | MB_ICONSTOP*/) const
{
	MessageBox(0, msgtxt.c_str(), this->msgbox_title.c_str(), type);
}

void IdaQuickLaunch::RunIda(const std::wstring& ida_path, const std::wstring& bin_path) const {
	std::wstring ida_full_path;

	ida_full_path.reserve(MAX_PATH);
	if (this->Is32bitPE(bin_path)) {
         ida_full_path = this->CheckAndFormatIDAPath(ida_path, ida32);
	}
	else {
		 ida_full_path = this->CheckAndFormatIDAPath(ida_path, ida64);
	}

	ida_full_path += L" " + bin_path;

	STARTUPINFOW startupinfo { 0 };
	PROCESS_INFORMATION processinfo{ 0 };

	if (!CreateProcessW(NULL,ida_full_path.data() , NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startupinfo, &processinfo)) {
		throw WinApiException(L"IDA Pro execution failed");
	}
}

void IdaQuickLaunch::InstallRegistry(const std::wstring& ida_path) const
{
	auto current_path = this->GetCurrentExePath();
	
	auto key_value = L"\"" + current_path + L"\" -run \"%1\" \"" + ida_path + L"\\\"";
	auto value_icon = ida_path + L"ida.exe";

	for (auto& name : this->reg_shellfile_subkeys) {
        auto key_command = name + L"\\shell\\" + this->reg_str_contexual_menu + L"\\command\\";
        auto key_icon = name + L"\\shell\\" + this->reg_str_contexual_menu + L"\\Icon";
		try {
			winapi::Registry::WriteString(HKEY_CLASSES_ROOT, key_command, key_value);
			winapi::Registry::WriteString(HKEY_CLASSES_ROOT, key_icon, value_icon);
		}
		catch (WinApiException e) {
			if (e.n_error_code == ERROR_ACCESS_DENIED) {
				throw Exception(L"Installation in the registry failed, you must run with elevated privilege (ERROR_ACCESS_DENIED)");
			}
			else {
				throw Exception(L"Installation in the registry failed, error: " + e.formated());
			}
		}
	}
	this->DisplayMsgBox(L"Installation completed.", MB_OK | MB_ICONINFORMATION);
}

void IdaQuickLaunch::UninstallRegistry() const
{
	for (auto& name : this->reg_shellfile_subkeys) {
        auto key_path = name + L"\\shell\\" + this->reg_str_contexual_menu + L"\\";
		try {
			winapi::Registry::DeleteEntry(HKEY_CLASSES_ROOT, key_path);
		}
		catch (WinApiException e) {
			if (e.n_error_code == ERROR_ACCESS_DENIED) {
			throw Exception(L"Uninstall failed, you must run with elevated privilege (ERROR_ACCESS_DENIED)");
			}
		}
	}
	this->DisplayMsgBox(L"Uninstall completed.", MB_OK | MB_ICONINFORMATION);
}

const std::wstring IdaQuickLaunch::LookupIdaPath() const
{
	std::wstring idapath;

	if (!winapi::Registry::IsPathExist(HKEY_LOCAL_MACHINE ,this->reg_hexrays_subkey)) {
		throw Exception{ L"Can't determine where is installed IDA Pro, check command usage to manually specify IDA Pro path." };
	}

	auto ida_subkeys = winapi::Registry::ListSubkey(HKEY_LOCAL_MACHINE, this->reg_hexrays_subkey);
	if (ida_subkeys.empty()) {
		throw Exception{ L"IDA Pro does not seem installed. If so please give the IDA folder path in command line argument." };
	}
	else if (ida_subkeys.size() > 1) {
		throw Exception{ L"It seems that several version of IDA Pro are installed. Please give the IDA Pro folder path in command line argument" };
	}

	try {
		idapath = winapi::Registry::ReadString(HKEY_LOCAL_MACHINE, this->reg_hexrays_subkey +
			ida_subkeys.at(0) + std::wstring{ L"\\Location" });
	}
	catch (WinApiException e) {
		throw Exception{ L"Can't determine where is installed IDA Pro, check command usage to manually specify IDA Pro path." };
	}

	return idapath + std::wstring{ L"\\" };
}
