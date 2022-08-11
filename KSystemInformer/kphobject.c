/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>

#include <trace.h>

//
// N.B. If more object types are added the array must be expanded.
//

static volatile LONG KphpObjectTypeCount = 0;
static KPH_OBJECT_TYPE KphpObjectTypes[9];

C_ASSERT(ARRAYSIZE(KphpObjectTypes) < MAXUCHAR);

/**
 * \brief Creates an object type.
 *
 * \param[in] TypeName The name of the type, this must always be resident.
 * Preferably a string literal.
 * \param[in] TypeInfo The information for the type being created.
 * \param[out] ObjectType Set to a pointer to the object type on success.
 */
VOID
KphCreateObjectType(
    _In_ PUNICODE_STRING TypeName,
    _In_ PKPH_OBJECT_TYPE_INFO TypeInfo,
    _Outptr_ PKPH_OBJECT_TYPE* ObjectType
    )
{
    PKPH_OBJECT_TYPE type;
    LONG index;

    index = (InterlockedIncrement(&KphpObjectTypeCount) - 1);

    //
    // We have failure free object type creation, to achieve this we have
    // a pre-reserved sized array above. If this asserts the array wasn't
    // expanded correctly to support a new type.
    //
    NT_ASSERT((index >= 0) && (index < ARRAYSIZE(KphpObjectTypes)));
    NT_ASSERT(index < MAXUCHAR);

    type = &KphpObjectTypes[index];

    type->Name.Buffer = TypeName->Buffer;
    type->Name.MaximumLength = TypeName->MaximumLength;
    type->Name.Length = TypeName->Length;

    type->Index = (UCHAR)index;
    type->TotalNumberOfObjects = 0;
    type->HighWaterNumberOfObjects = 0;

    RtlCopyMemory(&type->TypeInfo, TypeInfo, sizeof(*TypeInfo));

    *ObjectType = type;
}

/**
 * \brief Creates an object of a given type.
 *
 * \param[in] ObjectType The type of object to create.
 * \param[in] ObjectBodySize The size of the object body to create.
 * \param[out] Object Set to the new object on success.
 * \param[in] Parameter Optional parameter passed to initialization routine.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS
KphCreateObject(
    _In_ PKPH_OBJECT_TYPE ObjectType,
    _In_ ULONG ObjectBodySize,
    _Outptr_result_nullonfailure_ PVOID* Object,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    PKPH_OBJECT_HEADER header;
    PVOID object;
    SIZE_T total;

    *Object = NULL;

    header = ObjectType->TypeInfo.Allocate(KphAddObjectHeaderSize(ObjectBodySize));
    if (!header)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    object = KphObjectHeaderToObject(header);

    if (ObjectType->TypeInfo.Initialize)
    {
        status = ObjectType->TypeInfo.Initialize(object, Parameter);
        if (!NT_SUCCESS(status))
        {
            ObjectType->TypeInfo.Free(header);
            return status;
        }
    }

    header->PointerCount = 1;
    header->TypeIndex = ObjectType->Index;

    total = InterlockedIncrementSizeT(&ObjectType->TotalNumberOfObjects);

    InterlockedExchangeIfGreaterSizeT(&ObjectType->HighWaterNumberOfObjects,
                                      total);

    *Object = object;
    return STATUS_SUCCESS;
}

/**
 * \brief References an object.
 *
 * \param[in] Object The object to reference.
 */
VOID
KphReferenceObject(
    _In_ PVOID Object
    )
{
    PKPH_OBJECT_HEADER header;

    header = KphObjectToObjectHeader(Object);

    NT_VERIFY(InterlockedIncrementSSizeT(&header->PointerCount) > 0);
}

/**
 * \brief Dereferences an object.
 *
 * \param[in] Object The object to dereference.
 */
VOID
KphDereferenceObject(
    _In_ PVOID Object
    )
{
    PKPH_OBJECT_HEADER header;
    PKPH_OBJECT_TYPE type;
    SSIZE_T refCount;

    header = KphObjectToObjectHeader(Object);

    refCount = InterlockedDecrementSSizeT(&header->PointerCount);
    if (refCount > 0)
    {
        return;
    }

    NT_ASSERT(refCount == 0);

    type = &KphpObjectTypes[header->TypeIndex];

    if (type->TypeInfo.Delete)
    {
        type->TypeInfo.Delete(Object);
    }

    type->TypeInfo.Free(header);

    InterlockedDecrementSizeT(&type->TotalNumberOfObjects);
}

/**
 * \brief Retrieves the type from an object.
 *
 * \param[in] Object The object to get the type of.
 *
 * \return Pointer to the type of object.
 */
_Must_inspect_result_
PKPH_OBJECT_TYPE
KphGetObjectType(
    _In_ PVOID Object
    )
{
    PKPH_OBJECT_HEADER header;
    UCHAR index;
    LONG count;

    header = KphObjectToObjectHeader(Object);
    index = header->TypeIndex;
    count = KphpObjectTypeCount;

    if (index >= count)
    {
        return NULL;
    }

    return &KphpObjectTypes[index];
}
