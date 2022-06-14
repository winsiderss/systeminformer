/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#include <phbase.h>

/**
 * Initializes an AVL tree.
 *
 * \param Tree The tree.
 * \param CompareFunction A function used to compare tree elements.
 */
VOID PhInitializeAvlTree(
    _Out_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_TREE_COMPARE_FUNCTION CompareFunction
    )
{
    Tree->Root.Parent = NULL;
    Tree->Root.Left = NULL;
    Tree->Root.Right = NULL;
    Tree->Root.Balance = 0;
    Tree->Count = 0;

    Tree->CompareFunction = CompareFunction;
}

/**
 * Finds an element in an AVL tree.
 *
 * \param Tree The tree.
 * \param Element The element to find.
 * \param Result The result of the search.
 */
FORCEINLINE PPH_AVL_LINKS PhpFindElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element,
    _Out_ PLONG Result
    )
{
    PPH_AVL_LINKS links;
    LONG result;

    links = PhRootElementAvlTree(Tree);

    if (!links)
    {
        *Result = 1;
        return &Tree->Root;
    }

    while (TRUE)
    {
        result = Tree->CompareFunction(Element, links);

        if (result == 0)
        {
            *Result = 0;
            return links;
        }
        else if (result < 0)
        {
            if (links->Left)
            {
                links = links->Left;
            }
            else
            {
                *Result = -1;
                return links;
            }
        }
        else
        {
            if (links->Right)
            {
                links = links->Right;
            }
            else
            {
                *Result = 1;
                return links;
            }
        }
    }
}

FORCEINLINE VOID PhpRotateLeftAvlLinks(
    _Inout_ PPH_AVL_LINKS *Root
    )
{
    PPH_AVL_LINKS P;
    PPH_AVL_LINKS Q;

    //    P
    //  |   |
    //  A   Q
    //     | |
    //     B C
    //
    // becomes
    //
    //    Q
    //  |   |
    //  P   C
    // | |
    // A B
    //
    // P and Q must exist.
    // B may not exist.
    // A and C are not affected.

    P = *Root;
    Q = P->Right;

    // The new root is Q

    *Root = Q;
    Q->Parent = P->Parent;

    // P.Right = Q.Left (transfer B)

    P->Right = Q->Left;

    if (P->Right)
        P->Right->Parent = P;

    // Q.Left = P

    Q->Left = P;
    P->Parent = Q;
}

FORCEINLINE VOID PhpRotateLeftTwiceAvlLinks(
    _Inout_ PPH_AVL_LINKS *Root
    )
{
    PPH_AVL_LINKS P;
    PPH_AVL_LINKS Q;
    PPH_AVL_LINKS R;

    //     P
    //  |     |
    //  A     Q
    //      |   |
    //      R   D
    //     | |
    //     B C
    //
    // becomes
    //
    //     R
    //  |     |
    //  P     Q
    // | |   | |
    // A B   C D
    //
    // P, Q, and R must exist.
    // B and C may not exist.
    // A and D are not affected.

    // PhpRotateRightAvlLinks(&(*Root)->Right);
    // PhpRotateLeftAvlLinks(Root);

    // P is the current root
    // Q is P.Right
    // R is Q.Left (P.Right.Left)

    P = *Root;
    Q = P->Right;
    R = Q->Left;

    // The new root is R

    *Root = R;
    R->Parent = P->Parent;

    // Q.Left = R.Right (transfer C)

    Q->Left = R->Right;

    if (Q->Left)
        Q->Left->Parent = Q;

    // R.Right = Q

    R->Right = Q;
    Q->Parent = R;

    // P.Right = R.Left (transfer B)

    P->Right = R->Left;

    if (P->Right)
        P->Right->Parent = P;

    // R.Left = P

    R->Left = P;
    P->Parent = R;
}

FORCEINLINE VOID PhpRotateRightAvlLinks(
    _Inout_ PPH_AVL_LINKS *Root
    )
{
    PPH_AVL_LINKS Q;
    PPH_AVL_LINKS P;

    //    Q
    //  |   |
    //  P   C
    // | |
    // A B
    //
    // becomes
    //
    //    P
    //  |   |
    //  A   Q
    //     | |
    //     B C
    //
    // Q and P must exist.
    // B may not exist.
    // A and C are not affected.

    Q = *Root;
    P = Q->Left;

    // The new root is P

    *Root = P;
    P->Parent = Q->Parent;

    // Q.Left = P.Right (transfer B)

    Q->Left = P->Right;

    if (Q->Left)
        Q->Left->Parent = Q;

    // P.Right = Q

    P->Right = Q;
    Q->Parent = P;
}

FORCEINLINE VOID PhpRotateRightTwiceAvlLinks(
    _Inout_ PPH_AVL_LINKS *Root
    )
{
    PPH_AVL_LINKS P;
    PPH_AVL_LINKS Q;
    PPH_AVL_LINKS R;

    //       P
    //    |     |
    //    Q     D
    //  |   |
    //  A   R
    //     | |
    //     B C
    //
    // becomes
    //
    //     R
    //  |     |
    //  Q     P
    // | |   | |
    // A B   C D
    //
    // P, Q, and R must exist.
    // B and C may not exist.
    // A and D are not affected.

    // PhpRotateLeftAvlLinks(&(*Root)->Left);
    // PhpRotateRightAvlLinks(Root);

    // P is the current root
    // Q is P.Left
    // R is Q.Right (P.Left.Right)

    P = *Root;
    Q = P->Left;
    R = Q->Right;

    // The new root is R

    *Root = R;
    R->Parent = P->Parent;

    // Q.Right = R.Left (transfer B)

    Q->Right = R->Left;

    if (Q->Right)
        Q->Right->Parent = Q;

    // R.Left = Q

    R->Left = Q;
    Q->Parent = R;

    // P.Left = R.Right (transfer C)

    P->Left = R->Right;

    if (P->Left)
        P->Left->Parent = P;

    // R.Right = P

    R->Right = P;
    P->Parent = R;
}

ULONG PhpRebalanceAvlLinks(
    _Inout_ PPH_AVL_LINKS *Root
    )
{
    PPH_AVL_LINKS P;
    PPH_AVL_LINKS Q;
    PPH_AVL_LINKS R;

    P = *Root;

    if (P->Balance == -1)
    {
        Q = P->Left;

        if (Q->Balance == -1)
        {
            // Left-left

            PhpRotateRightAvlLinks(Root);

            P->Balance = 0;
            Q->Balance = 0;

            return 1;
        }
        else if (Q->Balance == 1)
        {
            // Left-right

            R = Q->Right;

            PhpRotateRightTwiceAvlLinks(Root);

            if (R->Balance == -1)
            {
                P->Balance = 1;
                Q->Balance = 0;
            }
            else if (R->Balance == 1)
            {
                P->Balance = 0;
                Q->Balance = -1;
            }
            else
            {
                P->Balance = 0;
                Q->Balance = 0;
            }

            R->Balance = 0;

            return 2;
        }
        else
        {
            // Special (only occurs when removing)

            //    D
            //  |   |
            //  B   E
            // | |
            // A C
            //
            // Removing E results in:
            //
            //    D
            //  |
            //  B
            // | |
            // A C
            //
            // which is unbalanced. Rotating right at B results in:
            //
            //   B
            // |   |
            // A   D
            //    |
            //    C
            //
            // The same applies for the mirror case.

            PhpRotateRightAvlLinks(Root);

            Q->Balance = 1;

            return 3;
        }
    }
    else
    {
        Q = P->Right;

        if (Q->Balance == 1)
        {
            // Right-right

            PhpRotateLeftAvlLinks(Root);

            P->Balance = 0;
            Q->Balance = 0;

            return 1;
        }
        else if (Q->Balance == -1)
        {
            // Right-left

            R = Q->Left;

            PhpRotateLeftTwiceAvlLinks(Root);

            if (R->Balance == -1)
            {
                P->Balance = 0;
                Q->Balance = 1;
            }
            else if (R->Balance == 1)
            {
                P->Balance = -1;
                Q->Balance = 0;
            }
            else
            {
                P->Balance = 0;
                Q->Balance = 0;
            }

            R->Balance = 0;

            return 2;
        }
        else
        {
            // Special (only occurs when removing)

            PhpRotateLeftAvlLinks(Root);

            Q->Balance = -1;

            return 3;
        }
    }
}

/**
 * Adds an element to an AVL tree.
 *
 * \param Tree The tree.
 * \param Element The element to add.
 *
 * \return NULL if the element was added, or an existing element.
 */
PPH_AVL_LINKS PhAddElementAvlTree(
    _Inout_ PPH_AVL_TREE Tree,
    _Out_ PPH_AVL_LINKS Element
    )
{
    LONG result;
    PPH_AVL_LINKS P;
    PPH_AVL_LINKS root;
    LONG balance;

    P = PhpFindElementAvlTree(Tree, Element, &result);

    if (result < 0)
        P->Left = Element;
    else if (result > 0)
        P->Right = Element;
    else
        return P;

    Element->Parent = P;
    Element->Left = NULL;
    Element->Right = NULL;
    Element->Balance = 0;

    // Balance the tree.

    P = Element;
    root = PhRootElementAvlTree(Tree);

    while (P != root)
    {
        // In this implementation, the balance factor is the right height minus left height.

        if (P->Parent->Left == P)
            balance = -1;
        else
            balance = 1;

        P = P->Parent;

        if (P->Balance == 0)
        {
            // The balance becomes -1 or 1. Rotations are not needed
            // yet, but we should keep tracing upwards.

            P->Balance = balance;
        }
        else if (P->Balance != balance)
        {
            // The balance is opposite the new balance, so it now
            // becomes 0.

            P->Balance = 0;

            break;
        }
        else
        {
            PPH_AVL_LINKS *ref;

            // The balance is the same as the new balance, meaning
            // it now becomes -2 or 2. Rotations are needed.

            if (P->Parent->Left == P)
                ref = &P->Parent->Left;
            else
                ref = &P->Parent->Right;

            PhpRebalanceAvlLinks(ref);

            break;
        }
    }

    Tree->Count++;

    return NULL;
}

/**
 * Removes an element from an AVL tree.
 *
 * \param Tree The tree.
 * \param Element An element already present in the tree.
 */
VOID PhRemoveElementAvlTree(
    _Inout_ PPH_AVL_TREE Tree,
    _Inout_ PPH_AVL_LINKS Element
    )
{
    PPH_AVL_LINKS newElement;
    PPH_AVL_LINKS *replace;
    PPH_AVL_LINKS P;
    PPH_AVL_LINKS root;
    LONG balance;

    if (!Element->Left || !Element->Right)
    {
        newElement = Element;
    }
    else if (Element->Balance >= 0) // Pick the side depending on the balance to minimize rebalances
    {
        newElement = Element->Right;

        while (newElement->Left)
            newElement = newElement->Left;
    }
    else
    {
        newElement = Element->Left;

        while (newElement->Right)
            newElement = newElement->Right;
    }

    if (newElement->Parent->Left == newElement)
    {
        replace = &newElement->Parent->Left;
        balance = -1;
    }
    else
    {
        replace = &newElement->Parent->Right;
        balance = 1;
    }

    if (!newElement->Right)
    {
        *replace = newElement->Left;

        if (newElement->Left)
            newElement->Left->Parent = newElement->Parent;
    }
    else
    {
        *replace = newElement->Right;
        newElement->Right->Parent = newElement->Parent; // we know Right exists
    }

    P = newElement->Parent;
    root = &Tree->Root;

    while (P != root)
    {
        if (P->Balance == balance)
        {
            // The balance is cancelled by the remove operation and becomes 0.
            // Rotations are not needed yet, but we should keep tracing upwards.

            P->Balance = 0;
        }
        else if (P->Balance == 0)
        {
            // The balance is 0, so it now becomes -1 or 1.

            P->Balance = -balance;

            break;
        }
        else
        {
            PPH_AVL_LINKS *ref;

            // The balance is the same as the new balance, meaning
            // it now becomes -2 or 2. Rotations are needed.

            if (P->Parent->Left == P)
                ref = &P->Parent->Left;
            else
                ref = &P->Parent->Right;

            // We can stop tracing if we have a special case rotation.
            if (PhpRebalanceAvlLinks(ref) == 3)
                break;

            P = P->Parent;
        }

        if (P->Parent->Left == P)
            balance = -1;
        else
            balance = 1;

        P = P->Parent;
    }

    if (newElement != Element)
    {
        // Replace the subject with the new subject.

        *newElement = *Element;

        if (Element->Parent->Left == Element)
            newElement->Parent->Left = newElement;
        else
            newElement->Parent->Right = newElement;

        if (newElement->Left)
            newElement->Left->Parent = newElement;
        if (newElement->Right)
            newElement->Right->Parent = newElement;
    }

    Tree->Count--;
}

/**
 * Finds an element in an AVL tree.
 *
 * \param Tree The tree.
 * \param Element An element to find.
 *
 * \return The element, or NULL if it could not be found.
 */
PPH_AVL_LINKS PhFindElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element
    )
{
    PPH_AVL_LINKS links;
    LONG result;

    links = PhpFindElementAvlTree(Tree, Element, &result);

    if (result == 0)
        return links;
    else
        return NULL;
}

/**
 * Finds the first element in an AVL tree that is greater than or equal to the specified element.
 *
 * \param Tree The tree.
 * \param Element The element to find.
 *
 * \return The bound element, or NULL if the tree is empty.
 */
PPH_AVL_LINKS PhLowerBoundElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element
    )
{
    PPH_AVL_LINKS links;
    PPH_AVL_LINKS closest;
    LONG result;

    links = PhRootElementAvlTree(Tree);
    closest = NULL;

    while (links)
    {
        result = Tree->CompareFunction(Element, links);

        if (result > 0)
        {
            links = links->Right;
        }
        else
        {
            closest = links;
            links = links->Left;
        }
    }

    return closest;
}

/**
 * Finds the first element in an AVL tree that is greater than the specified element.
 *
 * \param Tree The tree.
 * \param Element The element to find.
 *
 * \return The bound element, or NULL if the tree is empty.
 */
PPH_AVL_LINKS PhUpperBoundElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element
    )
{
    PPH_AVL_LINKS links;
    PPH_AVL_LINKS closest;
    LONG result;

    links = PhRootElementAvlTree(Tree);
    closest = NULL;

    while (links)
    {
        result = Tree->CompareFunction(Element, links);

        if (result >= 0)
        {
            links = links->Right;
        }
        else
        {
            closest = links;
            links = links->Left;
        }
    }

    return closest;
}

/**
 * Finds the last element in an AVL tree that is less than the specified element.
 *
 * \param Tree The tree.
 * \param Element The element to find.
 *
 * \return The bound element, or NULL if the tree is empty.
 */
PPH_AVL_LINKS PhLowerDualBoundElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element
    )
{
    PPH_AVL_LINKS links;
    PPH_AVL_LINKS closest;
    LONG result;

    links = PhRootElementAvlTree(Tree);
    closest = NULL;

    while (links)
    {
        result = Tree->CompareFunction(Element, links);

        if (result > 0)
        {
            closest = links;
            links = links->Right;
        }
        else
        {
            links = links->Left;
        }
    }

    return closest;
}

/**
 * Finds the last element in an AVL tree that is less than or equal to the specified element.
 *
 * \param Tree The tree.
 * \param Element The element to find.
 *
 * \return The bound element, or NULL if the tree is empty.
 */
PPH_AVL_LINKS PhUpperDualBoundElementAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PPH_AVL_LINKS Element
    )
{
    PPH_AVL_LINKS links;
    PPH_AVL_LINKS closest;
    LONG result;

    links = PhRootElementAvlTree(Tree);
    closest = NULL;

    while (links)
    {
        result = Tree->CompareFunction(Element, links);

        if (result >= 0)
        {
            closest = links;
            links = links->Right;
        }
        else
        {
            links = links->Left;
        }
    }

    return closest;
}

/**
 * Finds the smallest element in an AVL tree.
 *
 * \param Tree The tree.
 *
 * \return An element, or NULL if the tree is empty.
 */
PPH_AVL_LINKS PhMinimumElementAvlTree(
    _In_ PPH_AVL_TREE Tree
    )
{
    PPH_AVL_LINKS links;

    links = PhRootElementAvlTree(Tree);

    if (!links)
        return NULL;

    while (links->Left)
        links = links->Left;

    return links;
}

/**
 * Finds the biggest element in an AVL tree.
 *
 * \param Tree The tree.
 *
 * \return An element, or NULL if the tree is empty.
 */
PPH_AVL_LINKS PhMaximumElementAvlTree(
    _In_ PPH_AVL_TREE Tree
    )
{
    PPH_AVL_LINKS links;

    links = PhRootElementAvlTree(Tree);

    if (!links)
        return NULL;

    while (links->Right)
        links = links->Right;

    return links;
}

/**
 * Finds the next element in an AVL tree.
 *
 * \param Element The element.
 *
 * \return The next element, or NULL if there are no more elements.
 */
PPH_AVL_LINKS PhSuccessorElementAvlTree(
    _In_ PPH_AVL_LINKS Element
    )
{
    PPH_AVL_LINKS links;

    if (Element->Right)
    {
        Element = Element->Right;

        while (Element->Left)
            Element = Element->Left;

        return Element;
    }
    else
    {
        // Trace back to the next vertical level. Note
        // that this code does in fact return NULL when there
        // are no more elements because of the way the root
        // element is constructed.

        links = Element->Parent;

        while (links && links->Right == Element)
        {
            Element = links;
            links = links->Parent;
        }

        return links;
    }
}

/**
 * Finds the previous element in an AVL tree.
 *
 * \param Element The element.
 *
 * \return The previous element, or NULL if there are no more elements.
 */
PPH_AVL_LINKS PhPredecessorElementAvlTree(
    _In_ PPH_AVL_LINKS Element
    )
{
    PPH_AVL_LINKS links;

    if (Element->Left)
    {
        Element = Element->Left;

        while (Element->Right)
            Element = Element->Right;

        return Element;
    }
    else
    {
        links = Element->Parent;

        while (links && links->Left == Element)
        {
            Element = links;
            links = links->Parent;
        }

        if (links)
        {
            // We need an additional check because the tree root is
            // stored in Root.Right, not Left.
            if (!links->Parent)
                return NULL; // reached Root, so no more elements
        }

        return links;
    }
}

/**
 * Enumerates the elements in an AVL tree.
 *
 * \param Tree The tree.
 * \param Order The enumeration order.
 * \param Callback The callback function.
 * \param Context A user-defined value to pass to the callback function.
 */
VOID PhEnumAvlTree(
    _In_ PPH_AVL_TREE Tree,
    _In_ PH_TREE_ENUMERATION_ORDER Order,
    _In_ PPH_ENUM_AVL_TREE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    // The maximum height of an AVL tree is around 1.44 * log2(n).
    // The maximum number of elements in this implementation is 2^32, so the maximum height is
    // around 46.08.
    PPH_AVL_LINKS stackBase[47];
    PPH_AVL_LINKS *stack;
    PPH_AVL_LINKS links;

    stack = stackBase;

    switch (Order)
    {
    case TreeEnumerateInOrder:
        links = PhRootElementAvlTree(Tree);

        while (links)
        {
            *stack++ = links;
            links = links->Left;
        }

        while (stack != stackBase)
        {
            links = *--stack;

            if (!Callback(Tree, links, Context))
                break;

            links = links->Right;

            while (links)
            {
                *stack++ = links;
                links = links->Left;
            }
        }

        break;
    case TreeEnumerateInReverseOrder:
        links = PhRootElementAvlTree(Tree);

        while (links)
        {
            *stack++ = links;
            links = links->Right;
        }

        while (stack != stackBase)
        {
            links = *--stack;

            if (!Callback(Tree, links, Context))
                break;

            links = links->Left;

            while (links)
            {
                *stack++ = links;
                links = links->Right;
            }
        }

        break;
    }
}
