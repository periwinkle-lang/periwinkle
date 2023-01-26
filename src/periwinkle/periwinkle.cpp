#include "periwinkle.h"
#include "vm.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "utils.h"
#include "pconfig.h"

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
    vm::VirtualMachine virtualMachine;
    virtualMachine.execute(comp.compile());
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
            << (token.tokenType == lexer::TokenType::STRING ? utils::escapeString(token.text) : token.text)
            << "\"" << std::endl;
    }
}

void periwinkle::Periwinkle::printDisassemble()
{
    lexer::Lexer lex(code);
    parser::Parser parser(lex.tokenize(), code);
    compiler::Compiler comp(parser.parse(), code);
    compiler::Decompiler decompiler(comp.compile()->codeObject);
    std::cout << decompiler.decompile();
}
#endif

periwinkle::Periwinkle::Periwinkle(std::string code)
{
    //utils::replaceTabToSpace(code);
    this->code = code;
};
