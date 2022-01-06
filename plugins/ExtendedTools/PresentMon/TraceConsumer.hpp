// Copyright (C) 2017-2020 Intel Corporation
// SPDX-License-Identifier: MIT

#pragma once

#include <evntcons.h>
#include <tdh.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <deque>
#include <map>
#include <memory>
#include <tuple>
#include <set>

struct EventMetadataKey
{
    GUID guid_;
    EVENT_DESCRIPTOR desc_;
};

struct EventMetadataKeyHash { size_t operator()(EventMetadataKey const& k) const; }; 
struct EventMetadataKeyEqual { bool operator()(EventMetadataKey const& lhs, EventMetadataKey const& rhs) const; };

enum PropertyStatus {
    PROP_STATUS_NOT_FOUND       = 0,
    PROP_STATUS_FOUND           = 1 << 0,
    PROP_STATUS_CHAR_STRING     = 1 << 1,
    PROP_STATUS_WCHAR_STRING    = 1 << 2,
    PROP_STATUS_NULL_TERMINATED = 1 << 3,
    PROP_STATUS_POINTER_SIZE    = 1 << 4,
};

struct EventDataDesc
{
    wchar_t const* name_;   // Property name
    uint32_t arrayIndex_;   // Array index (optional)
    void* data_;            // OUT pointer to property data
    uint32_t size_;         // OUT size of property data
    uint32_t status_;       // OUT PropertyStatus result of search

    template<typename T> T GetData() const
    {
        if (!(status_ & PROP_STATUS_FOUND))
        {
            T t{};
            return t;
        }

        //assert(status_ & PROP_STATUS_FOUND);
        assert(data_ != nullptr);

        // Expect the correct size, except allow 32-bit pointer/size_t to promote to u64
        assert(
            size_ == sizeof(T) ||
            ((status_ & PROP_STATUS_POINTER_SIZE) != 0 && size_ == 4 && sizeof(T) == 8));

        T t {};
        memcpy(&t, data_, size_);
        return t;
    }
};

template<> std::string EventDataDesc::GetData<std::string>() const;
template<> std::wstring EventDataDesc::GetData<std::wstring>() const;

struct EventMetadata
{
    std::unordered_map<EventMetadataKey, std::vector<uint8_t>, EventMetadataKeyHash, EventMetadataKeyEqual> metadata_;

    void AddMetadata(EVENT_RECORD* eventRecord);
    void GetEventData(EVENT_RECORD* eventRecord, EventDataDesc* desc, uint32_t descCount, uint32_t optionalCount=0);

    template<typename T> T GetEventData(EVENT_RECORD* eventRecord, wchar_t const* name, uint32_t arrayIndex = 0)
    {
        EventDataDesc desc = { name, arrayIndex, };
        GetEventData(eventRecord, &desc, 1);
        return desc.GetData<T>();
    }
};
