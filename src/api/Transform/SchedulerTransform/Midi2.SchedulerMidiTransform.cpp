// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License
// ============================================================================
// This is part of the Windows MIDI Services App API and should be used
// in your Windows application via an official binary distribution.
// Further information: https://aka.ms/midi
// ============================================================================


#include "pch.h"
#include "midi2.Schedulertransform.h"



#define MIDI_SCHEDULER_MINIMUM_EVENT_SLEEP_TIME_MS 2000
#define MIDI_OUTBOUND_EMPTY_QUEUE_SLEEP_DURATION_MS 60000




_Use_decl_annotations_
HRESULT
CMidi2SchedulerMidiTransform::Initialize(
    LPCWSTR deviceId,
    PTRANSFORMCREATIONPARAMS creationParams,
    DWORD * mmcssTaskId,
    IMidiCallback * callback,
    LONGLONG context,
    IUnknown* /*MidiDeviceManager*/
)
{
    UNREFERENCED_PARAMETER(deviceId);
    UNREFERENCED_PARAMETER(creationParams);
    UNREFERENCED_PARAMETER(mmcssTaskId);
    
    TraceLoggingWrite(
        MidiSchedulerTransformTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this")
        );


    m_callback = callback;

    m_context = context;

    // need to make sure this is created before starting up the worker thread
 //   m_messageProcessorWakeup.create(wil::EventOptions::ManualReset);

    // create the queue worker thread
    std::thread workerThread(
        &CMidi2SchedulerMidiTransform::QueueWorker,
        this);

    // TODO: may need to set thread priority
    //SetThreadPriority(workerThread.native_handle(), ... );

    m_queueWorkerThread = std::move(workerThread);

    // start up the worker thread
    m_queueWorkerThread.detach();


    return S_OK;
}

HRESULT
CMidi2SchedulerMidiTransform::Cleanup()
{
    TraceLoggingWrite(
        MidiSchedulerTransformTelemetryProvider::Provider(),
        MIDI_TRACE_EVENT_INFO,
        TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
        TraceLoggingLevel(WINEVENT_LEVEL_INFO),
        TraceLoggingPointer(this, "this")
        );

    try
    {

        // tell the thread to quit. Call SetEvent in case it is in a wait
        m_continueProcessing = false;

        // give the thread a change to terminate
        m_messageProcessorWakeup.SetEvent();
        Sleep(0);

        //// clear the queue
        //bool locked = false;
        //while (!locked)
        //{
        //    // clear the queue
        //    if (m_queueLock.try_lock())
        //    {
        //        while (m_messageQueue.size() > 0)
        //        {
        //            m_messageQueue.pop();
        //        }

        //        locked = true;
        //    }

        //    Sleep(0);
        //}


        // join the worker thread and wait for it to end
        if (m_queueWorkerThread.joinable())
            m_queueWorkerThread.join();

        // don't let the component shut down until we're sure everything is wrapped up.
        // this has a bit of a smell to it, but was having problems with the queue
        // getting torn down before the worker thread was done

        //locked = false;

        //while (!locked)
        //{
        //    // clear the queue
        //    if (m_queueLock.try_lock())
        //    {
        //        locked = true;
        //    }

        //    Sleep(0);
        //}

        return S_OK;
    }
    catch (...)
    {
        TraceLoggingWrite(
            MidiSchedulerTransformTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Exception cleaning up", MIDI_TRACE_EVENT_MESSAGE_FIELD)
        );

        return S_OK;    // we don't care when cleaning up
    }
}

// This is called for messages coming out of the scheduler queue (via
// the overload), or when the scheduler is bypassed due to a timestamp
// of 0 or a timestamp within the "now" send window
_Use_decl_annotations_
HRESULT
CMidi2SchedulerMidiTransform::SendMidiMessageNow(
    PVOID data,
    UINT size,
    LONGLONG timestamp)
{
    try
    {
        if (m_callback != nullptr)
        {
            return m_callback->Callback(data, size, timestamp, m_context);
        }
        else
        {
            TraceLoggingWrite(
                MidiSchedulerTransformTelemetryProvider::Provider(),
                MIDI_TRACE_EVENT_ERROR,
                TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                TraceLoggingPointer(this, "this"),
                TraceLoggingWideString(L"Callback is nullptr", MIDI_TRACE_EVENT_MESSAGE_FIELD)
            );

            return E_FAIL;
        }
    }
    catch (...)
    {
        TraceLoggingWrite(
            MidiSchedulerTransformTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Exception sending MIDI Message", MIDI_TRACE_EVENT_MESSAGE_FIELD)
        );

        return E_FAIL;
    }

}


// This is called for messages coming out of the scheduler queue
_Use_decl_annotations_
HRESULT 
CMidi2SchedulerMidiTransform::SendMidiMessageNow(
    ScheduledUmpMessage const message)
{
    if (!m_continueProcessing) return S_OK;

    return SendMidiMessageNow(
        (PVOID)(message.Data), 
        message.ByteCount, 
        (LONGLONG)(message.Timestamp));

}




_Use_decl_annotations_
HRESULT
CMidi2SchedulerMidiTransform::SendMidiMessage(
    PVOID data,
    UINT size,
    LONGLONG timestamp)
{
    if (!m_continueProcessing) return S_OK;

    try
    {
        // check to see if we're bypassing scheduling by sending a zero timestamp
        if (timestamp == 0)
        {
            // bypass scheduling logic completely
            auto hr = SendMidiMessageNow(data, size, timestamp);

            if (SUCCEEDED(hr))
            {
                return HR_S_MIDI_SENDMSG_IMMEDIATE;
            }
            else
            {
                TraceLoggingWrite(
                    MidiSchedulerTransformTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_ERROR,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Error sending MIDI Message now (bypass queue)", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                    TraceLoggingHResult(hr, MIDI_TRACE_EVENT_HRESULT_FIELD)
                );

                return hr;
            }
        }
        else if (internal::GetCurrentMidiTimestamp() >= timestamp - m_tickWindow - m_deviceLatencyTicks)
        {
            // timestamp is in the past or within our tick window: so send now
            auto hr = SendMidiMessageNow(data, size, timestamp);

            if (SUCCEEDED(hr))
            {
                return HR_S_MIDI_SENDMSG_IMMEDIATE;
            }
            else
            {
                TraceLoggingWrite(
                    MidiSchedulerTransformTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_ERROR,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Error sending MIDI Message now (message timestamp older than window)", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                    TraceLoggingHResult(hr, MIDI_TRACE_EVENT_HRESULT_FIELD)
                );

                return hr;
            }
        }
        else
        {
            // otherwise, we schedule the message

            if (size >= MINIMUM_UMP_DATASIZE && size <= MAXIMUM_UMP_DATASIZE)
            {
                std::lock_guard<std::mutex> lock{ m_queueMutex };

                // recycle the current received index whenever the queue is empty. Prevents long-term wrapping
                if (m_messageQueue.size() == 0)
                {
                    m_currentReceivedIndex = 0;
                }

                // schedule the message for sending in the future

                if (m_messageQueue.size() < MIDI_OUTGOING_MESSAGE_QUEUE_MAX_MESSAGE_COUNT)
                {
                    m_messageQueue.emplace(timestamp, ++m_currentReceivedIndex, size, (byte*)data);

                    // notify the worker thread
                    if (m_continueProcessing) m_messageProcessorWakeup.SetEvent();
                       
                    // break out of the loop and return
                    return HR_S_MIDI_SENDMSG_SCHEDULED;
                }
                else
                {
                    // priority queue doesn't give us any way to pop from the back
                    // so we just have to fail. 

                    TraceLoggingWrite(
                        MidiSchedulerTransformTelemetryProvider::Provider(),
                        __FUNCTION__,
                        TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                        TraceLoggingPointer(this, "this"),
                        TraceLoggingWideString(L"Outbound message queue full", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                    );

                    return HR_E_MIDI_SENDMSG_SCHEDULER_QUEUE_FULL;
                }
            }
            else
            {
                TraceLoggingWrite(
                    MidiSchedulerTransformTelemetryProvider::Provider(),
                    MIDI_TRACE_EVENT_ERROR,
                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                    TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                    TraceLoggingPointer(this, "this"),
                    TraceLoggingWideString(L"Invalid message data size", MIDI_TRACE_EVENT_MESSAGE_FIELD)
                );

                // invalid data size
                return HR_E_MIDI_SENDMSG_INVALID_MESSAGE;
            }
        }
    }
    catch (...)
    {
        TraceLoggingWrite(
            MidiSchedulerTransformTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Exception scheduling message", MIDI_TRACE_EVENT_MESSAGE_FIELD)
        );

        return E_FAIL;
    }

}

_Use_decl_annotations_
HRESULT
CMidi2SchedulerMidiTransform::GetTopMessageTimestamp(internal::MidiTimestamp &timestamp)
{
    if (!m_continueProcessing) return E_FAIL;

    HRESULT ret = E_FAIL;
    timestamp = 0;

    try
    {
        // auto resets on exit
        //auto lock = m_queueLock.try_lock();

        std::lock_guard<std::mutex> lock{ m_queueMutex };

        if (m_continueProcessing && !m_messageQueue.empty())
        {
            timestamp = m_messageQueue.top().Timestamp;
            ret = S_OK;
        }
        else
        {
            ret = E_FAIL;
        }
    }
    catch (...)
    {
        TraceLoggingWrite(
            MidiSchedulerTransformTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Exception getting top-message timestamp", MIDI_TRACE_EVENT_MESSAGE_FIELD)
        );

        ret = E_FAIL;
    }

    return ret;
}


_Use_decl_annotations_
HRESULT
CMidi2SchedulerMidiTransform::CalculateSafeSleepTime(
    internal::MidiTimestamp nextWakeupWindowTimestamp, 
    uint32_t& sleepMS
)
{
    if (!m_continueProcessing) return E_FAIL;

    HRESULT ret = E_FAIL;
    sleepMS = 0;

    try
    {
        auto now = internal::GetCurrentMidiTimestamp();

        if (nextWakeupWindowTimestamp > now)
        {
            // calculate diff in seconds
            auto diff = (nextWakeupWindowTimestamp - now) / m_timestampFrequency;

            // convert to milliseconds
            sleepMS = (uint32_t)(diff * 1000);

            // if the sleep time is under the limit, we don't sleep at all because the timing is not that accurate
            if (sleepMS < MIDI_SCHEDULER_MINIMUM_EVENT_SLEEP_TIME_MS)
            {
                sleepMS = 0;
            }
        }

        return S_OK;
    }
    catch (...)
    {
        TraceLoggingWrite(
            MidiSchedulerTransformTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Exception calculating safe sleep time", MIDI_TRACE_EVENT_MESSAGE_FIELD)
        );

        ret = E_FAIL;
    }

    return ret;
}



void CMidi2SchedulerMidiTransform::QueueWorker()
{
    try
    {
        const uint64_t totalExpectedLatency = m_deviceLatencyTicks + MIDI_SCHEDULER_LOCK_AND_SEND_FUNCTION_LATENCY_TICKS;

        while (m_continueProcessing)
        {
            // check to see if the queue is empty, and if so, go to sleep until we're signaled
            // to wake up due to a new message arriving or due to shut down.
            if (m_continueProcessing && m_messageQueue.empty())
            {
                // queue empty so sleep until we get notified to wake up
                m_messageProcessorWakeup.wait(MIDI_OUTBOUND_EMPTY_QUEUE_SLEEP_DURATION_MS);
                //bool triggered = m_messageProcessorWakeup.wait(MIDI_OUTBOUND_EMPTY_QUEUE_SLEEP_DURATION_MS);
            }
            else if (m_continueProcessing && !m_messageQueue.empty())
            {
                internal::MidiTimestamp topTimestamp = 0;

                // get the timestamp of the most urgent message. We fetch this
                // each time in case messages were added since last loop iteration
                if (SUCCEEDED(GetTopMessageTimestamp(topTimestamp)))
                {
                    uint64_t nextMessageSendTime = topTimestamp - totalExpectedLatency;

                    // check to see if it's time to send the message. If not, we'll just
                    // wrap back around
                    if (internal::GetCurrentMidiTimestamp() >= nextMessageSendTime)
                    {
                        std::lock_guard<std::mutex> lock{ m_queueMutex };

                        // we have the queue locked, so send ALL messages that have the *same* timestamp
                        // but we need to limit the number to send at once here, so we do.

                        uint32_t processedMessages = 0;

                        while (!m_messageQueue.empty() && 
                            processedMessages < MIDI_SCHEDULER_MAX_MESSAGES_TO_PROCESS_AT_ONCE &&
                            m_messageQueue.top().Timestamp <= topTimestamp)
                        {
                            // send the message
                            auto hr = SendMidiMessageNow(m_messageQueue.top());

                            if (SUCCEEDED(hr))
                            {
                                // pop the top item off the queue
                                m_messageQueue.pop();
                            }
                            else
                            {
                                LOG_IF_FAILED(hr);

                                TraceLoggingWrite(
                                    MidiSchedulerTransformTelemetryProvider::Provider(),
                                    MIDI_TRACE_EVENT_ERROR,
                                    TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
                                    TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                                    TraceLoggingPointer(this, "this"),
                                    TraceLoggingWideString(L"Unable to send message", MIDI_TRACE_EVENT_MESSAGE_FIELD),
                                    TraceLoggingHResult(hr, MIDI_TRACE_EVENT_HRESULT_FIELD)
                                );

                                // we failed to send for some reason, so break out of the loop. We'll catch 
                                // these messages the next time around.
                                break;
                            }

                            processedMessages++;
                        }
                    }
                    else
                    {
                        // not yet time to send the top message, so we'll sleep
                        uint32_t sleepTime{ 0 };

                        if (m_continueProcessing && SUCCEEDED(CalculateSafeSleepTime(nextMessageSendTime, sleepTime)))
                        {
                            if (sleepTime > 0)
                            {
                                // waiting will get interrupted if a new message comes in. We want that.
                                m_messageProcessorWakeup.wait(sleepTime);
                            }
                        }
                    }
                }
                else
                {
                    // couldn't get top timestamp for some reason.
                }

            }

            if (m_continueProcessing)
            {
                // we're looping, not sleeping now, so we need to reset this to make sure we don't miss new messages
                if (m_messageProcessorWakeup.is_signaled())
                {
                    m_messageProcessorWakeup.ResetEvent();
                }

                // return the rest of the time slice so we're not in a tight loop
                Sleep(0);
            }

        } // main loop
    }
    catch (...)
    {
        TraceLoggingWrite(
            MidiSchedulerTransformTelemetryProvider::Provider(),
            MIDI_TRACE_EVENT_ERROR,
            TraceLoggingString(__FUNCTION__, MIDI_TRACE_EVENT_LOCATION_FIELD),
            TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
            TraceLoggingPointer(this, "this"),
            TraceLoggingWideString(L"Exception processing queue", MIDI_TRACE_EVENT_MESSAGE_FIELD)
        );
    }
}


