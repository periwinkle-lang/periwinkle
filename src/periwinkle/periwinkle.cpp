#include <array>

#include "periwinkle.h"
#include "vm.h"
#include "parser.hpp"
#include "compiler.h"
#include "utils.h"
#include "pconfig.h"

#include "string_object.h"

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
    PParser::Parser parser(code);
    compiler::Compiler comp(parser.parse(), code);
    std::array<vm::Object*, 512> stack{};
    auto frame = comp.compile();
    frame->sp = &stack[0];
    frame->bp = &stack[0];
    vm::VirtualMachine virtualMachine(frame);
    virtualMachine.execute();
}

#ifdef DEBUG

#include <iomanip>
#include "disassembler.h"

void periwinkle::Periwinkle::printDisassemble()
{
    PParser::Parser parser(code);
    compiler::Compiler comp(parser.parse(), code);
    compiler::Disassembler disassembler;
    std::cout << disassembler.disassemble(comp.compile()->codeObject);
}
#endif

periwinkle::Periwinkle::Periwinkle(std::string code)
{
    //utils::replaceTabToSpace(code);
    this->code = code;
};
