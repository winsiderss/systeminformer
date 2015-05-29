/*
 * Process Hacker Extra Plugins -
 *   Network Extras Plugin
 *
 * Copyright (C) 2015 dmex
 * Copyright (C) 2015 TETYYS
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

#include "main.h"

VOID UpdateNetworkNode(
    _In_ NETWORK_COLUMN_ID ColumnID,
    _In_ PPH_NETWORK_NODE Node,
    _In_ PNETWORK_EXTENSION Extension
    )
{
    switch (ColumnID)
    {
    case NETWORK_COLUMN_ID_LOCAL_SERVICE:
        {
            if (!Extension->LocalValid)
            {
                //PH_STRING_BUILDER stringBuilder;

                //PhInitializeStringBuilder(&stringBuilder, 24);

                for (ULONG x = 0; x < _countof(ResolvedPortsTable); x++)
                {
                    if (Node->NetworkItem->LocalEndpoint.Port == ResolvedPortsTable[x].Port)
                    {
                        //PhAppendFormatStringBuilder(&stringBuilder, L"%s,", ResolvedPortsTable[x].Name);

                        PhSwapReference(&Extension->LocalServiceName, PhCreateString(ResolvedPortsTable[x].Name));
                        break;
                    }
                }

                //if (stringBuilder.String->Length != 0)
                //    PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

                //PhSwapReference(&Extension->LocalServiceName, PhFinalStringBuilderString(&stringBuilder));

                Extension->LocalValid = TRUE;
            }
        }
        break;
    case NETWORK_COLUMN_ID_REMOTE_SERVICE:
        {
            if (!Extension->RemoteValid)
            {
                //PH_STRING_BUILDER stringBuilder;

                //PhInitializeStringBuilder(&stringBuilder, 24);

                for (ULONG x = 0; x < _countof(ResolvedPortsTable); x++)
                {
                    if (Node->NetworkItem->RemoteEndpoint.Port == ResolvedPortsTable[x].Port)
                    {
                        //PhAppendFormatStringBuilder(&stringBuilder, L"%s,", ResolvedPortsTable[x].Name);

                        PhSwapReference(&Extension->RemoteServiceName, PhCreateString(ResolvedPortsTable[x].Name));
                        break;
                    }
                }

                //if (stringBuilder.String->Length != 0)
                //    PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

                //PhSwapReference(&Extension->RemoteServiceName, PhFinalStringBuilderString(&stringBuilder));

                Extension->RemoteValid = TRUE;
            }
        }
        break;
    }
}