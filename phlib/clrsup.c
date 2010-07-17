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
#define CINTERFACE
#define COBJMACROS
#include <corhdr.h>
#include <corpub.h>
#include <cordebug.h>
#include <corsym.h>

PVOID PhClrCreatePublish()
{
    ICorPublish *publish;

    if (SUCCEEDED(CoCreateInstance(
        &CLSID_CorpubPublish,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_ICorPublish,
        &publish
        )))
    {
        return publish;
    }
    else
    {
        return NULL;
    }
}

VOID PhClrFreePublish(
    __in PVOID Publish
    )
{
    ICorPublish_Release((ICorPublish *)Publish);
}
