// Copyright (c) Microsoft Corporation. All rights reserved.
#pragma once

MODULE_SETUP(ModuleSetup);
bool ModuleSetup()
{
    winrt::init_apartment();
    return true;
}

class Midi2DriverTests
    : public WEX::TestClass<Midi2DriverTests>,
    public IMidiCallback
{
public:

    BEGIN_TEST_CLASS(Midi2DriverTests)
        TEST_CLASS_PROPERTY(L"TestClassification", L"Unit")
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Minmidi.sys")        
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"usbmidi2.sys")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup);
    TEST_CLASS_CLEANUP(ClassCleanup);

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    //Generic Tests
    TEST_METHOD(TestCyclicUMPMidiIO);
    TEST_METHOD(TestCyclicByteStreamMidiIO);
    TEST_METHOD(TestStandardMidiIO);

    TEST_METHOD(TestCyclicUMPMidiIO_ManyMessages);
    TEST_METHOD(TestCyclicByteStreamMidiIO_ManyMessages);
    TEST_METHOD(TestStandardMidiIO_ManyMessages);

    TEST_METHOD(TestCyclicUMPMidiIO_Latency);
    TEST_METHOD(TestCyclicByteStreamMidiIO_Latency);
    TEST_METHOD(TestStandardMidiIO_Latency);

    TEST_METHOD(TestCyclicUMPMidiIOSlowMessages_Latency);
    TEST_METHOD(TestCyclicByteStreamMidiIOSlowMessages_Latency);
    TEST_METHOD(TestStandardMidiIOSlowMessages_Latency);

    Midi2DriverTests()
    {}

    STDMETHOD(Callback)(_In_ PVOID Data, _In_ UINT Size, _In_ LONGLONG Position, _In_ LONGLONG Context)
    {
        if (m_MidiInCallback)
        {
            m_MidiInCallback(Data, Size, Position, Context);
        }
        return S_OK;
    }

    // The test library is not refcounted, stubbed.
    STDMETHODIMP QueryInterface(REFIID, void**) { return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 1; }

private:
    void TestMidiIO(MidiTransport Transport);
    void TestMidiIO_ManyMessages(MidiTransport Transport);
    void TestMidiIO_Latency(MidiTransport Transport, BOOL DelayedMessages);
    
    std::function<void(PVOID, UINT32, LONGLONG, LONGLONG)> m_MidiInCallback;
};

