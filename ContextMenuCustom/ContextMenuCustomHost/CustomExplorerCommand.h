#pragma once
#include "BaseExplorerCommand.h"
#include <string>

class __declspec(uuid("46F650E5-9959-48D6-AC13-A9637C5B3787"))   CustomExplorerCommand : public BaseExplorerCommand
{
public:
	const int MultipleFilesFlagEACH = 1;
	const int MultipleFilesFlagJOIN = 2;
	CustomExplorerCommand();
	IFACEMETHODIMP GetIcon(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* icon) override;
	const EXPCMDFLAGS Flags() override;
	IFACEMETHODIMP GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL okToBeSlow, _Out_ EXPCMDSTATE* cmdState) override;
	IFACEMETHODIMP GetTitle(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* name) override;
	IFACEMETHODIMP GetCanonicalName(_Out_ GUID* guidCommandName) override;
	IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept override;
	//IFACEMETHODIMP EnumSubCommands(__RPC__deref_out_opt IEnumExplorerCommand** enumCommands) override;
	void ReadCommands(bool multipeFiles, const std::wstring& current_path);

private:
	std::vector<ComPtr<IExplorerCommand>> m_commands;
	void Execute(HWND parent, const std::wstring& path);
	std::wstring _title;
	std::wstring _icon;
	std::wstring _exe;
	std::wstring _param;
	bool _accept_directory;
	std::wstring _accept_exts;
	bool _accept_multiple_files;
	int _accept_multiple_files_flag;
	std::wstring _path_delimiter;
	std::wstring _param_for_multiple_files;
};
