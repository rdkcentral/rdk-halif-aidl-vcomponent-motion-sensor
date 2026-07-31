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

namespace com::rdk::hal::sensor::motion
{

namespace
{
constexpr const char* componentName = "MotionSensor";

/**
 * @brief Return a consistent unsupported-operation Binder status for stub-only APIs.
 *
 * @param[in] action The API action being rejected.
 *
 * @return Unsupported-operation Binder status.
 */
android::binder::Status unsupportedStubOnlyStatus(const char* action)
{
    LOGF_WARN(
        "%s: %s called on stub-only implementation; operation is not supported",
        componentName,
        action);
    return android::binder::Status::fromExceptionCode(android::binder::Status::EX_UNSUPPORTED_OPERATION);
}
} // namespace

MotionSensor::MotionSensor(const IMotionSensor::Id& id)
    : m_id(id)
{
    LOGF_INFO(
        "%s: created stub-only motion sensor placeholder id=%d",
        componentName,
        static_cast<int>(m_id.value));
}

android::binder::Status MotionSensor::getCapabilities(Capabilities* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getCapabilities: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = Capabilities{};
    return unsupportedStubOnlyStatus("getCapabilities");
}

android::binder::Status MotionSensor::getState(State* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getState: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = State::STOPPED;
    return unsupportedStubOnlyStatus("getState");
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

    return unsupportedStubOnlyStatus("open");
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

    return unsupportedStubOnlyStatus("close");
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

    return unsupportedStubOnlyStatus("registerEventListener");
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

    return unsupportedStubOnlyStatus("unregisterEventListener");
}

void MotionSensor::changeStateLocked(std::unique_lock<std::mutex>& lock, State newState)
{
    (void)lock;
    (void)newState;
    LOGF_WARN("%s: changeStateLocked ignored because the implementation is stub-only", componentName);
}

bool MotionSensor::controllerMatchesLocked(const android::sp<IMotionSensorController>& controller) const
{
    (void)controller;
    return false;
}

void MotionSensor::binderDied(const ::android::wp<::android::IBinder>& who)
{
    (void)who;
    LOGF_WARN("%s: binderDied ignored because the implementation is stub-only", componentName);
}

} // namespace com::rdk::hal::sensor::motion
