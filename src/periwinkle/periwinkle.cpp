#include <array>
#include <functional>

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
    using namespace std::placeholders;
    PParser::Parser parser(code);
    parser.setErrorHandler(std::bind(static_cast<void(*)(const std::string&, std::string, size_t)>(throwSyntaxError), code, _1, _2));
    auto ast = parser.parse();
    if (!ast.has_value()) { exit(1); }
    compiler::Compiler comp(ast.value(), code);
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
    auto ast = parser.parse();
    if (!ast.has_value()) { exit(1); }
    compiler::Compiler comp(ast.value(), code);
    compiler::Disassembler disassembler;
    std::cout << disassembler.disassemble(comp.compile()->codeObject);
}
#endif

periwinkle::Periwinkle::Periwinkle(std::string code)
{
    //utils::replaceTabToSpace(code);
    this->code = code;
};

void periwinkle::throwSyntaxError(const std::string& code, std::string message, size_t position)
{
    auto lineno = utils::linenoFromPosition(code, position);
    auto positionInLine = utils::positionInLineFromPosition(code, position);
    throwSyntaxError(code, message, lineno, positionInLine);
}

void periwinkle::throwSyntaxError(const std::string& code, std::string message, size_t lineno, size_t col)
{
    std::cerr << "Синтаксична помилка: ";
    std::cerr << message << " (знайнено на " << lineno << " рядку)\n";
    const auto& line = utils::getLineFromString(code, lineno);
    std::cerr << utils::indent(4) << line << std::endl;
    auto offset = utils::utf8Size(line.substr(0, col));
    std::cerr << utils::indent(4 + offset) << "^\n";
}
