/*
 * Process Hacker ToolStatus -
 *   search filter callbacks
 *
 * Copyright (C) 2011-2013 dmex
 * Copyright (C) 2010-2012 wj32
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

#include "toolstatus.h"

static BOOLEAN WordMatch(
    __in PPH_STRINGREF Text,
    __in PPH_STRINGREF Search,
    __in BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = *Search;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, ' ', &part, &remainingPart);

        if (part.Length != 0)
        {
            if (PhFindStringInStringRef(Text, &part, IgnoreCase) != -1)
                return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN ProcessTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    HWND textboxHandle = (HWND)Context;
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;
    PPH_STRING textboxText;

    textboxText = PhGetWindowText(textboxHandle);

    if (textboxText)
    {
        if (textboxText->Length > 0)
        {
            BOOLEAN showItem = FALSE;
            PH_STRINGREF pidStringRef;

            PhInitializeStringRef(&pidStringRef, processNode->ProcessItem->ProcessIdString);
      
            // Search process PIDs
            if (WordMatch(&pidStringRef, &textboxText->sr, TRUE))
                showItem = TRUE;

            // Search process names
            if (processNode->ProcessItem->ProcessName)
            {
                if (WordMatch(&processNode->ProcessItem->ProcessName->sr, &textboxText->sr, TRUE))
                    showItem = TRUE;
            }

            // Search process company
            if (processNode->ProcessItem->VersionInfo.CompanyName)
            {
                if (WordMatch(&processNode->ProcessItem->VersionInfo.CompanyName->sr, &textboxText->sr, TRUE))
                    showItem = TRUE;
            }

            // Search process descriptions
            if (processNode->ProcessItem->VersionInfo.FileDescription)
            {
                if (WordMatch(&processNode->ProcessItem->VersionInfo.FileDescription->sr, &textboxText->sr, TRUE))
                    showItem = TRUE;
            }

            PhDereferenceObject(textboxText);
            return showItem;
        }

        PhDereferenceObject(textboxText);
    }

    return TRUE;
}

BOOLEAN ServiceTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    HWND textboxHandle = (HWND)Context;
    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)Node; 
    PPH_STRING textboxText;

    textboxText = PhGetWindowText(textboxHandle);

    if (textboxText)
    {
        if (textboxText->Length > 0)
        {
            BOOLEAN showItem = FALSE;
            PH_STRINGREF pidStringRef;

            PhInitializeStringRef(&pidStringRef, serviceNode->ServiceItem->ProcessIdString);

            // Search process PIDs
            if (WordMatch(&pidStringRef, &textboxText->sr, TRUE))
                showItem = TRUE;

            // Search service name
            if (serviceNode->ServiceItem->Name)
            {
                if (WordMatch(&serviceNode->ServiceItem->Name->sr, &textboxText->sr, TRUE))
                    showItem = TRUE;
            }

            // Search service display name
            if (serviceNode->ServiceItem->DisplayName)
            {
                if (WordMatch(&serviceNode->ServiceItem->DisplayName->sr, &textboxText->sr, TRUE))
                    showItem = TRUE;
            }

            PhDereferenceObject(textboxText);
            return showItem;
        }

        PhDereferenceObject(textboxText);
    }

    return TRUE;
}

BOOLEAN NetworkTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    HWND textboxHandle = (HWND)Context;
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;
    PPH_STRING textboxText;

    textboxText = PhGetWindowText(textboxHandle);

    if (textboxText)
    {
        if (textboxText->Length > 0)
        {
            BOOLEAN showItem = FALSE;
            PH_STRINGREF pidStringRef;
            WCHAR pidString[32];

            // Search PID
            PhPrintUInt32(pidString, (ULONG)networkNode->NetworkItem->ProcessId);
            PhInitializeStringRef(&pidStringRef, pidString);

            // Search network process PIDs
            if (WordMatch(&pidStringRef, &textboxText->sr, TRUE))
            {
                showItem = TRUE;
            }

            // Search connection process name
            if (networkNode->NetworkItem->ProcessName)
            {
                if (WordMatch(&networkNode->NetworkItem->ProcessName->sr, &textboxText->sr, TRUE))
                {
                    showItem = TRUE;
                }
            }

            // Search connection local IP address
            if (networkNode->NetworkItem->LocalAddressString)
            {
                PH_STRINGREF localAddressRef;
                PhInitializeStringRef(&localAddressRef, networkNode->NetworkItem->LocalAddressString);

                if (WordMatch(&localAddressRef, &textboxText->sr, TRUE))
                    showItem = TRUE;
            }

            if (networkNode->NetworkItem->LocalPortString)
            {
                PH_STRINGREF localPortRef;
                PhInitializeStringRef(&localPortRef, networkNode->NetworkItem->LocalPortString);

                if (WordMatch(&localPortRef, &textboxText->sr, TRUE))
                    showItem = TRUE;
            }

            if (networkNode->NetworkItem->RemoteAddressString)
            {
                PH_STRINGREF remoteAddressRef;
                PhInitializeStringRef(&remoteAddressRef, networkNode->NetworkItem->RemoteAddressString);

                if (WordMatch(&remoteAddressRef, &textboxText->sr, TRUE))
                    showItem = TRUE;
            }
                        
            if (networkNode->NetworkItem->RemotePortString)
            {
                PH_STRINGREF remotePortRef;

                PhInitializeStringRef(&remotePortRef, networkNode->NetworkItem->RemotePortString);

                if (WordMatch(&remotePortRef, &textboxText->sr, TRUE))
                    showItem = TRUE;
            }

            if (networkNode->NetworkItem->RemoteHostString)
            {
                if (WordMatch(&networkNode->NetworkItem->RemoteHostString->sr, &textboxText->sr, TRUE))
                    showItem = TRUE;
            }

            PhDereferenceObject(textboxText);
            return showItem;
        }

        PhDereferenceObject(textboxText);
    }

    return TRUE;
}