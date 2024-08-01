#include <span>

#include "gc.hpp"
#include "native_method_object.hpp"
#include "builtins.hpp"
#include "plogger.hpp"
#include "periwinkle.hpp"

using namespace vm;

void vm::GC::mark(Frame* frame)
{
    Frame* rootFrame = frame;
    while (rootFrame->previous != nullptr)
    {
        rootFrame = rootFrame->previous;
    }
    // Обхід стеку
    for (auto o : std::span{ rootFrame->bp, frame->sp + 1 })
    {
        vm::mark(o);
    }

    // Обхід глобальних змінних
    Frame* _frame = frame;
    while (_frame != nullptr)
    {
        for (auto it = _frame->globals->cbegin(); it != _frame->globals->cend(); ++it)
        {
            vm::mark(it->second);
        }
        _frame = _frame->previous;
    }

    // Обхід кореневого CodeObject
    vm::mark(reinterpret_cast<Object*>(rootFrame->codeObject));

    // Обхід вбудованих об'єктів
    auto builtins = getBuiltin();
    for (auto it = builtins->begin(); it != builtins->end(); ++it)
    {
        vm::mark(it->second);
    }

    // Клас Periwinkle зберігає посилання на об'єкт помилки
    vm::mark(getCurrentState()->exceptionOccurred());
}

static inline void finalizeAndDeleteObject(Object* o)
{
    if (auto finalizer = o->objectType->destructor)
    {
        // Винятки в фіналізаторах ігноруються
        finalizer->method(o, {}, nullptr, nullptr);
    }
    if (auto dealloc = o->objectType->dealloc)
    {
        dealloc(o);
    }
}

void vm::GC::sweep()
{
    objects.remove_if([this](Object* o) {
        if (o->marked == false)
        {
            allocated -= o->objectType->size;
            finalizeAndDeleteObject(o);
            return true;
        }
        else
        {
            o->marked = false;
            return false;
        }
    });
}

void vm::GC::gc(Frame* frame)
{
    if (allocated > threshold)
    {
        mark(frame);
        sweep();
        threshold = GC_THRESHOLD * (allocated / GC_THRESHOLD + 1);
    }
}

void vm::GC::addObject(Object* o)
{
    plog::passert(o->objectType->size != 0) << "Потрібно вказати в TypeObject поле size";
    allocated += o->objectType->size;
    objects.push_front(o);
}

void vm::GC::clean()
{
    for (auto& object : objects)
    {
        finalizeAndDeleteObject(object);
    };
    objects.clear();
}

vm::GC::GC()
{
}
