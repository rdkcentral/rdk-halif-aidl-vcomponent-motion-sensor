/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "aidl/vcomponent_MotionSensor.h"

#include "aidl/vcomponent_MotionSensorController.h"
#include "common/logger.h"

#include <com/rdk/hal/sensor/motion/MotionEvent.h>

#include <binder/IInterface.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <ctime>

namespace com::rdk::hal::sensor::motion
{

namespace
{
constexpr const char* logPrefix = "[VDEVICE_MOTION]<MotionSensor>";
constexpr int32_t secondsPerDay = 24 * 60 * 60;

int32_t currentLocalTimeOfDaySeconds()
{
    const std::time_t currentTime = std::time(nullptr);
    std::tm localTime{};

#if defined(_WIN32)
    if (localtime_s(&localTime, &currentTime) != 0)
#else
    if (localtime_r(&currentTime, &localTime) == nullptr)
#endif
    {
        return -1;
    }

    return (localTime.tm_hour * 60 * 60) + (localTime.tm_min * 60) + localTime.tm_sec;
}

android::sp<android::IBinder> binderForController(
    const android::sp<IMotionSensorController>& controller)
{
    return controller == nullptr ? nullptr : android::IInterface::asBinder(controller);
}

android::sp<android::IBinder> binderForControllerListener(
    const android::sp<IMotionSensorControllerListener>& listener)
{
    return listener == nullptr ? nullptr : android::IInterface::asBinder(listener);
}

android::sp<android::IBinder> binderForEventListener(
    const android::sp<IMotionSensorEventListener>& listener)
{
    return listener == nullptr ? nullptr : android::IInterface::asBinder(listener);
}
} // namespace

MotionSensor::MotionSensor(
    const IMotionSensor::Id& id,
    const Capabilities& capabilities,
    const StartConfig& defaultStartConfig,
    const std::vector<TimeWindow>& defaultActiveWindows)
    : m_id(id)
    , m_capabilities(capabilities)
    , m_sensitivity(capabilities.minSensitivity)
    , m_autonomousDuringDeepSleepEnabled(false)
    , m_startConfig(defaultStartConfig)
    , m_defaultActiveWindows(defaultActiveWindows)
    , m_activeWindows(defaultActiveWindows)
{
    LOGF_INFO(
        "%s: created motion sensor id=%d sensitivity=%d",
        logPrefix,
        static_cast<int>(m_id.value),
        static_cast<int>(m_sensitivity));
}

MotionSensor::~MotionSensor()
{
    std::thread lifecycleTimer;
    std::thread noMotionTimer;
    std::thread activeWindowScheduler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lifecycleTimerCancelled = true;
        ++m_lifecycleGeneration;
        m_lifecycleTimerCondition.notify_all();
        m_noMotionTimerCancelled = true;
        ++m_noMotionTimerGeneration;
        m_noMotionTimerCondition.notify_all();
        m_activeWindowSchedulerCancelled = true;
        ++m_activeWindowSchedulerGeneration;
        m_activeWindowSchedulerCondition.notify_all();
        lifecycleTimer = std::move(m_lifecycleTimerThread);
        noMotionTimer = std::move(m_noMotionTimerThread);
        activeWindowScheduler = std::move(m_activeWindowSchedulerThread);
    }

    if (lifecycleTimer.joinable())
    {
        lifecycleTimer.join();
    }

    if (noMotionTimer.joinable())
    {
        noMotionTimer.join();
    }

    if (activeWindowScheduler.joinable())
    {
        activeWindowScheduler.join();
    }
}

android::binder::Status MotionSensor::getCapabilities(Capabilities* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getCapabilities: null _aidl_return", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    *_aidl_return = m_capabilities;
    return android::binder::Status::ok();
}

android::binder::Status MotionSensor::getState(State* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getState: null _aidl_return", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    *_aidl_return = m_state;
    return android::binder::Status::ok();
}

android::binder::Status MotionSensor::open(
    const android::sp<IMotionSensorControllerListener>& listener,
    android::sp<IMotionSensorController>* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: open: null _aidl_return", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = nullptr;

    if (listener == nullptr)
    {
        LOGF_ERR("%s: open: null listener", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    android::sp<android::IBinder> ownerBinder = binderForControllerListener(listener);
    if (ownerBinder == nullptr)
    {
        LOGF_ERR("%s: open: listener has no Binder identity", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_controller != nullptr)
    {
        LOGF_WARN("%s: open rejected because sensor id=%d is already open", logPrefix, m_id.value);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    m_controllerListener = listener;
    m_ownerBinder = ownerBinder;
    m_controller = android::sp<MotionSensorController>::make(this, listener);
    *_aidl_return = m_controller;

    const android::status_t linkStatus = ownerBinder->linkToDeath(this);
    if (linkStatus != android::NO_ERROR)
    {
        LOGF_WARN(
            "%s: open could not link controller listener death recipient for sensor id=%d status=%d",
            logPrefix,
            m_id.value,
            static_cast<int>(linkStatus));
    }

    LOGF_INFO("%s: opened sensor id=%d", logPrefix, m_id.value);
    return android::binder::Status::ok();
}

android::binder::Status MotionSensor::close(
    const android::sp<IMotionSensorController>& controller,
    bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: close: null _aidl_return", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;

    if (controller == nullptr)
    {
        LOGF_ERR("%s: close: null controller", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_state != State::STOPPED && m_state != State::ERROR)
    {
        LOGF_WARN(
            "%s: close rejected for sensor id=%d while state=%d",
            logPrefix,
            m_id.value,
            static_cast<int32_t>(m_state));
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (!controllerMatchesLocked(controller))
    {
        LOGF_WARN("%s: close supplied non-owner controller for sensor id=%d", logPrefix, m_id.value);
        return android::binder::Status::ok();
    }

    if (m_state == State::ERROR)
    {
        changeStateLocked(lock, State::STOPPED);
    }

    releaseControllerLocked();
    *_aidl_return = true;

    LOGF_INFO("%s: closed sensor id=%d", logPrefix, m_id.value);
    return android::binder::Status::ok();
}

android::binder::Status MotionSensor::registerEventListener(
    const android::sp<IMotionSensorEventListener>& motionSensorEventListener,
    bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: registerEventListener: null _aidl_return", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;

    if (motionSensorEventListener == nullptr)
    {
        LOGF_ERR("%s: registerEventListener: null listener", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    android::sp<android::IBinder> listenerBinder = binderForEventListener(motionSensorEventListener);
    if (listenerBinder == nullptr)
    {
        LOGF_ERR("%s: registerEventListener: listener has no Binder identity", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    const auto existing = std::find_if(
        m_eventListeners.begin(),
        m_eventListeners.end(),
        [&](const EventListenerRegistration& registration) {
            return registration.binder == listenerBinder;
        });

    if (existing != m_eventListeners.end())
    {
        LOGF_INFO(
            "%s: registerEventListener ignored duplicate listener for sensor id=%d",
            logPrefix,
            m_id.value);
        return android::binder::Status::ok();
    }

    m_eventListeners.push_back(EventListenerRegistration{listenerBinder, motionSensorEventListener});
    *_aidl_return = true;

    LOGF_INFO(
        "%s: registered event listener for sensor id=%d totalListeners=%zu",
        logPrefix,
        m_id.value,
        m_eventListeners.size());
    return android::binder::Status::ok();
}

android::binder::Status MotionSensor::unregisterEventListener(
    const android::sp<IMotionSensorEventListener>& motionSensorEventListener,
    bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: unregisterEventListener: null _aidl_return", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;

    if (motionSensorEventListener == nullptr)
    {
        LOGF_ERR("%s: unregisterEventListener: null listener", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    android::sp<android::IBinder> listenerBinder = binderForEventListener(motionSensorEventListener);
    if (listenerBinder == nullptr)
    {
        LOGF_ERR("%s: unregisterEventListener: listener has no Binder identity", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    const auto newEnd = std::remove_if(
        m_eventListeners.begin(),
        m_eventListeners.end(),
        [&](const EventListenerRegistration& registration) {
            return registration.binder == listenerBinder;
        });

    if (newEnd == m_eventListeners.end())
    {
        LOGF_INFO(
            "%s: unregisterEventListener did not find listener for sensor id=%d",
            logPrefix,
            m_id.value);
        return android::binder::Status::ok();
    }

    m_eventListeners.erase(newEnd, m_eventListeners.end());
    *_aidl_return = true;

    LOGF_INFO(
        "%s: unregistered event listener for sensor id=%d totalListeners=%zu",
        logPrefix,
        m_id.value,
        m_eventListeners.size());
    return android::binder::Status::ok();
}

bool MotionSensor::injectMotionEvent()
{
    MotionEvent event{};
    event.mode = OperationalMode::MOTION;
    const auto eventTime = std::chrono::steady_clock::now().time_since_epoch();
    event.timestampMonotonicMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(eventTime).count();
    const int64_t eventTimestampNs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(eventTime).count();

    std::vector<android::sp<IMotionSensorEventListener>> listeners;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_state != State::STARTED)
        {
            LOGF_WARN(
                "%s: suppressing motion event for sensor id=%d because state=%d",
                logPrefix,
                m_id.value,
                static_cast<int32_t>(m_state));
            return false;
        }

        if (!m_isWithinActiveWindow)
        {
            LOGF_INFO(
                "%s: suppressing motion event outside active window for sensor id=%d "
                "configuredWindows=%zu",
                logPrefix,
                m_id.value,
                m_activeWindows.size());
            return false;
        }

        if (m_startConfig.operationalMode == OperationalMode::NO_MOTION)
        {
            // Physical motion restarts the contiguous inactivity period. The
            // AIDL listener receives only the configured active event mode.
            startNoMotionTimerLocked();
            LOGF_INFO(
                "%s: detected motion and restarted no-motion timer for sensor id=%d "
                "noMotionSeconds=%d",
                logPrefix,
                m_id.value,
                m_startConfig.noMotionSeconds);
            return true;
        }

        listeners.reserve(m_eventListeners.size());
        for (const EventListenerRegistration& registration : m_eventListeners)
        {
            listeners.push_back(registration.listener);
        }

        LOGF_INFO(
            "%s: delivering motion event for sensor id=%d listenerCount=%zu",
            logPrefix,
            m_id.value,
            listeners.size());
    }

    bool delivered = false;
    for (const android::sp<IMotionSensorEventListener>& listener : listeners)
    {
        if (listener == nullptr)
        {
            continue;
        }

        const android::binder::Status notifyStatus = listener->onEvent(event);
        if (!notifyStatus.isOk())
        {
            LOGF_WARN(
                "%s: motion event callback failed for sensor id=%d exception=%d",
                logPrefix,
                m_id.value,
                notifyStatus.exceptionCode());
            continue;
        }

        delivered = true;
    }

    if (delivered)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastEventInfo = LastEventInfo{};
        m_lastEventInfo->mode = event.mode;
        m_lastEventInfo->timestampNs = eventTimestampNs;
    }

    return true;
}

void MotionSensor::changeStateLocked(std::unique_lock<std::mutex>& lock, State newState)
{
    if (m_state == newState)
    {
        return;
    }

    const State oldState = m_state;
    m_state = newState;
    android::sp<IMotionSensorControllerListener> listener = m_controllerListener;

    LOGF_INFO(
        "%s: sensor id=%d state changed %d -> %d",
        logPrefix,
        m_id.value,
        static_cast<int32_t>(oldState),
        static_cast<int32_t>(newState));

    if (listener != nullptr)
    {
        lock.unlock();
        const android::binder::Status notifyStatus = listener->onStateChanged(oldState, newState);
        if (!notifyStatus.isOk())
        {
            LOGF_WARN(
                "%s: onStateChanged callback failed for sensor id=%d exception=%d",
                logPrefix,
                m_id.value,
                notifyStatus.exceptionCode());
        }
        lock.lock();
    }
}

void MotionSensor::startLifecycleTimerLocked(
    uint64_t lifecycleGeneration,
    int32_t activationDelaySeconds,
    int32_t activeStopSeconds)
{
    cancelLifecycleTimerLocked();
    m_lifecycleTimerCancelled = false;
    m_lifecycleTimerThread = std::thread(
        [this, lifecycleGeneration, activationDelaySeconds, activeStopSeconds]() {
            std::unique_lock<std::mutex> lock(m_mutex);
            const auto waitForCancellation = [this, &lock](int32_t seconds) -> bool {
                return m_lifecycleTimerCondition.wait_for(
                    lock,
                    std::chrono::seconds(seconds),
                    [this]() { return m_lifecycleTimerCancelled; });
            };

            if (activationDelaySeconds > 0 && waitForCancellation(activationDelaySeconds))
            {
                return;
            }

            if (m_lifecycleTimerCancelled || m_lifecycleGeneration != lifecycleGeneration)
            {
                return;
            }

            if (activationDelaySeconds > 0 && m_state == State::STARTING)
            {
                LOGF_INFO(
                    "%s: activating sensor id=%d after configured activation delay",
                    logPrefix,
                    m_id.value);
                changeStateLocked(lock, State::STARTED);
                updateActiveWindowLocked(lock);
                startNoMotionTimerLocked();
            }

            if (activeStopSeconds <= 0 || waitForCancellation(activeStopSeconds))
            {
                return;
            }

            if (m_lifecycleTimerCancelled ||
                m_lifecycleGeneration != lifecycleGeneration ||
                m_state != State::STARTED)
            {
                return;
            }

            LOGF_INFO(
                "%s: automatically stopping sensor id=%d after activeStopSeconds",
                logPrefix,
                m_id.value);
            cancelNoMotionTimerLocked();
            cancelActiveWindowLocked();
            changeStateLocked(lock, State::STOPPING);
            changeStateLocked(lock, State::STOPPED);
        });
}

void MotionSensor::cancelLifecycleTimerLocked()
{
    m_lifecycleTimerCancelled = true;
    m_lifecycleTimerCondition.notify_all();

    if (!m_lifecycleTimerThread.joinable())
    {
        return;
    }

    std::thread lifecycleTimer = std::move(m_lifecycleTimerThread);
    if (lifecycleTimer.get_id() == std::this_thread::get_id())
    {
        lifecycleTimer.detach();
        return;
    }

    m_mutex.unlock();
    lifecycleTimer.join();
    m_mutex.lock();
}

void MotionSensor::startNoMotionTimerLocked()
{
    cancelNoMotionTimerLocked();

    if (m_state != State::STARTED ||
        m_startConfig.operationalMode != OperationalMode::NO_MOTION ||
        m_startConfig.noMotionSeconds <= 0)
    {
        return;
    }

    m_noMotionTimerCancelled = false;
    const uint64_t timerGeneration = ++m_noMotionTimerGeneration;
    const int32_t noMotionSeconds = m_startConfig.noMotionSeconds;

    m_noMotionTimerThread = std::thread([this, timerGeneration, noMotionSeconds]() {
        std::vector<android::sp<IMotionSensorEventListener>> listeners;
        MotionEvent event{};
        int64_t eventTimestampNs = 0;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            const bool cancelled = m_noMotionTimerCondition.wait_for(
                lock,
                std::chrono::seconds(noMotionSeconds),
                [this, timerGeneration]() {
                    return m_noMotionTimerCancelled ||
                           m_noMotionTimerGeneration != timerGeneration;
                });
            if (cancelled ||
                m_state != State::STARTED ||
                m_startConfig.operationalMode != OperationalMode::NO_MOTION)
            {
                return;
            }

            if (!m_isWithinActiveWindow)
            {
                LOGF_INFO(
                    "%s: suppressing timed no-motion event outside active window for sensor id=%d "
                    "configuredWindows=%zu",
                    logPrefix,
                    m_id.value,
                    m_activeWindows.size());
                return;
            }

            event.mode = OperationalMode::NO_MOTION;
            const auto eventTime = std::chrono::steady_clock::now().time_since_epoch();
            event.timestampMonotonicMs =
                std::chrono::duration_cast<std::chrono::milliseconds>(eventTime).count();
            eventTimestampNs =
                std::chrono::duration_cast<std::chrono::nanoseconds>(eventTime).count();
            listeners.reserve(m_eventListeners.size());
            for (const EventListenerRegistration& registration : m_eventListeners)
            {
                listeners.push_back(registration.listener);
            }
        }

        LOGF_INFO(
            "%s: delivering timed no-motion event for sensor id=%d "
            "noMotionSeconds=%d listenerCount=%zu",
            logPrefix,
            m_id.value,
            noMotionSeconds,
            listeners.size());

        bool delivered = false;
        for (const android::sp<IMotionSensorEventListener>& listener : listeners)
        {
            if (listener == nullptr)
            {
                continue;
            }

            const android::binder::Status notifyStatus = listener->onEvent(event);
            if (!notifyStatus.isOk())
            {
                LOGF_WARN(
                    "%s: timed no-motion callback failed for sensor id=%d exception=%d",
                    logPrefix,
                    m_id.value,
                    notifyStatus.exceptionCode());
                continue;
            }

            delivered = true;
        }

        if (delivered)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lastEventInfo = LastEventInfo{};
            m_lastEventInfo->mode = event.mode;
            m_lastEventInfo->timestampNs = eventTimestampNs;
        }
    });
}

void MotionSensor::cancelNoMotionTimerLocked()
{
    m_noMotionTimerCancelled = true;
    m_noMotionTimerCondition.notify_all();

    if (!m_noMotionTimerThread.joinable())
    {
        return;
    }

    std::thread noMotionTimer = std::move(m_noMotionTimerThread);
    if (noMotionTimer.get_id() == std::this_thread::get_id())
    {
        noMotionTimer.detach();
        return;
    }

    m_mutex.unlock();
    noMotionTimer.join();
    m_mutex.lock();
}

void MotionSensor::invalidateLifecycleTimersLocked()
{
    ++m_lifecycleGeneration;
    cancelLifecycleTimerLocked();
    ++m_noMotionTimerGeneration;
    cancelNoMotionTimerLocked();
}

bool MotionSensor::controllerMatchesLocked(const android::sp<IMotionSensorController>& controller) const
{
    if (controller == nullptr || m_controller == nullptr)
    {
        return false;
    }

    const android::sp<android::IBinder> expected = binderForController(m_controller);
    const android::sp<android::IBinder> supplied = binderForController(controller);
    return expected != nullptr && expected == supplied;
}

bool MotionSensor::isWithinActiveWindowLocked(int32_t timeOfDaySeconds) const
{
    // No windows means 24-hour monitoring.
    if (m_activeWindows.empty())
    {
        return true;
    }

    for (const TimeWindow& window : m_activeWindows)
    {
        const int32_t start = window.startTimeOfDaySeconds;
        const int32_t end = window.endTimeOfDaySeconds;

        if (start == 0 && end == 0)
        {
            return true;
        }

        if ((start < end && timeOfDaySeconds >= start && timeOfDaySeconds <= end) ||
            (start > end && (timeOfDaySeconds >= start || timeOfDaySeconds <= end)))
        {
            return true;
        }
    }

    return false;
}

void MotionSensor::updateActiveWindowLocked(std::unique_lock<std::mutex>& lock)
{
    if (m_state != State::STARTED)
    {
        return;
    }

    const int32_t timeOfDaySeconds = currentLocalTimeOfDaySeconds();
    const bool active = timeOfDaySeconds >= 0 && isWithinActiveWindowLocked(timeOfDaySeconds);
    const bool enteredSession = active && !m_activeWindowSchedulerThread.joinable();
    const bool stateChanged = active != m_isWithinActiveWindow;

    m_isWithinActiveWindow = active;
    if (stateChanged || enteredSession)
    {
        notifyActiveWindowLocked(lock, active);
    }

    scheduleActiveWindowLocked();
}

void MotionSensor::scheduleActiveWindowLocked()
{
    cancelActiveWindowLocked();

    if (m_state != State::STARTED || m_activeWindows.empty())
    {
        return;
    }

    const int32_t timeOfDaySeconds = currentLocalTimeOfDaySeconds();
    if (timeOfDaySeconds < 0)
    {
        LOGF_WARN("%s: unable to schedule active-window boundary due to local-time lookup failure", logPrefix);
        return;
    }

    int32_t secondsUntilBoundary = secondsPerDay;
    for (const TimeWindow& window : m_activeWindows)
    {
        // Active windows include their end second, so the exit transition
        // occurs at the following second and wraps correctly at midnight.
        const int32_t boundaries[] = {
            window.startTimeOfDaySeconds,
            (window.endTimeOfDaySeconds + 1) % secondsPerDay,
        };
        for (const int32_t boundary : boundaries)
        {
            int32_t untilBoundary = boundary - timeOfDaySeconds;
            if (untilBoundary <= 0)
            {
                untilBoundary += secondsPerDay;
            }
            secondsUntilBoundary = std::min(secondsUntilBoundary, untilBoundary);
        }
    }

    m_activeWindowSchedulerCancelled = false;
    const uint64_t schedulerGeneration = ++m_activeWindowSchedulerGeneration;
    m_activeWindowSchedulerThread = std::thread(
        [this, schedulerGeneration, secondsUntilBoundary]() {
            std::unique_lock<std::mutex> lock(m_mutex);
            const bool cancelled = m_activeWindowSchedulerCondition.wait_for(
                lock,
                std::chrono::seconds(secondsUntilBoundary),
                [this, schedulerGeneration]() {
                    return m_activeWindowSchedulerCancelled ||
                           m_activeWindowSchedulerGeneration != schedulerGeneration;
                });
            if (cancelled ||
                m_activeWindowSchedulerGeneration != schedulerGeneration ||
                m_state != State::STARTED)
            {
                return;
            }

            updateActiveWindowLocked(lock);
        });
}

void MotionSensor::cancelActiveWindowLocked()
{
    m_activeWindowSchedulerCancelled = true;
    m_activeWindowSchedulerCondition.notify_all();

    if (!m_activeWindowSchedulerThread.joinable())
    {
        return;
    }

    std::thread scheduler = std::move(m_activeWindowSchedulerThread);
    if (scheduler.get_id() == std::this_thread::get_id())
    {
        scheduler.detach();
        return;
    }

    m_mutex.unlock();
    scheduler.join();
    m_mutex.lock();
}

void MotionSensor::notifyActiveWindowLocked(
    std::unique_lock<std::mutex>& lock,
    bool entered)
{
    if (m_state != State::STARTED)
    {
        return;
    }

    android::sp<IMotionSensorControllerListener> listener = m_controllerListener;
    if (listener == nullptr)
    {
        return;
    }

    lock.unlock();
    const android::binder::Status notifyStatus =
        entered ? listener->onActiveWindowEntered() : listener->onActiveWindowExited();
    if (!notifyStatus.isOk())
    {
        LOGF_WARN(
            "%s: active-window transition callback failed for sensor id=%d entered=%d exception=%d",
            logPrefix,
            m_id.value,
            entered,
            notifyStatus.exceptionCode());
    }
    else
    {
        LOGF_INFO(
            "%s: active-window transition callback delivered for sensor id=%d entered=%d",
            logPrefix,
            m_id.value,
            entered);
    }
    lock.lock();
}

void MotionSensor::releaseControllerLocked()
{
    invalidateLifecycleTimersLocked();
    cancelActiveWindowLocked();

    if (m_ownerBinder != nullptr)
    {
        const android::status_t unlinkStatus = m_ownerBinder->unlinkToDeath(this);
        if (unlinkStatus != android::NO_ERROR)
        {
            LOGF_WARN(
                "%s: unlinkToDeath returned status=%d for sensor id=%d",
                logPrefix,
                static_cast<int>(unlinkStatus),
                m_id.value);
        }
    }

    m_ownerBinder.clear();
    m_controller.clear();
    m_controllerListener.clear();
}

void MotionSensor::binderDied(const ::android::wp<::android::IBinder>& who)
{
    android::sp<android::IBinder> deadBinder = who.promote();

    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_ownerBinder == nullptr || (deadBinder != nullptr && deadBinder != m_ownerBinder))
    {
        LOGF_WARN("%s: controller death notification ignored", logPrefix);
        return;
    }

    LOGF_WARN(
        "%s: controller owner died; implicitly stopping and closing sensor id=%d",
        logPrefix,
        m_id.value);

    invalidateLifecycleTimersLocked();
    cancelActiveWindowLocked();

    if (m_state == State::STARTING || m_state == State::STARTED)
    {
        changeStateLocked(lock, State::STOPPING);
        changeStateLocked(lock, State::STOPPED);
    }
    else if (m_state != State::STOPPED)
    {
        changeStateLocked(lock, State::STOPPED);
    }

    releaseControllerLocked();
}

} // namespace com::rdk::hal::sensor::motion
