#ifndef NODE_H
#define NODE_H

#include "node_kind.h"

namespace parser
{
    enum class NodeKind;

    struct Node
    {
        NodeKind kind;

        Node(NodeKind kind) : kind(kind) {};
    };
}

#endif
