#include <Windows.h>
#include <vector>

#include "launcher.hpp"
#include "utils.hpp"

int wmain(int argc, wchar_t* w_argv[])
{
	char** argv = new char* [argc];
	for (int i = 0; i < argc; ++i)
	{
		auto utf8String = utils::convertWideToUtf8(w_argv[i]);
		argv[i] = new char[utf8String.size()];
		strcpy(argv[i], utf8String.c_str());
	}
	SetConsoleOutputCP(CP_UTF8);

	std::vector<std::string_view> args(
		argv, std::next(argv, static_cast<std::ptrdiff_t>(argc))
	);

	delete[] argv;
	return launcher(args);
}
