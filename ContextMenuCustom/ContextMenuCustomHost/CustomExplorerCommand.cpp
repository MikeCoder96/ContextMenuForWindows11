#include "pch.h"
#include "CustomExplorerCommand.h"
#include "CustomExplorerCommandEnum.h"
#include <winrt/base.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <fstream>
#include <ppltasks.h>
#include <shlwapi.h>
#include "PathHelper.hpp"
#include <ShlObj.h>
#include <iostream>

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Data::Json;
using namespace std::filesystem;

CustomExplorerCommand::CustomExplorerCommand(){
}

const EXPCMDFLAGS CustomExplorerCommand::Flags() { return ECF_HASSPLITBUTTON; }

IFACEMETHODIMP CustomExplorerCommand::GetTitle(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* name)
{
	*name = nullptr;
	winrt::hstring title = winrt::unbox_value_or<winrt::hstring>(winrt::Windows::Storage::ApplicationData::Current().LocalSettings().Values().Lookup(L"Custom_Menu_Name"), L"Open With Notepad++");
	return SHStrDupW(title.data(), name);
}

IFACEMETHODIMP CustomExplorerCommand::GetCanonicalName(_Out_ GUID* guidCommandName)
{
	*guidCommandName = __uuidof(this);
	return S_OK;
}

IFACEMETHODIMP CustomExplorerCommand::GetIcon(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* icon)
{
	*icon = nullptr;
	if (!_icon.empty()) {
		return SHStrDupW(_icon.c_str(), icon);
	}
	else {
		return BaseExplorerCommand::GetIcon(items, icon);
	}
}

IFACEMETHODIMP CustomExplorerCommand::GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL okToBeSlow, _Out_ EXPCMDSTATE* cmdState) {
	HRESULT hr;
	HWND hWnd = nullptr;
	if (m_site)
	{
		//hidden menu on the classic context menu.
		Microsoft::WRL::ComPtr<IOleWindow> oleWindow;
		m_site.As(&oleWindow);
		if (oleWindow)
		{
			// fix right click on explorer left tree view 
			//https://github.com/TortoiseGit/TortoiseGit/blob/master/src/TortoiseShell/ContextMenu.cpp
			oleWindow->GetWindow(&hWnd);
			TCHAR szWndClassName[MAX_PATH] = { 0 };
			GetClassName(hWnd, szWndClassName, MAX_PATH);
			// window class name: "NamespaceTreeControl"
			if (StrCmp(szWndClassName, L"NamespaceTreeControl"))
			{
				*cmdState = ECS_HIDDEN;
			    return S_OK;
			}
		}
	}

	if (okToBeSlow)
	{
		if (selection) {
			DWORD count;
			selection->GetCount(&count);
			if (count > 1) {
				std::wstring currentPath;
				ReadCommands(true, currentPath);
			}
			else {
				auto currentPath = PathHelper::getPath(selection);
				ReadCommands(false, currentPath);
			}
			*cmdState = ECS_ENABLED;
		}
		else {
			std::wstring currentPath;
			//fix right click on desktop 
			//https://github.com/microsoft/terminal/blob/main/src/cascadia/ShellExtension/OpenTerminalHere.cpp
			auto hwnd = ::GetForegroundWindow();
			if (hwnd)
			{
				TCHAR szName[MAX_PATH] = { 0 };
				::GetClassName(hwnd, szName, MAX_PATH);
				if (0 == StrCmp(szName, L"WorkerW") ||
					0 == StrCmp(szName, L"Progman"))
				{
					//special folder: desktop
					hr = ::SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, szName);
					if (SUCCEEDED(hr))
					{
						currentPath= szName;
					}
				}
			}
			ReadCommands(false, currentPath);
			*cmdState = ECS_HIDDEN;
		}

		if (_exe.empty()) {
			*cmdState = ECS_HIDDEN;
			return S_OK;
		}
		/*if (m_commands.size() == 0) {
			*cmdState = ECS_HIDDEN;
		}
		else {
			*cmdState = ECS_ENABLED;
		}*/

	}
	else
	{
		*cmdState = ECS_DISABLED;
		hr = E_PENDING;
	}

	return S_OK;
}

IFACEMETHODIMP CustomExplorerCommand::Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept try
{
	HWND parent = nullptr;
	if (m_site)
	{
		ComPtr<IOleWindow> oleWindow;
		RETURN_IF_FAILED(m_site.As(&oleWindow));
		RETURN_IF_FAILED(oleWindow->GetWindow(&parent));
	}

	if (selection)
	{
		DWORD count;
		selection->GetCount(&count);
		/*if (count > 1 && _accept_multiple_files_flag == MultipleFilesFlagJOIN) {
			auto paths = PathHelper::getPaths(selection, _path_delimiter);
			if (!paths.empty()) {
				auto param = _param_for_multiple_files.empty() ? std::wstring{ _param } : std::wstring{ _param_for_multiple_files };
				//get parent from first path
				if (param.find(L"{parent}") != std::wstring::npos) {
					auto firstPath = PathHelper::getPath(selection);
					if (!firstPath.empty()) {
						std::filesystem::path file(firstPath);
						PathHelper::replaceAll(param, L"{parent}", file.parent_path().wstring());
					}
				}

				PathHelper::replaceAll(param, L"{path}", paths);
				MessageBox(parent, param.c_str(), L"", 0);
				ShellExecute(parent, L"open", _exe.c_str(), param.c_str(), nullptr, SW_SHOWNORMAL);
			}
		}
		else */if (count > 1 /*&& _accept_multiple_files_flag == MultipleFilesFlagEACH*/) {
			auto paths = PathHelper::getPathList(selection);
			if (!paths.empty()) {
				for (auto& path : paths)
				{
					if (path.empty()) {
						continue;
					}
					Execute(parent, path);
				}
			}
		}
		else if (count > 0) {
			auto path = PathHelper::getPath(selection);
			Execute(parent, path);
		}

	}
	return S_OK;
}
CATCH_RETURN();

void CustomExplorerCommand::Execute(HWND parent, const std::wstring& path) {
	if (!path.empty()) {
		auto param = std::wstring{ _param };

		auto needReplaceParent = param.find(L"{parent}") != std::wstring::npos;
		auto needReplaceName = param.find(L"{name}") != std::wstring::npos;

		if (needReplaceParent || needReplaceName) {
			std::filesystem::path file(path);
			if (needReplaceParent) {
				PathHelper::replaceAll(param, L"{parent}", file.parent_path().wstring());
			}
			if (needReplaceName) {
				PathHelper::replaceAll(param, L"{name}", file.filename().wstring());
			}
		}
		PathHelper::replaceAll(param, L"{path}", path);

		ShellExecute(parent, L"open", _exe.c_str(), param.c_str(), nullptr, SW_SHOWNORMAL);
	}
}

void CustomExplorerCommand::ReadCommands(bool multipeFiles,const std::wstring& currentPath)
{
	auto menus = winrt::Windows::Storage::ApplicationData::Current().LocalSettings().CreateContainer(L"menus", ApplicationDataCreateDisposition::Always).Values();
	if (menus.Size() > 0) {
		std::wstring ext;
		bool isDirectory = true; //TODO current_path may be empty when right click on desktop.  set directory as default?
		if (!multipeFiles) {
			PathHelper::getExt(currentPath, isDirectory, ext);
		}

		auto current = menus.begin();
		do {
			if (current.HasCurrent()) {
				auto conent=winrt::unbox_value_or<winrt::hstring>(current.Current().Value(), L"");
				if (conent.size() > 0) {
					JsonObject result;
					if (JsonObject::TryParse(conent, result)) {
						_title = result.GetNamedString(L"title", L"Custom Menu");
						_exe = result.GetNamedString(L"exe", L"");
						_param = result.GetNamedString(L"param", L"");
						_icon = result.GetNamedString(L"icon", L"");
						_accept_directory = result.GetNamedBoolean(L"acceptDirectory", false);
						_accept_exts = result.GetNamedString(L"acceptExts", L"");
						_accept_multiple_files = result.GetNamedBoolean(L"acceptMultipleFiles", false);
						_path_delimiter = result.GetNamedString(L"pathDelimiter", L"");
						_param_for_multiple_files = result.GetNamedString(L"paramForMultipleFiles", L"");
						_accept_multiple_files_flag = (int)result.GetNamedNumber(L"acceptMultipleFilesFlag", 0);

						//TODO remove ,fix for 1.9 
						if (_accept_multiple_files && _accept_multiple_files_flag != MultipleFilesFlagJOIN && _accept_multiple_files_flag != MultipleFilesFlagEACH) {
							_accept_multiple_files_flag = MultipleFilesFlagJOIN;
						}
					}
				}
			}
		} while (current.MoveNext());
	}
	else {
		/*auto localFolder = ApplicationData::Current().LocalFolder().Path();
		concurrency::create_task([&]
			{
				path folder{ localFolder.c_str() };
				folder /= "custom_commands";
				if (exists(folder) && is_directory(folder)) {
					std::wstring ext;
					bool isDirectory = true; //TODO current_path may be empty when right click on desktop.  set directory as default?
					if (!multipeFiles) {
						PathHelper::getExt(currentPath, isDirectory, ext);
					}

					for (auto& file : directory_iterator{ folder })
					{
						std::ifstream fs{ file.path() };
						std::stringstream buffer;
						buffer << fs.rdbuf();//TODO 
						auto content = winrt::to_hstring(buffer.str());
						JsonObject result;
						if (JsonObject::TryParse(content, result)) {
							_title = result.GetNamedString(L"title", L"Custom Menu");
							_exe = result.GetNamedString(L"exe", L"");
							_param = result.GetNamedString(L"param", L"");
							_icon = result.GetNamedString(L"icon", L"");
							_accept_directory = result.GetNamedBoolean(L"acceptDirectory", false);
							_accept_exts = result.GetNamedString(L"acceptExts", L"");
							_accept_multiple_files = result.GetNamedBoolean(L"acceptMultipleFiles", false);
							_path_delimiter = result.GetNamedString(L"pathDelimiter", L"");
							_param_for_multiple_files = result.GetNamedString(L"paramForMultipleFiles", L"");
							_accept_multiple_files_flag = (int)result.GetNamedNumber(L"acceptMultipleFilesFlag", 0);

							//TODO remove ,fix for 1.9 
							if (_accept_multiple_files && _accept_multiple_files_flag != MultipleFilesFlagJOIN && _accept_multiple_files_flag != MultipleFilesFlagEACH) {
								_accept_multiple_files_flag = MultipleFilesFlagJOIN;
							}
							break;
						}
					}
				}
			}).wait();*/
	}
}
