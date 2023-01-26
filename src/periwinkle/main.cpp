#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "periwinkle.h"

#ifdef _WIN32
    #include "windows.h"

std::wstring convertUtf8ToWide(const std::string& str)
{
    int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], count);
    return wstr;
}

std::string convertWideToUtf8(const std::wstring& wstr)
{
    int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string str(count, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
    return str;
}
#endif


std::string readCode(std::string filename)
{
#ifdef _WIN32
    std::ifstream filestream(convertUtf8ToWide(filename));
#else
    std::ifstream filestream(filename);
#endif

    std::stringstream ss;
    if (filestream.is_open())
    {
        ss << filestream.rdbuf();
        filestream.close();
        return ss.str();
    }
    else
    {
        std::cerr << "Неможливо відкрити файл \"" << filename << "\"." << std::endl;
        exit(1);
    }
}

std::string usage(std::string programName)
{
    std::stringstream ss;
    ss << "Барвінок " << periwinkle::Periwinkle::getVersionAsString() << std::endl;
    ss << "Використання: " << programName << " [опції] <файл.бр | файл.барвінок>" << std::endl;
    ss << "Опції:" << std::endl;
    ss << "\t" << "-д, --допомога     Виводить це повідомлення." << std::endl;
#ifdef DEBUG
    ss << "\t" << "-т, --токени       Виводить токени на які розбився код. Не запускає програму." << std::endl;
    ss << "\t" << "-а, --асемблер     Виводить згенерований код для віртуальної машини. Не запускає програму." << std::endl;
#endif
    return ss.str();
}

std::string getCmdOption(std::vector<std::string> tokens, std::string option, std::string fullOption = "")
{
    auto itr = std::find(tokens.begin() + 1, tokens.end(), option);
    if (itr != tokens.end() && ++itr != tokens.end())
    {
        return *itr;
    }

    if (fullOption != "")
    {
        auto fullItr = std::find(tokens.begin() + 1, tokens.end(), fullOption);
        if (fullItr != tokens.end() && ++fullItr != tokens.end())
        {
            return *fullItr;
        }
    }

    std::cerr << "Опція \"" << fullOption << "\" відсутня!" << std::endl;
    exit(1);
}

bool cmdOptionExists(std::vector<std::string> tokens, std::string option, std::string fullOption = "")
{
    bool optionExists = std::find(tokens.begin() + 1, tokens.end(), option) != tokens.end();
    bool fullOptionExists = false;
    if (fullOption != "")
    {
        fullOptionExists = std::find(tokens.begin() + 1, tokens.end(), fullOption) != tokens.end();
    }
    return optionExists || fullOptionExists;
}

std::string getFilename(std::vector<std::string> tokens)
{
    auto result = std::find_if(tokens.begin() + 1, tokens.end(),
        [](std::string i) { return i.ends_with(".бр") || i.ends_with(".барвінок"); });

    if (result == tokens.end())
    {
        std::cout << usage(tokens[0]);
        exit(0);
    }

    return *result;
}

#ifdef _WIN32
int wmain(int argc, wchar_t* w_argv[])
{
	char** argv = new char* [argc];
	for (int i = 0; i < argc; ++i)
	{
		auto utf8String = convertWideToUtf8(std::wstring(w_argv[i]));
		argv[i] = new char[utf8String.size()];
		strcpy(argv[i], utf8String.c_str());
	}
	SetConsoleOutputCP(CP_UTF8);
#else
int main(int argc, char* argv[])
{
#endif
    auto programName = std::string(argv[0]);
    programName = programName.substr(programName.find_last_of("/\\") + 1);
    if (argc == 1)
    {
        std::cout << usage(programName);
        return 0;
    }

    std::vector<std::string> tokens;
    for (int i = 0; i < argc; ++i)
    {
        tokens.push_back(std::string(argv[i]));
    }

	if (cmdOptionExists(tokens, "-д", "--допомога"))
	{
		std::cout << usage(programName);
		return 0;
	}

    auto filename = getFilename(tokens);

	auto code = readCode(filename);
	periwinkle::Periwinkle interpreter(code);

#ifdef DEBUG
	if (cmdOptionExists(tokens, "-т", "--токени"))
	{
		interpreter.printTokens();
		return 0;
	}
	if (cmdOptionExists(tokens, "-а", "--асемблер"))
	{
		interpreter.printDisassemble();
		return 0;
	}
#endif
	interpreter.execute();

	return 0;
}
