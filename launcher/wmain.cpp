#include <Windows.h>
#include <vector>
#include <string_view>

#include "launcher.hpp"

int wmain(int argc, wchar_t* argv[])
{
	SetConsoleOutputCP(CP_UTF8);

	std::vector<std::wstring_view> args(
		argv, std::next(argv, static_cast<std::ptrdiff_t>(argc))
	);
	return launcher(args);
}
