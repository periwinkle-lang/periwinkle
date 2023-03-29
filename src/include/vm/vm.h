#ifndef VM_H
#define VM_H

#include <algorithm>
#include <vector>
#include <unordered_map>
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

        // Операції над об'єктами
        POS, NEG, GET_ITER, ADD, SUB, MUL, DIV, MOD, FLOOR_DIV,

        // Бінарні операції
        BIN_AND, BIN_OR, BIN_NOT, XOR, SHL, SHR, IS, COMPARE,

        // Логічні операції
        AND, OR, NOT,

        // Операції контролю потоку виконання
        JMP, JMP_IF_TRUE, JMP_IF_FALSE, CALL, RETURN, FOR_EACH,

        // Операції для роботи з пам'яттю
        LOAD_CONST, LOAD_GLOBAL, STORE_GLOBAL, LOAD_LOCAL, STORE_LOCAL,
        GET_CELL, LOAD_CELL, STORE_CELL, GET_ATTR,

        LOAD_METHOD, CALL_METHOD,

        MAKE_FUNCTION,

        HALT, // Завершення роботи віртуальної машини
        COUNT // Кількість операцій
    )

    struct CodeObject;

    struct Frame
    {
        Frame* previous;
        CodeObject* codeObject;
        using object_map_t = std::unordered_map<std::string, Object*>;
        object_map_t* globals; // Глобальні змінні

        Object** sp; // stack pointer. Посилається на вершину стека
        Object** bp; // base pointer. Посилається на початок стека для даного фрейма

        // Посилається на стек після локальних змінних, те саме що й
        //  bp + кількість_локальних_змінних.
        //  Спочатку йдуть комірки, потім вільні змінні
        Object** freevars;
    };

    class VirtualMachine
    {
    private:
        Frame* frame;
        WORD* ip;
        Object**& sp;
        Object**& bp;
        Object**& freevars;

    public:
        void throwException(TypeObject* exception, std::string message, WORD lineno=0);
        Object* execute();
        Frame* getFrame() const;

        static VirtualMachine* currentVm;
        VirtualMachine(Frame* frame);
    };
}
#endif
