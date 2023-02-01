#ifndef VM_H
#define VM_H

#include <array>
#include <algorithm>
#include <vector>
#include <map>
#include <iostream>

#include "object.h"
#include "types.h"
#include "string_enum.h"
#include "exception_object.h"

namespace vm
{
    using WORD = u64;

    STRING_ENUM(OpCode,
        // Операції зі стеком
        POP, DUP,

        // Арифметичні операції
        INC, DEC, NEG, ADD, SUB, MUL, DIV, MOD, FLOOR_DIV,

        // Бінарні операції
        BIN_AND, BIN_OR, BIN_NOT, XOR, SHL, SHR,

        // Логічні операції
        AND, OR, NOT, EQUAL, NOT_EQUAL, GREATER, LESS, GREATER_EQUAL, LESS_EQUAL,

        // Операції контролю потоку виконання
        JMP, JMP_IF_TRUE, JMP_IF_FALSE, CALL, RETURN,

        // Операції для роботи з пам'яттю
        LOAD_CONST, LOAD_GLOBAL, STORE_GLOBAL, LOAD_NAME,

        MAKE_FUNCTION,

        HALT, // Завершення роботи віртуальної машини
        COUNT // Кількість операцій
    )

    struct CodeObject;

    struct Frame
    {
        Frame* previous;
        CodeObject* codeObject;
        std::unordered_map<std::string, Object*> globals; // Глобальні змінні
    };

    class VirtualMachine
    {
    private:
        std::array<Object*, 512> stack;

    public:
        void throwException(ExceptionObject* exception);
        void execute(Frame* frame);
    };
}
#endif
