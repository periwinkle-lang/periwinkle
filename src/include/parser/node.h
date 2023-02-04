#ifndef NODE_H
#define NODE_H

#include "node_kind.h"

namespace parser
{
    enum class NodeKind;

    struct Node
    {
        Node* parent;
        NodeKind kind;

        Node(Node* parent, NodeKind kind) : parent(parent), kind(kind) {};
    };
}

#endif
