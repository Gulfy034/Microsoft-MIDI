// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================


#pragma once


class MidiBenchmarks
    : public WEX::TestClass<MidiBenchmarks>
{
public:

    BEGIN_TEST_CLASS(MidiBenchmarks)
        TEST_CLASS_PROPERTY(L"TestClassification", L"Benchmark")
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.Devices.Midi2.dll")
    END_TEST_CLASS()

        //TEST_CLASS_SETUP(ClassSetup);
        //TEST_CLASS_CLEANUP(ClassCleanup);

        //TEST_METHOD_SETUP(TestSetup);
        //TEST_METHOD_CLEANUP(TestCleanup);

    TEST_METHOD(BenchmarkSendReceiveWordArray);
    TEST_METHOD(BenchmarkSendReceiveUmpRuntimeClass);


private:


};