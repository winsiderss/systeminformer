/*
 * Process Hacker - 
 *   binary trees
 * 
 * Copyright (C) 2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phbase.h>

VOID PhInitializeAvlTree(
    __out PPH_AVL_TREE Tree,
    __in PPH_AVL_TREE_COMPARE_FUNCTION CompareFunction
    )
{
    Tree->Root.Parent = NULL;
    Tree->Root.Left = NULL;
    Tree->Root.Right = NULL;
    Tree->Root.Balance = 0;
    Tree->Count = 0;

    Tree->CompareFunction = CompareFunction;
}

FORCEINLINE PPH_AVL_LINKS PhpFindElementAvlTree(
    __in PPH_AVL_TREE Tree,
    __in PPH_AVL_LINKS Element,
    __out PINT Result
    )
{
    PPH_AVL_LINKS links;
    INT result;

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

    assert(FALSE);
}

FORCEINLINE VOID PhpRotateLeftAvlLinks(
    __deref_inout PPH_AVL_LINKS *Root
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
    __deref_inout PPH_AVL_LINKS *Root
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
    __deref_inout PPH_AVL_LINKS *Root
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
    __deref_inout PPH_AVL_LINKS *Root
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
    __deref_inout PPH_AVL_LINKS *Root
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
        PPH_AVL_LINKS Q;

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

PPH_AVL_LINKS PhAddElementAvlTree(
    __inout PPH_AVL_TREE Tree,
    __out PPH_AVL_LINKS Element
    )
{
    INT result;
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

VOID PhRemoveElementAvlTree(
    __inout PPH_AVL_TREE Tree,
    __inout PPH_AVL_LINKS Element
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
    else if (Element->Balance >= 0) // pick the side depending on the balance to minimize rebalances
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

PPH_AVL_LINKS PhFindElementAvlTree(
    __in PPH_AVL_TREE Tree,
    __in PPH_AVL_LINKS Element
    )
{
    PPH_AVL_LINKS links;
    INT result;

    links = PhpFindElementAvlTree(Tree, Element, &result);

    if (result == 0)
        return links;
    else
        return NULL;
}

PPH_AVL_LINKS PhMinimumElementAvlTree(
    __in PPH_AVL_TREE Tree
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

PPH_AVL_LINKS PhMaximumElementAvlTree(
    __in PPH_AVL_TREE Tree
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

PPH_AVL_LINKS PhSuccessorElementAvlTree(
    __in PPH_AVL_LINKS Element
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

PPH_AVL_LINKS PhPredecessorElementAvlTree(
    __in PPH_AVL_LINKS Element
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

        // We need an additional check because the tree root is 
        // stored in Root.Right, not Left.
        if (!links->Parent)
            return NULL; // reached Root, so no more elements

        return links;
    }
}

VOID PhEnumAvlTree(
    __in PPH_AVL_TREE Tree,
    __in PH_TREE_ENUMERATION_ORDER Order,
    __in PPH_ENUM_AVL_TREE_CALLBACK Callback,
    __in_opt PVOID Context
    )
{
    // The maximum height of an AVL tree is around 1.44 * log2(n).
    // The maximum number of elements in this implementation is 
    // 2^32, so the maximum height is around 46.08.
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
