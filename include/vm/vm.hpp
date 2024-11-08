#ifndef VM_H
#define VM_H

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <iostream>

#include "object.hpp"
#include "types.hpp"
#include "string_enum.hpp"
#include "exception_object.hpp"

namespace vm
{
    STRING_ENUM(OpCode,
        // Операції зі стеком
        POP, DUP,

        // Операції над об'єктами
        UNARY_OP, BINARY_OP,

        IS, COMPARE,

        // Логічні операції
        NOT,

        // Операції контролю потоку виконання
        JMP, JMP_IF_TRUE, JMP_IF_FALSE, JMP_IF_TRUE_OR_POP, JMP_IF_FALSE_OR_POP,
        CALL, CALL_NA, RETURN, FOR_EACH,

        // Операції для роботи з пам'яттю
        LOAD_CONST,
        LOAD_GLOBAL, STORE_GLOBAL, DELETE_GLOBAL,
        LOAD_LOCAL, STORE_LOCAL, DELETE_LOCAL,
        GET_CELL, LOAD_CELL, STORE_CELL, GET_ATTR,

        LOAD_METHOD, CALL_METHOD, CALL_METHOD_NA,

        MAKE_FUNCTION,
        TRY, CATCH, END_TRY, RAISE,
        COUNT // Кількість операцій
    )

    constexpr const WORD OPCODE_MASK = 0xff;

    struct CodeObject;

    struct Frame
    {
        Frame* previous = nullptr;
        CodeObject* codeObject;
        using object_map_t = std::unordered_map<std::string, Object*>;
        object_map_t* globals; // Глобальні змінні

        Object** sp; // stack pointer. Посилається на вершину стека
        Object** bp; // base pointer. Посилається на початок стека для даного фрейма

        // Посилається на стек після локальних змінних, те саме що й
        //  bp + кількість_локальних_змінних.
        //  Спочатку йдуть комірки, потім вільні змінні
        Object** freevars;

        ~Frame()
        {
            delete globals;
        };
    };

    class VirtualMachine
    {
    private:
        Frame* frame;
        WORD* ip;
        Object**& sp;
        Object**& bp;
        Object**& freevars;

        i64 getLineno(WORD* ip) const;
    public:
        Object* execute();
        Frame* getFrame() const;

        static VirtualMachine* currentVm;
        VirtualMachine(Frame* frame);
    };
}
#endif
