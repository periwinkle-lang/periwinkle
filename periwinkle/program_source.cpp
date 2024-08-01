#include <iostream>
#include <fstream>
#include <sstream>

#include "program_source.hpp"
#include "utils.hpp"

using namespace periwinkle;

std::string_view ProgramSource::getText() const
{
    return text;
}

std::filesystem::path ProgramSource::getPath() const
{
    return path;
}

std::string ProgramSource::getFilename() const
{
    if (_hasFile) return path.filename().string();
    return name;
}

bool ProgramSource::hasFile() const
{
    return _hasFile;
};

ProgramSource::ProgramSource(const std::filesystem::path& path)
    : path(path), name(path.string())
{
#ifdef _WIN32
    std::ifstream filestream(utils::convertUtf8ToWide(path.string()));
#else
    std::ifstream filestream(path);
#endif
    std::stringstream ss;
    if (filestream.is_open())
    {
        ss << filestream.rdbuf();
        filestream.close();
        this->text = ss.str();
    }
    else
    {
        std::cerr << "Неможливо відкрити файл \"" << path.string() << "\"." << std::endl;
        exit(1);
    }

    this->_hasFile = true;
}

ProgramSource::ProgramSource(const std::string& code, const std::string& name)
{
    this->name = name;
    this->text = code;
    this->_hasFile = false;
}
