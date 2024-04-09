#include <array>
#include <functional>

#include "periwinkle.hpp"
#include "vm.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "utils.hpp"
#include "pconfig.hpp"
#include "string_object.hpp"

using namespace periwinkle;

int periwinkle::Periwinkle::getVersionAsInt()
{
    return PERIWINKLE_VERSION_MAJOR * 10000 + PERIWINKLE_VERSION_MINOR * 100 + PERIWINKLE_VERSION_PATCH;
}

std::string periwinkle::Periwinkle::getVersionAsString() { return std::string(PERIWINKLE_VERSION); }
int periwinkle::Periwinkle::majorVersion() { return PERIWINKLE_VERSION_MAJOR; }
int periwinkle::Periwinkle::minorVersion() { return PERIWINKLE_VERSION_MINOR; }
int periwinkle::Periwinkle::patchVersion() { return PERIWINKLE_VERSION_PATCH; }

void periwinkle::Periwinkle::execute()
{
    using namespace std::placeholders;
    PParser::Parser parser(source->getText());
    parser.setErrorHandler(std::bind(
        static_cast<void(*)(ProgramSource*, std::string, size_t)>(utils::throwSyntaxError), source, _1, _2));
    auto ast = parser.parse();
    if (!ast.has_value()) { exit(1); }
    compiler::Compiler comp(ast.value(), source);
    std::array<vm::Object*, 512> stack{};
    auto frame = comp.compile();
    frame->sp = &stack[0];
    frame->bp = &stack[0];
    vm::VirtualMachine virtualMachine(frame);
    virtualMachine.execute();
}

#ifdef DEBUG

#include <iomanip>
#include "disassembler.hpp"

void periwinkle::Periwinkle::printDisassemble()
{
    PParser::Parser parser(source->getText());
    auto ast = parser.parse();
    if (!ast.has_value()) { exit(1); }
    compiler::Compiler comp(ast.value(), source);
    compiler::Disassembler disassembler;
    std::cout << disassembler.disassemble(comp.compile()->codeObject);
}
#endif

periwinkle::Periwinkle::Periwinkle(const std::string& code)
    : source(new ProgramSource(code))
{
}

periwinkle::Periwinkle::Periwinkle(const std::filesystem::path& path)
    : source(new ProgramSource(path))
{
}

periwinkle::Periwinkle::Periwinkle(const ProgramSource& source)
    : source(new ProgramSource(source))
{
}
