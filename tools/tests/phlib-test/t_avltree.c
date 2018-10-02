#include "tests.h"

typedef struct _NODE
{
    PH_AVL_LINKS Links;
    ULONG Value;
} NODE, *PNODE;

static LONG CompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    )
{
    PNODE node1 = CONTAINING_RECORD(Links1, NODE, Links);
    PNODE node2 = CONTAINING_RECORD(Links2, NODE, Links);

    return uintcmp(node1->Value, node2->Value);
}

#define BOUND_FUNCTION(DefineName, FunctionName) \
    static ULONG DefineName( \
        _In_ PPH_AVL_TREE Tree, \
        _In_ ULONG Value \
    ) \
    { \
        NODE node; \
        PPH_AVL_LINKS result; \
\
        node.Value = Value; \
        result = FunctionName(Tree, &node.Links); \
\
        if (result) \
            return CONTAINING_RECORD(result, NODE, Links)->Value; \
        else \
            return -1; \
    }

BOUND_FUNCTION(lbound, PhLowerBoundElementAvlTree);
BOUND_FUNCTION(ubound, PhUpperBoundElementAvlTree);
BOUND_FUNCTION(ldbound, PhLowerDualBoundElementAvlTree);
BOUND_FUNCTION(udbound, PhUpperDualBoundElementAvlTree);

VOID Test_avltree(
    VOID
    )
{
    PH_AVL_TREE tree;
    NODE nodes[4];

    PhInitializeAvlTree(&tree, CompareFunction);

    nodes[0].Value = 3;
    PhAddElementAvlTree(&tree, &nodes[0].Links);
    nodes[1].Value = 1;
    PhAddElementAvlTree(&tree, &nodes[1].Links);
    nodes[2].Value = 5;
    PhAddElementAvlTree(&tree, &nodes[2].Links);
    nodes[3].Value = 7;
    PhAddElementAvlTree(&tree, &nodes[3].Links);

    assert(lbound(&tree, 0) == 1);
    assert(lbound(&tree, 1) == 1);
    assert(lbound(&tree, 2) == 3);
    assert(lbound(&tree, 3) == 3);
    assert(lbound(&tree, 4) == 5);
    assert(lbound(&tree, 5) == 5);
    assert(lbound(&tree, 6) == 7);
    assert(lbound(&tree, 7) == 7);
    assert(lbound(&tree, 8) == -1);

    assert(ubound(&tree, 0) == 1);
    assert(ubound(&tree, 1) == 3);
    assert(ubound(&tree, 2) == 3);
    assert(ubound(&tree, 3) == 5);
    assert(ubound(&tree, 4) == 5);
    assert(ubound(&tree, 5) == 7);
    assert(ubound(&tree, 6) == 7);
    assert(ubound(&tree, 7) == -1);
    assert(ubound(&tree, 8) == -1);

    assert(ldbound(&tree, 0) == -1);
    assert(ldbound(&tree, 1) == -1);
    assert(ldbound(&tree, 2) == 1);
    assert(ldbound(&tree, 3) == 1);
    assert(ldbound(&tree, 4) == 3);
    assert(ldbound(&tree, 5) == 3);
    assert(ldbound(&tree, 6) == 5);
    assert(ldbound(&tree, 7) == 5);
    assert(ldbound(&tree, 8) == 7);

    assert(udbound(&tree, 0) == -1);
    assert(udbound(&tree, 1) == 1);
    assert(udbound(&tree, 2) == 1);
    assert(udbound(&tree, 3) == 3);
    assert(udbound(&tree, 4) == 3);
    assert(udbound(&tree, 5) == 5);
    assert(udbound(&tree, 6) == 5);
    assert(udbound(&tree, 7) == 7);
    assert(udbound(&tree, 8) == 7);
}
