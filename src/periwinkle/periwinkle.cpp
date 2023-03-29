#include <array>

#include "periwinkle.h"
#include "vm.h"
#include "lexer.h"
#include "parser.h"
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
    lexer::Lexer lex(code);
    parser::Parser parser(lex.tokenize(), code);
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
#include "decompiler.h"

void periwinkle::Periwinkle::printTokens()
{
    lexer::Lexer lex(code);
    auto tokens = lex.tokenize();
    for (auto& token : tokens)
    {
        std::cout << std::left << std::setw(15) << lexer::stringEnum::enumToString(token.tokenType) << " \""
            << (token.tokenType == lexer::TokenType::STRING
                || token.tokenType == lexer::TokenType::SHEBANG
                ? utils::escapeString(token.text) : token.text)
            << "\"" << std::endl;
    }
}

void periwinkle::Periwinkle::printDisassemble()
{
    lexer::Lexer lex(code);
    parser::Parser parser(lex.tokenize(), code);
    compiler::Compiler comp(parser.parse(), code);
    compiler::Decompiler decompiler;
    std::cout << decompiler.decompile(comp.compile()->codeObject);
}
#endif

periwinkle::Periwinkle::Periwinkle(std::string code)
{
    //utils::replaceTabToSpace(code);
    this->code = code;
};
