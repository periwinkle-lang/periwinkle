#include <iostream>

#include "platform.hpp"

using namespace platform;

std::string platform::readline()
{
    std::string line;
    std::getline(std::cin, line);
    return line;
}
