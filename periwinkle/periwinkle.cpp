#include <array>
#include <functional>
#include <format>

#include "periwinkle.hpp"
#include "vm.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "utils.hpp"
#include "pconfig.hpp"
#include "string_object.hpp"

using namespace periwinkle;

static Periwinkle* _currentState = nullptr;

int periwinkle::Periwinkle::getVersionAsInt()
{
    return PERIWINKLE_VERSION_MAJOR * 10000 + PERIWINKLE_VERSION_MINOR * 100 + PERIWINKLE_VERSION_PATCH;
}

std::string periwinkle::Periwinkle::getVersionAsString() { return std::string(PERIWINKLE_VERSION); }
int periwinkle::Periwinkle::majorVersion() { return PERIWINKLE_VERSION_MAJOR; }
int periwinkle::Periwinkle::minorVersion() { return PERIWINKLE_VERSION_MINOR; }
int periwinkle::Periwinkle::patchVersion() { return PERIWINKLE_VERSION_PATCH; }

vm::Object* periwinkle::Periwinkle::execute()
{
    using namespace std::placeholders;
    PParser::Parser parser(source->getText());
    parser.setErrorHandler(std::bind(
        static_cast<void(*)(ProgramSource*, std::string, size_t)>(utils::throwSyntaxError),
            source, _1, _2
        )
    );
    auto ast = parser.parse();
    if (!ast.has_value()) { exit(1); }
    compiler::Compiler comp(ast.value(), source);
    std::array<vm::Object*, 512> stack{};
    auto frame = comp.compile();
    frame->sp = &stack[0];
    frame->bp = &stack[0];
    vm::VirtualMachine virtualMachine(frame);
    auto result = virtualMachine.execute();
    delete frame;
    return result;
}

void periwinkle::Periwinkle::setException(vm::TypeObject* type, const std::string& message)
{
    if (!vm::isException(type))
    {
        setException(&vm::InternalErrorObjectType,
            std::format("Об'єкт типу \"{}\" не є підкласом типу \"Виняток\"", type->name));
        return;
    }
    currentException = vm::ExceptionObject::create(type, message);
}

void periwinkle::Periwinkle::setException(vm::Object* o)
{
    if (!vm::isException(o->objectType))
    {
        setException(&vm::InternalErrorObjectType,
            std::format("Об'єкт типу \"{}\" не є підкласом типу \"Виняток\"", o->objectType->name));
        return;
    }
    currentException = static_cast<vm::ExceptionObject*>(o);
}

vm::ExceptionObject* periwinkle::Periwinkle::exceptionOccurred() const
{
    return currentException;
}

void periwinkle::Periwinkle::exceptionClear()
{
    currentException = nullptr;
}

void periwinkle::Periwinkle::printException() const
{
    std::cerr << currentException->objectType->name << ": " << currentException->message << "\n";
    std::cerr << currentException->formatStackTrace();
}

vm::GC* periwinkle::Periwinkle::getGC()
{
    return gc;
}

#ifdef DEV_TOOLS

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
    _currentState = this;
    gc = new vm::GC();
}

periwinkle::Periwinkle::Periwinkle(const std::filesystem::path& path)
    : source(new ProgramSource(path))
{
    _currentState = this;
    gc = new vm::GC();
}

periwinkle::Periwinkle::Periwinkle(const ProgramSource& source)
    : source(new ProgramSource(source))
{
    _currentState = this;
    gc = new vm::GC();
}

periwinkle::Periwinkle::~Periwinkle()
{
    delete source;
    gc->clean();
    delete gc;
}

Periwinkle* getCurrentState()
{
    return _currentState;
}
