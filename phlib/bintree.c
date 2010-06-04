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

VOID PhInitializeBinaryTree(
    __out PPH_BINARY_TREE Tree,
    __in PPH_BINARY_TREE_COMPARE_FUNCTION CompareFunction,
    __in PVOID Context
    )
{
    Tree->Root.Parent = NULL;
    Tree->Root.Left = NULL;
    Tree->Root.Right = NULL;
    Tree->CompareFunction = CompareFunction;
    Tree->Context = Context;
}

FORCEINLINE PPH_BINARY_LINKS PhpSearchBinaryTree(
    __in PPH_BINARY_TREE Tree,
    __in PPH_BINARY_LINKS Subject,
    __out PINT Result
    )
{
    PPH_BINARY_LINKS links;
    INT result;

    links = Tree->Root.Right;

    if (!links)
    {
        *Result = 1;

        return &Tree->Root;
    }

    while (TRUE)
    {
        result = Tree->CompareFunction(Tree, links, Subject);

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

PPH_BINARY_LINKS PhBinaryTreeAdd(
    __inout PPH_BINARY_TREE Tree,
    __out PPH_BINARY_LINKS Subject
    )
{
    PPH_BINARY_LINKS links;
    INT result;

    links = PhpSearchBinaryTree(Tree, Subject, &result);

    if (result < 0)
        links->Left = Subject;
    else if (result > 0)
        links->Right = Subject;
    else
        return links;

    Subject->Parent = links;
    Subject->Left = NULL;
    Subject->Right = NULL;

    return NULL;
}

BOOLEAN PhBinaryTreeRemove(
    __inout PPH_BINARY_TREE Tree,
    __inout PPH_BINARY_LINKS Subject
    )
{
    PPH_BINARY_LINKS *replace;
    PPH_BINARY_LINKS child;

    if (Subject->Parent->Left == Subject)
    {
        replace = &Subject->Parent->Left;
    }
    else if (Subject->Parent->Right == Subject)
    {
        replace = &Subject->Parent->Right;
    }
    else
    {
        assert(FALSE);
    }

    if (!Subject->Left && !Subject->Right)
    {
        *replace = NULL;
    }
    else if (!Subject->Left)
    {
        *replace = Subject->Right;
    }
    else if (!Subject->Right)
    {
        *replace = Subject->Left;
    }
    else
    {
        LONG useLeft;

        // We replace the node with either its in-order successor or 
        // its in-order predecessor. This is done by copying the links.
        if (useLeft & 0x9) // this may give us slightly random values, which is what we want
        {
            child = Subject->Left;

            while (child->Right)
                child = child->Right;
        }
        else
        {
            child = Subject->Right;

            while (child->Left)
                child = child->Left;
        }

        child->Parent = Subject->Parent;
        child->Left = Subject->Left;
        child->Right = Subject->Right;
        *replace = child;
    }

    return TRUE;
}

PPH_BINARY_LINKS PhBinaryTreeSearch(
    __in PPH_BINARY_TREE Tree,
    __in PPH_BINARY_LINKS Subject
    )
{
    PPH_BINARY_LINKS links;
    INT result;

    links = PhpSearchBinaryTree(Tree, Subject, &result);

    if (result == 0)
        return links;
    else
        return NULL;
}

VOID PhInitializeAvlTree(
    __out PPH_AVL_TREE Tree,
    __in PPH_AVL_TREE_COMPARE_FUNCTION CompareFunction,
    __in PVOID Context
    )
{
    Tree->Root.Parent = NULL;
    Tree->Root.Left = NULL;
    Tree->Root.Right = NULL;
    Tree->Root.Balance = 0;
    Tree->CompareFunction = CompareFunction;
    Tree->Context = Context;
}

FORCEINLINE PPH_AVL_LINKS PhpSearchAvlTree(
    __in PPH_AVL_TREE Tree,
    __in PPH_AVL_LINKS Subject,
    __out PINT Result
    )
{
    PPH_AVL_LINKS links;
    INT result;

    links = Tree->Root.Right;

    if (!links)
    {
        *Result = 1;

        return &Tree->Root;
    }

    while (TRUE)
    {
        result = Tree->CompareFunction(Tree, links, Subject);

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
    __deref_out PPH_AVL_LINKS *Root
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

FORCEINLINE VOID PhpRotateRightAvlLinks(
    __deref_out PPH_AVL_LINKS *Root
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

    // Q.Left = P->Right (transfer B)

    Q->Left = P->Right;

    if (Q->Left)
        Q->Left->Parent = Q;

    // P.Right = Q

    P->Right = Q;
    Q->Parent = P;
}

PPH_AVL_LINKS PhAvlTreeAdd(
    __inout PPH_AVL_TREE Tree,
    __out PPH_AVL_LINKS Subject
    )
{
    PPH_AVL_LINKS links;
    INT result;
    LONG balance;

    links = PhpSearchAvlTree(Tree, Subject, &result);

    if (result < 0)
        links->Left = Subject;
    else if (result > 0)
        links->Right = Subject;
    else
        return links;

    Subject->Parent = links;
    Subject->Left = NULL;
    Subject->Right = NULL;
    Subject->Balance = 0;

    // Balance the tree.

    links = Subject;

    while (TRUE)
    {
        if (links->Parent->Left == links)
            balance = -1;
        else
            balance = 1;

        links = links->Parent;

        if (links == &Tree->Root)
            break;

        if (links->Balance == 0)
        {
            // The balance becomes -1 or 1. Rotations are not needed 
            // yet, but we should keep tracing upwards.

            links->Balance = balance;
        }
        else if (links->Balance != balance)
        {
            // The balance is opposite the new balance, so it now 
            // becomes 0.

            links->Balance = 0;

            break;
        }
        else
        {
            // The balance is the same as the new balance, meaning 
            // it now becomes -2 or 2. Rotations are needed.



            break;
        }
    }

    return NULL;
}

BOOLEAN PhAvlTreeRemove(
    __inout PPH_AVL_TREE Tree,
    __inout PPH_AVL_LINKS Subject
    )
{
    return TRUE;
}

PPH_AVL_LINKS PhAvlTreeSearch(
    __in PPH_AVL_TREE Tree,
    __in PPH_AVL_LINKS Subject
    )
{
    PPH_AVL_LINKS links;
    INT result;

    links = PhpSearchAvlTree(Tree, Subject, &result);

    if (result == 0)
        return links;
    else
        return NULL;
}
