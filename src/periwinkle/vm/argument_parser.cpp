#include "argument_parser.h"

#include "vm.h"
#include "exception_object.h"
#include "utils.h"

using namespace vm;

void vm::ArgParser::parse(
    const std::span<Object*> args)
{
    for (size_t i = 0; i < description.size(); ++i)
    {
        auto& desc = description[i];
        auto arg = args[i];

        if (isInstance(arg, desc.type))
        {
            *((Object**)desc.pointer) = arg;
        }
        else
        {
            VirtualMachine::currentVm->throwException(
                &TypeErrorObjectType,
                utils::format(
                    "Тип аргументу \"%s\" має бути \"%s\", але був переданий об'єкт типу \"%s\"",
                    desc.name.c_str(),
                    desc.type.name.c_str(),
                    arg->objectType->name.c_str())
            );
        }
    }
}
