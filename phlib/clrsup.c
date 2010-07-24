/*
 * Process Hacker - 
 *   CLR debugging support
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

#include <ph.h>
#include <clrsup.h>
#define CINTERFACE
#define COBJMACROS
#include <corhdr.h>
#include <corpub.h>
#include <cordebug.h>

PPH_CLR_PUBLISH PhClrCreatePublish()
{
    static CLSID CLSID_CorpubPublish_I = { 0x047a9a40, 0x657e, 0x11d3, { 0x8d, 0x5b, 0x00, 0x10, 0x4b, 0x35, 0xe7, 0xef } };
    static IID IID_ICorPublish_I = { 0x9613a0e7, 0x5a68, 0x11d3, { 0x8f, 0x84, 0x00, 0xa0, 0xc9, 0xb4, 0xd5, 0x0c } };

    ICorPublish *publish;

    if (SUCCEEDED(CoCreateInstance(
        &CLSID_CorpubPublish_I,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_ICorPublish_I,
        &publish
        )))
    {
        return (PPH_CLR_PUBLISH)publish;
    }
    else
    {
        return NULL;
    }
}

VOID PhClrFreePublish(
    __in PPH_CLR_PUBLISH Publish
    )
{
    ICorPublish_Release((ICorPublish *)Publish);
}

PPH_CLR_PUBLISH_PROCESS PhClrGetProcessPublish(
    __in PPH_CLR_PUBLISH Publish,
    __in HANDLE ProcessId
    )
{
    ICorPublishProcess *process;

    if (SUCCEEDED(ICorPublish_GetProcess(
        (ICorPublish *)Publish,
        (ULONG)ProcessId,
        &process
        )))
    {
        return (PPH_CLR_PUBLISH_PROCESS)process;
    }
    else
    {
        return NULL;
    }
}

VOID PhClrFreePublishProcess(
    __in PPH_CLR_PUBLISH_PROCESS Process
    )
{
    ICorPublishProcess_Release((ICorPublishProcess *)Process);
}

BOOLEAN PhClrIsManagedPublishProcess(
    __in PPH_CLR_PUBLISH_PROCESS Process
    )
{
    BOOL managed;

    if (SUCCEEDED(ICorPublishProcess_IsManaged(
        (ICorPublishProcess *)Process,
        &managed
        )))
    {
        return !!managed;
    }

    return FALSE;
}

BOOLEAN PhClrEnumAppDomainsPublishProcess(
    __in PPH_CLR_PUBLISH_PROCESS Process,
    __out PPH_CLR_PUBLISH_APPDOMAIN *AppDomains,
    __out PULONG NumberOfAppDomains
    )
{
    ICorPublishAppDomainEnum *appDomainEnum;

    if (SUCCEEDED(ICorPublishProcess_EnumAppDomains(
        (ICorPublishProcess *)Process,
        &appDomainEnum
        )))
    {
        PPH_CLR_PUBLISH_APPDOMAIN *appDomains;
        ULONG count;

        if (SUCCEEDED(ICorPublishAppDomainEnum_GetCount(
            appDomainEnum,
            &count
            )))
        {
            appDomains = PhAllocate(sizeof(PPH_CLR_PUBLISH_APPDOMAIN) * count);

            if (SUCCEEDED(ICorPublishAppDomainEnum_Next(
                appDomainEnum,
                count,
                (ICorPublishAppDomain **)appDomains,
                &count
                )))
            {
                ICorPublishAppDomainEnum_Release(appDomainEnum);

                *AppDomains = appDomains;
                *NumberOfAppDomains = count;

                return TRUE;
            }

            PhFree(appDomains);
        }

        ICorPublishAppDomainEnum_Release(appDomainEnum);
    }

    return FALSE;
}

VOID PhClrFreePublishAppDomain(
    __in PPH_CLR_PUBLISH_APPDOMAIN AppDomain
    )
{
    ICorPublishAppDomain_Release((ICorPublishAppDomain *)AppDomain);
}

PPH_STRING PhClrGetNamePublishAppDomain(
    __in PPH_CLR_PUBLISH_APPDOMAIN AppDomain
    )
{
    PPH_STRING name;

    name = PhCreateStringEx(NULL, 0x100);

    if (SUCCEEDED(ICorPublishAppDomain_GetName(
        (ICorPublishAppDomain *)AppDomain,
        name->Length,
        NULL,
        name->Buffer
        )))
    {
        PhTrimStringToNullTerminator(name);

        return name;
    }

    PhDereferenceObject(name);

    return NULL;
}
