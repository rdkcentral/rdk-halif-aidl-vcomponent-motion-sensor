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

#include <binder/IInterface.h>

#include <algorithm>

namespace com::rdk::hal::sensor::motion
{

namespace
{
constexpr const char* componentName = "MotionSensor";

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
    , m_startConfig(defaultStartConfig)
    , m_activeWindows(defaultActiveWindows)
{
    LOGF_INFO(
        "%s: created motion sensor id=%d sensitivity=%d",
        componentName,
        static_cast<int>(m_id.value),
        static_cast<int>(m_sensitivity));
}

android::binder::Status MotionSensor::getCapabilities(Capabilities* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getCapabilities: null _aidl_return", componentName);
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
        LOGF_ERR("%s: getState: null _aidl_return", componentName);
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
        LOGF_ERR("%s: open: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = nullptr;

    if (listener == nullptr)
    {
        LOGF_ERR("%s: open: null listener", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    android::sp<android::IBinder> ownerBinder = binderForControllerListener(listener);
    if (ownerBinder == nullptr)
    {
        LOGF_ERR("%s: open: listener has no Binder identity", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_controller != nullptr)
    {
        LOGF_WARN("%s: open rejected because sensor id=%d is already open", componentName, m_id.value);
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
            componentName,
            m_id.value,
            static_cast<int>(linkStatus));
    }

    LOGF_INFO("%s: opened sensor id=%d", componentName, m_id.value);
    return android::binder::Status::ok();
}

android::binder::Status MotionSensor::close(
    const android::sp<IMotionSensorController>& controller,
    bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: close: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;

    if (controller == nullptr)
    {
        LOGF_ERR("%s: close: null controller", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_state != State::STOPPED && m_state != State::ERROR)
    {
        LOGF_WARN(
            "%s: close rejected for sensor id=%d while state=%d",
            componentName,
            m_id.value,
            static_cast<int32_t>(m_state));
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (!controllerMatchesLocked(controller))
    {
        LOGF_WARN("%s: close supplied non-owner controller for sensor id=%d", componentName, m_id.value);
        return android::binder::Status::ok();
    }

    if (m_state == State::ERROR)
    {
        changeStateLocked(lock, State::STOPPED);
    }

    releaseControllerLocked();
    *_aidl_return = true;

    LOGF_INFO("%s: closed sensor id=%d", componentName, m_id.value);
    return android::binder::Status::ok();
}

android::binder::Status MotionSensor::registerEventListener(
    const android::sp<IMotionSensorEventListener>& motionSensorEventListener,
    bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: registerEventListener: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;

    if (motionSensorEventListener == nullptr)
    {
        LOGF_ERR("%s: registerEventListener: null listener", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    android::sp<android::IBinder> listenerBinder = binderForEventListener(motionSensorEventListener);
    if (listenerBinder == nullptr)
    {
        LOGF_ERR("%s: registerEventListener: listener has no Binder identity", componentName);
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
            componentName,
            m_id.value);
        return android::binder::Status::ok();
    }

    m_eventListeners.push_back(EventListenerRegistration{listenerBinder, motionSensorEventListener});
    *_aidl_return = true;

    LOGF_INFO(
        "%s: registered event listener for sensor id=%d totalListeners=%zu",
        componentName,
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
        LOGF_ERR("%s: unregisterEventListener: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;

    if (motionSensorEventListener == nullptr)
    {
        LOGF_ERR("%s: unregisterEventListener: null listener", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    android::sp<android::IBinder> listenerBinder = binderForEventListener(motionSensorEventListener);
    if (listenerBinder == nullptr)
    {
        LOGF_ERR("%s: unregisterEventListener: listener has no Binder identity", componentName);
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
            componentName,
            m_id.value);
        return android::binder::Status::ok();
    }

    m_eventListeners.erase(newEnd, m_eventListeners.end());
    *_aidl_return = true;

    LOGF_INFO(
        "%s: unregistered event listener for sensor id=%d totalListeners=%zu",
        componentName,
        m_id.value,
        m_eventListeners.size());
    return android::binder::Status::ok();
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
        componentName,
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
                componentName,
                m_id.value,
                notifyStatus.exceptionCode());
        }
        lock.lock();
    }
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

void MotionSensor::releaseControllerLocked()
{
    if (m_ownerBinder != nullptr)
    {
        const android::status_t unlinkStatus = m_ownerBinder->unlinkToDeath(this);
        if (unlinkStatus != android::NO_ERROR)
        {
            LOGF_WARN(
                "%s: unlinkToDeath returned status=%d for sensor id=%d",
                componentName,
                static_cast<int>(unlinkStatus),
                m_id.value);
        }
    }

    m_ownerBinder.clear();
    m_controller.clear();
    m_controllerListener.clear();
    m_lastEventInfo.reset();
}

void MotionSensor::binderDied(const ::android::wp<::android::IBinder>& who)
{
    android::sp<android::IBinder> deadBinder = who.promote();

    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_ownerBinder == nullptr || (deadBinder != nullptr && deadBinder != m_ownerBinder))
    {
        return;
    }

    LOGF_WARN(
        "%s: controller owner died; implicitly stopping and closing sensor id=%d",
        componentName,
        m_id.value);

    if (m_state != State::STOPPED)
    {
        changeStateLocked(lock, State::STOPPED);
    }

    releaseControllerLocked();
}

} // namespace com::rdk::hal::sensor::motion
