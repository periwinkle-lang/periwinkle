#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#ifdef _WIN32
    #include<windows.h>
#endif

#include "periwinkle.hpp"
#include "utils.hpp"

using std::string;
using std::vector;

string usage(string programName)
{
    std::stringstream ss;
    ss << "Барвінок " << periwinkle::Periwinkle::getVersionAsString() << std::endl;
    ss << "Використання: " << programName << " [опції] <файл>" << std::endl;
    ss << "Опції:" << std::endl;
    ss << "\t" << "-д, --допомога     Виводить це повідомлення." << std::endl;
#ifdef DEV_TOOLS
    ss << "\t" << "-а, --асемблер     Виводить згенерований код для віртуальної машини. Не запускає програму." << std::endl;
#endif
    return ss.str();
}

bool cmdOptionExists(vector<string> tokens, string option, string fullOption = "")
{
    bool optionExists = std::find(tokens.begin(), tokens.end(), option) != tokens.end();
    bool fullOptionExists = false;
    if (fullOption != "")
    {
        fullOptionExists = std::find(tokens.begin(), tokens.end(), fullOption) != tokens.end();
    }
    return optionExists || fullOptionExists;
}

#define COMPARE_OPTION(token, option, fullOption) \
    (token == option || token == fullOption)

#ifdef _WIN32
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
#else
int main(int argc, char* argv[])
{
#endif
    auto programName = string(argv[0]);
    programName = programName.substr(programName.find_last_of("/\\") + 1);
    if (argc == 1)
    {
        std::cout << usage(programName);
        return 0;
    }

    vector<string> tokens(argv + 1, argv + argc);
    vector<string> argsForInterpreter; // Аргументи для інтерпретатора
    vector<string> argsForProgram; // Аргументи для програми запущеної інтерпретатором
    for (int i = 0; i < (int)tokens.size(); ++i)
    {
        auto token = tokens[i];
        if (COMPARE_OPTION(token, "-д", "--допомога"))
        {
            std::cout << usage(programName);
            return 0;
        }
#ifdef DEV_TOOLS
        else if (COMPARE_OPTION(token, "-а", "--асемблер"))
        {
            continue;
        }
#endif
        else if (!token.starts_with("-"))
        {
            for (int j = 0; j < (int)tokens.size(); ++j)
            {
                if (j <= i) argsForInterpreter.push_back(tokens[j]);
                else argsForProgram.push_back(tokens[j]);
            }
            break;
        }
        else
        {
            std::cout << "Невідомий аргумент: \"" << token << "\"" << std::endl;
            return 0;
        }
    }

	periwinkle::Periwinkle interpreter(std::filesystem::path(argsForInterpreter.back()));

#ifdef DEV_TOOLS
	if (cmdOptionExists(argsForInterpreter, "-а", "--асемблер"))
	{
		interpreter.printDisassemble();
		return 0;
	}
#endif
    auto result = interpreter.execute();
    if (result == nullptr) { interpreter.printException(); };

#ifdef _WIN32
    delete[] argv;
#endif
	return 0;
}
