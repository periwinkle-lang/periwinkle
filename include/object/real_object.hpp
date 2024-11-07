#ifndef REAL_OBJECT_H
#define REAL_OBJECT_H

#include "object.hpp"

namespace vm
{
    extern TypeObject realObjectType;

    struct RealObject : Object
    {
        double value;

        static RealObject* create(double value);
    };

    extern RealObject P_realNan, P_realInf;
}

#endif
