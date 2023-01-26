#ifndef NODE_H
#define NODE_H

#include "node_kind.h"

namespace parser
{
    enum class NodeKind;

    #define NODE_KIND(NAME) virtual NodeKind kind() final { return NodeKind::NAME; }

    struct Node
    {
        virtual NodeKind kind() = 0;
    };
}

#endif
