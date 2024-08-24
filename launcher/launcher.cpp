#include <iostream>
#include <sstream>

#include "launcher.hpp"
#include "periwinkle.hpp"
#include "utils.hpp"

static std::string usage(std::string_view programName)
{
    std::stringstream ss;
    ss << "Барвінок " << periwinkle::Periwinkle::getVersionAsString() << "\n";
    ss << "Використання: " << programName << " [опції] <файл>\n";
    ss << "Опції:\n";
    ss << "\t" << "-д, --допомога     Виводить це повідомлення.\n";
#ifdef DEV_TOOLS
    ss << "\t" << "-а, --асемблер     Виводить згенерований код для віртуальної машини. Не запускає програму.\n";
#endif
    return ss.str();
}

static bool cmdOptionExists(std::span<const std::string_view> tokens, std::string_view option, std::string_view fullOption = "")
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

int launcher(std::span<const std::string_view> args) noexcept
{
    const std::string_view programName = args[0].substr(args[0].find_last_of("/\\") + 1);
    if (args.size() == 1)
    {
        std::cout << usage(programName);
        return 0;
    }

    std::span<const std::string_view> tokens(args.begin() + 1, args.end());
    std::span<const std::string_view> argsForInterpreter; // Аргументи для інтерпретатора
    std::span<const std::string_view> argsForProgram; // Аргументи для програми запущеної інтерпретатором
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        std::string_view token = tokens[i];
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
            argsForInterpreter = { tokens.begin(), tokens.begin() + i + 1 };
            argsForProgram = { tokens.begin() + i + 1, tokens.end() };
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
	return 0;
}
