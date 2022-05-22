#include "pch.h"
#include "BaseExplorerCommand.h"
#include <Shlwapi.h>


const EXPCMDFLAGS BaseExplorerCommand::Flags() { return ECF_DEFAULT; }
const  wchar_t* BaseExplorerCommand::GetIconId() { return L",-101"; }

IFACEMETHODIMP BaseExplorerCommand::GetTitle(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* name)
{
	*name = nullptr;
	return SHStrDupW(L"Open With Notepad++", name);
}

IFACEMETHODIMP BaseExplorerCommand::GetIcon(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* icon)
{
	*icon = nullptr;
	std::filesystem::path modulePath{ wil::GetModuleFileNameW<std::wstring>(wil::GetModuleInstanceHandle()) };
	const auto iconPath{ modulePath.wstring() + GetIconId() };
	return SHStrDupW(iconPath.c_str(), icon);
}

IFACEMETHODIMP BaseExplorerCommand::GetToolTip(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* infoTip)
{
	*infoTip = nullptr;
	return E_NOTIMPL;
}

IFACEMETHODIMP BaseExplorerCommand::GetCanonicalName(_Out_ GUID* guidCommandName)
{
	*guidCommandName = GUID_NULL;
	return S_OK;
}

IFACEMETHODIMP BaseExplorerCommand::GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL okToBeSlow, _Out_ EXPCMDSTATE* cmdState)
{
	*cmdState = ECS_ENABLED;
	return S_OK;
}

IFACEMETHODIMP BaseExplorerCommand::Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept try
{
	return S_OK;
}
CATCH_RETURN();

IFACEMETHODIMP BaseExplorerCommand::GetFlags(_Out_ EXPCMDFLAGS* flags)
{
	*flags = Flags();
	return S_OK;
}
IFACEMETHODIMP BaseExplorerCommand::EnumSubCommands(_COM_Outptr_ IEnumExplorerCommand** enumCommands)
{
	*enumCommands = nullptr;
	return E_NOTIMPL;
}

IFACEMETHODIMP BaseExplorerCommand::SetSite(_In_ IUnknown* site) noexcept
{
	m_site = site;
	return S_OK;
}
IFACEMETHODIMP BaseExplorerCommand::GetSite(_In_ REFIID riid, _COM_Outptr_ void** site) noexcept
{
	return m_site.CopyTo(riid, site);
}