#pragma once

// https://msdn.microsoft.com/en-us/magazine/mt763237
std::wstring Utf8ToUtf16(std::string_view utf8)
{
	if (utf8.empty())
	{
		return {};
	}

	constexpr DWORD kFlags = MB_ERR_INVALID_CHARS;
	const int utf8Length = static_cast<int>(utf8.length());
	const int utf16Length = MultiByteToWideChar(
		CP_UTF8,
		kFlags,
		utf8.data(),
		utf8Length,
		nullptr,
		0
	);
	THROW_LAST_ERROR_IF(utf16Length == 0);

	std::wstring utf16(utf16Length, L'\0');
	const int result = MultiByteToWideChar(
		CP_UTF8,
		kFlags,
		utf8.data(),
		utf8Length,
		utf16.data(),
		utf16Length
	);
	THROW_LAST_ERROR_IF(result == 0);

	return utf16;
}

std::string Utf16ToUtf8(std::wstring_view utf16)
{
	if (utf16.empty())
	{
		return {};
	}

	constexpr DWORD kFlags = WC_ERR_INVALID_CHARS;
	const int utf16Length = static_cast<int>(utf16.length());
	const int utf8Length = WideCharToMultiByte(
		CP_UTF8,
		kFlags,
		utf16.data(),
		utf16Length,
		nullptr,
		0,
		nullptr, nullptr
	);
	THROW_LAST_ERROR_IF(utf8Length == 0);

	std::string utf8(utf8Length, '\0');
	const int result = WideCharToMultiByte(
		CP_UTF8,
		kFlags,
		utf16.data(),
		utf16Length,
		utf8.data(),
		utf8Length,
		nullptr, nullptr
	);
	THROW_LAST_ERROR_IF(result == 0);

	return utf8;
}

// https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/author-coclasses#add-helper-types-and-functions
// License: see the https://github.com/MicrosoftDocs/windows-uwp/blob/docs/LICENSE-CODE file
auto GetModuleFsPath(HMODULE hModule)
{
	std::wstring path(MAX_PATH, L'\0');
	DWORD actualSize;

	while (1)
	{
		actualSize = GetModuleFileNameW(hModule, path.data(), static_cast<DWORD>(path.size()));

		if (static_cast<size_t>(actualSize) + 1 > path.size())
			path.resize(path.size() * 2);
		else
			break;
	}

	path.resize(actualSize);
	return fs::path(std::move(path));
}

struct virtualallocex_mem
{
	LPVOID ptr;
	HANDLE hProcess;
	virtualallocex_mem(LPVOID ptr_, HANDLE hProcess_) noexcept { ptr = ptr_; hProcess = hProcess_; }
	operator LPVOID() const noexcept { return ptr; }
	static void free(virtualallocex_mem self) noexcept { ::VirtualFreeEx(self.hProcess, self.ptr, 0, MEM_RELEASE); }
};
using unique_virtualallocex_addr = wil::unique_any<LPVOID, decltype(virtualallocex_mem::free), virtualallocex_mem::free, wil::details::pointer_access_noaddress, virtualallocex_mem>;

auto RAIIVirtualAllocEx(_In_ HANDLE hProcess, _In_opt_ LPVOID lpAddress, _In_ SIZE_T dwSize, _In_ DWORD flAllocationType, _In_ DWORD flProtect) noexcept
{
	return unique_virtualallocex_addr(virtualallocex_mem(VirtualAllocEx(hProcess, lpAddress, dwSize, flAllocationType, flProtect), hProcess));
}
