#ifndef GC_HPP
#define GC_HPP

#include <forward_list>

#include "vm.hpp"

constexpr const i32 GC_THRESHOLD = 16384; // В байтах

namespace vm
{
    class GC
    {
    private:
        std::forward_list<Object*> objects;
        u64 allocated = 0; // Розмір виділеної пам'яті в байтах

        // Поріг, після якого запускається очищення пам'яті.
        // Початковий поріг виставлений в 4 кібібайти.
        u64 threshold = 4096;

        void mark(Frame* frame);
        void sweep();
    public:
        // Приймає поточний фрейм
        void gc(Frame* frame);
        void addObject(Object* o);

        // Видялає всі об'єкти
        void clean();

        GC();
    };
}

#endif
