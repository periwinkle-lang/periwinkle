#include <vector>

#include "launcher.hpp"

int main(int argc, char* argv[])
{
    std::vector<std::string_view> args(
        argv, std::next(argv, static_cast<std::ptrdiff_t>(argc))
    );
    return launcher(args);
}
