/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"

#include <usernotesintf.h>

// What the user has saved against a process in UserNotes: the comment, and the priority, affinity
// and colour the plugin reapplies whenever that program runs again. The database is keyed on the
// file name or on the whole command line, never on the pid.

PUSERNOTES_INTERFACE AtGetUserNotesInterface(
    VOID
    )
{
    static PUSERNOTES_INTERFACE pluginInterface = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_PLUGIN plugin;

        if (plugin = PhFindPlugin(USERNOTES_PLUGIN_NAME))
        {
            pluginInterface = PhGetPluginInformation(plugin)->Interface;

            if (pluginInterface && pluginInterface->Version < USERNOTES_INTERFACE_VERSION)
                pluginInterface = NULL;
        }

        PhEndInitOnce(&initOnce);
    }

    return pluginInterface;
}

VOID AtpSetUserNotesMissing(
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AtSetToolHint(Result, AT_HINT_PLUGIN_MISSING);
    AtSetToolError(
        Result,
        "plugin_missing",
        STATUS_NOT_FOUND,
        L"The UserNotes plugin is not loaded, so there are no saved notes."
        );
}

VOID AtpGetProcessNotes(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PUSERNOTES_INTERFACE pluginInterface;
    USERNOTES_PROCESS_NOTES notes;
    AT_TARGET target;
    PVOID structured;

    if (!(pluginInterface = AtGetUserNotesInterface()))
    {
        AtpSetUserNotesMissing(Result);
        return;
    }

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);

    if (pluginInterface->GetProcessNotes(target.ProcessItem, &notes))
    {
        PhAddJsonObjectBoolean(structured, "has_entry", TRUE);
        // Which key the entry that answered is filed under. Writing to the other one leaves this
        // one in force, because the command line entry is the one the plugin applies.
        PhAddJsonObject(structured, "matched", notes.MatchedCommandLine ? "command_line" : "file_name");
        AtJsonAddString(structured, "comment", notes.Comment);

        if (notes.PriorityClass)
            AtJsonAddStringZ(structured, "saved_priority_class", AtPriorityClassString((UCHAR)notes.PriorityClass));
        else
            AtJsonAddNull(structured, "saved_priority_class");

        // The database keeps these one greater than the value they carry so that zero can mean
        // "nothing saved"; the saved value is what the caller wants.
        if (notes.IoPriorityPlusOne)
            PhAddJsonObjectUInt64(structured, "saved_io_priority", notes.IoPriorityPlusOne - 1);
        else
            AtJsonAddNull(structured, "saved_io_priority");

        if (notes.PagePriorityPlusOne)
            PhAddJsonObjectUInt64(structured, "saved_page_priority", notes.PagePriorityPlusOne - 1);
        else
            AtJsonAddNull(structured, "saved_page_priority");

        if (notes.AffinityMask)
            AtJsonAddHex(structured, "saved_affinity_mask", notes.AffinityMask);
        else
            AtJsonAddNull(structured, "saved_affinity_mask");

        if (notes.BackColor != ULONG_MAX)
            AtJsonAddHex(structured, "highlight_color", notes.BackColor);
        else
            AtJsonAddNull(structured, "highlight_color");

        PhAddJsonObjectBoolean(structured, "collapse", notes.Collapse);
        PhAddJsonObjectBoolean(structured, "boost", notes.Boost);
        PhAddJsonObjectBoolean(structured, "efficiency", notes.Efficiency);

        PhClearReference(&notes.Comment);
    }
    else
    {
        // Nothing saved is not an error: most processes have no entry at all.
        PhAddJsonObjectBoolean(structured, "has_entry", FALSE);
        AtJsonAddNull(structured, "matched");
        AtJsonAddNull(structured, "comment");
        AtJsonAddNull(structured, "saved_priority_class");
        AtJsonAddNull(structured, "saved_io_priority");
        AtJsonAddNull(structured, "saved_page_priority");
        AtJsonAddNull(structured, "saved_affinity_mask");
        AtJsonAddNull(structured, "highlight_color");
        PhAddJsonObjectBoolean(structured, "collapse", FALSE);
        PhAddJsonObjectBoolean(structured, "boost", FALSE);
        PhAddJsonObjectBoolean(structured, "efficiency", FALSE);
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

VOID AtpSetProcessComment(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PUSERNOTES_INTERFACE pluginInterface;
    PPH_STRING comment;
    BOOLEAN matchCommandLine;
    PVOID structured;

    if (!(pluginInterface = AtGetUserNotesInterface()))
    {
        AtpSetUserNotesMissing(Result);
        return;
    }

    matchCommandLine = AtJsonGetObjectBoolean(Call->Arguments, "match_command_line");

    if (matchCommandLine && !Target->ProcessItem->CommandLine)
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            L"This process has no command line to file the comment under."
            );
        return;
    }

    comment = AtGetArgumentString(Call->Arguments, "comment");

    if (!pluginInterface->SetProcessComment(
        Target->ProcessItem,
        comment ? &comment->sr : NULL,
        matchCommandLine
        ))
    {
        // The only way this fails after the checks above is clearing a comment that was not there.
        AtSetToolError(
            Result,
            "not_found",
            STATUS_NOT_FOUND,
            L"There was no saved entry to change."
            );
        PhClearReference(&comment);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    PhAddJsonObject(structured, "matched", matchCommandLine ? "command_line" : "file_name");
    AtJsonAddString(structured, "comment", comment && comment->Length ? comment : NULL);
    PhAddJsonObjectBoolean(structured, "cleared", !(comment && comment->Length));
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&comment);
}

VOID AtNoteInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionGetProcessNotes:
        AtpGetProcessNotes(Call, Result);
        break;
    case AtActionSetProcessComment:
        AtpSetProcessComment(Call, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
