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
    Tree->Root.Parent = &Tree->Root;
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

    links = &Tree->Root;

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
    PPH_BINARY_LINKS temp;

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
            temp = Subject->Left;

            while (temp->Right)
                temp = temp->Right;
        }
        else
        {
            temp = Subject->Right;

            while (temp->Left)
                temp = temp->Left;
        }

        temp->Parent = Subject->Parent;
        temp->Left = Subject->Left;
        temp->Right = Subject->Right;
        *replace = temp;
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
