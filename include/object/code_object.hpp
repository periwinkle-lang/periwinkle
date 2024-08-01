#ifndef CODE_OBJECT_H
#define CODE_OBJECT_H

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>

#include "object.hpp"
#include "vm.hpp"
#include "program_source.hpp"

namespace vm
{
    extern TypeObject codeObjectType;

    struct ExceptionHandler
    {
        Object** stackTop; // Для відновлення стеку опкод TRY записує його вершину
        WORD startAddress; // Адрес опкоду TRY
        WORD firstHandlerAddress; // Адрес першого обробника
        WORD endAddress; // Адрес END_TRY
        WORD finallyAddress; // Адрес початку блоку "наприкінці", якщо 0, то блок відсутній
    };

    struct CodeObject : Object
    {
        std::string name;
        WORD arity=0; // Не враховує варіативний аргумент
        bool isVariadic=false;
        periwinkle::ProgramSource* source;
        std::vector<WORD> code;
        std::vector<Object*> constants;
        std::vector<std::string> names; // Імена змінних
        std::vector<std::string> locals;
        std::vector<std::string> cells;
        std::vector<std::string> freevars;
        std::vector<std::string> argsAsCells; // Імена аргументів, які є комірками
        std::vector<std::string> defaults; // Імена параметрів за замовчуванням
        // Ключ - номер опкода, значення - номер лінії в коді
        std::map<WORD, WORD> ipToLineno;
        std::vector<ExceptionHandler> exceptionHandlers;

        std::optional<ExceptionHandler*> getExceptionHandler(WORD ip);
        ExceptionHandler* getHandlerByStartIp(WORD ip);
        ExceptionHandler* getHandlerByEndIp(WORD ip);

        static CodeObject* create(std::string name);
    };
}

#endif
