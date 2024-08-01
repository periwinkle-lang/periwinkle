#ifndef PROGRAM_SOURCE_HPP
#define PROGRAM_SOURCE_HPP

#include <string>
#include <string_view>
#include <filesystem>

#include "exports.hpp"

namespace periwinkle
{
    class API ProgramSource
    {
    private:
        std::filesystem::path path;
        std::string name;
        std::string text;
        bool _hasFile;

    public:
        std::string_view getText() const;
        std::filesystem::path getPath() const;
        std::string getFilename() const;
        bool hasFile() const;

        ProgramSource(const std::filesystem::path& path);
        ProgramSource(const std::string& code, const std::string& name = "код");
    };
}

#endif
