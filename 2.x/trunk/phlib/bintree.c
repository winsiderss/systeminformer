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
        result = Tree->CompareFunction(Tree, Subject, links);

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

VOID PhBinaryTreeRemove(
    __inout PPH_BINARY_TREE Tree,
    __inout PPH_BINARY_LINKS Subject
    )
{
    PPH_BINARY_LINKS *replace;
    PPH_BINARY_LINKS child;

    if (Subject->Parent->Left == Subject)
        replace = &Subject->Parent->Left;
    else
        replace = &Subject->Parent->Right;

    if (!Subject->Right)
    {
        *replace = Subject->Left;

        if (Subject->Left)
            Subject->Left->Parent = Subject->Parent;
    }
    else if (!Subject->Left)
    {
        *replace = Subject->Right;
        Subject->Right->Parent = Subject->Parent; // we know Right exists
    }
    else
    {
        // We replace the node with either its in-order successor or 
        // its in-order predecessor. This is done by copying the links.
        if ((ULONG_PTR)Subject & 0x6) // this may give us slightly random values, which is what we want
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

        if (child->Left)
            child->Left->Parent = child;
        if (child->Right)
            child->Right->Parent = child;
    }
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
        result = Tree->CompareFunction(Tree, Subject, links);

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

FORCEINLINE VOID PhpRotateLeftTwiceAvlLinks(
    __deref_out PPH_AVL_LINKS *Root
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

    // Q.Left = P.Right (transfer B)

    Q->Left = P->Right;

    if (Q->Left)
        Q->Left->Parent = Q;

    // P.Right = Q

    P->Right = Q;
    Q->Parent = P;
}

FORCEINLINE VOID PhpRotateRightTwiceAvlLinks(
    __deref_out PPH_AVL_LINKS *Root
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
    __deref_out PPH_AVL_LINKS *Root
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

PPH_AVL_LINKS PhAvlTreeAdd(
    __inout PPH_AVL_TREE Tree,
    __out PPH_AVL_LINKS Subject
    )
{
    INT result;
    PPH_AVL_LINKS P;
    PPH_AVL_LINKS root;
    LONG balance;

    P = PhpSearchAvlTree(Tree, Subject, &result);

    if (result < 0)
        P->Left = Subject;
    else if (result > 0)
        P->Right = Subject;
    else
        return P;

    Subject->Parent = P;
    Subject->Left = NULL;
    Subject->Right = NULL;
    Subject->Balance = 0;

    // Balance the tree.

    P = Subject;
    root = Tree->Root.Right;

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

    return NULL;
}

VOID PhAvlTreeRemove(
    __inout PPH_AVL_TREE Tree,
    __inout PPH_AVL_LINKS Subject
    )
{
    PPH_AVL_LINKS newSubject;
    PPH_AVL_LINKS *replace;
    PPH_AVL_LINKS P;
    PPH_AVL_LINKS root;
    LONG balance;

    if (!Subject->Left || !Subject->Right)
    {
        newSubject = Subject;
    }
    else if (Subject->Balance < 0) // pick the side depending on the balance to minimize rebalances
    {
        newSubject = Subject->Right;

        while (newSubject->Left)
            newSubject = newSubject->Left;
    }
    else
    {
        newSubject = Subject->Left;

        while (newSubject->Right)
            newSubject = newSubject->Right;
    }

    if (newSubject->Parent->Left == newSubject)
    {
        replace = &newSubject->Parent->Left;
        balance = -1;
    }
    else
    {
        replace = &newSubject->Parent->Right;
        balance = 1;
    }

    if (!newSubject->Right)
    {
        *replace = newSubject->Left;

        if (newSubject->Left)
            newSubject->Left->Parent = newSubject->Parent;
    }
    else
    {
        *replace = newSubject->Right;
        newSubject->Right->Parent = newSubject->Parent; // we know Right exists
    }

    P = newSubject->Parent;
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
        }

        if (P->Parent->Left == P)
            balance = -1;
        else
            balance = 1;

        P = P->Parent;
    }

    if (newSubject != Subject)
    {
        // Replace the subject with the new subject.

        newSubject->Parent = Subject->Parent;
        newSubject->Left = Subject->Left;
        newSubject->Right = Subject->Right;

        if (Subject->Parent->Left == Subject)
            newSubject->Parent->Left = newSubject;
        else
            newSubject->Parent->Right = newSubject;

        if (newSubject->Left)
            newSubject->Left->Parent = newSubject;
        if (newSubject->Right)
            newSubject->Right->Parent = newSubject;
    }
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
