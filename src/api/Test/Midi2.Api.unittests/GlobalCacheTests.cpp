// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://github.com/microsoft/MIDI/
// ============================================================================


#include "pch.h"

#include "catch_amalgamated.hpp"

using namespace winrt::Windows::Devices::Midi2;

TEST_CASE("Offline.Cache.Global Basics")
{
    SECTION("Add and Read Item")
    {
        std::cout << "Add and Read Global Cache Item" << std::endl;

        winrt::hstring propertyKey = L"TEST_KEY";
        winrt::hstring data = L"{ Some Test Data }";

        REQUIRE_NOTHROW(MidiService::GlobalCache());

        std::cout << " - Add item" << std::endl;

        // add item to the cache
        MidiService::GlobalCache().AddOrUpdateData(propertyKey, data);

        // get the item back from the cache

        std::cout << " - Is data present" << std::endl;

        REQUIRE(MidiService::GlobalCache().IsDataPresent(propertyKey));

        std::cout << " - Get data" << std::endl;

        auto retrievedData = MidiService::GlobalCache().GetData(propertyKey);

        std::cout << " - Comparison" << std::endl;

        REQUIRE(retrievedData == data);

        std::cout << " - Done" << std::endl;

    }
}
