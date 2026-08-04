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

#include "aidl/vcomponent_MotionSensorController.h"

#include "aidl/vcomponent_MotionSensor.h"
#include "common/logger.h"

namespace com::rdk::hal::sensor::motion
{

namespace
{
constexpr const char* componentName = "MotionSensorController";

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

MotionSensorController::MotionSensorController(
    MotionSensor* parent,
    const android::sp<IMotionSensorControllerListener>& listener)
    : m_parent(parent)
    , m_listener(listener)
{
    LOGF_INFO("%s: created stub-only controller placeholder", componentName);
}

android::binder::Status MotionSensorController::start(const StartConfig& config)
{
    (void)config;
    return unsupportedStubOnlyStatus("start");
}

android::binder::Status MotionSensorController::stop()
{
    return unsupportedStubOnlyStatus("stop");
}

android::binder::Status MotionSensorController::getStartConfig(StartConfig* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getStartConfig: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = StartConfig{};
    return unsupportedStubOnlyStatus("getStartConfig");
}

android::binder::Status MotionSensorController::getLastEventInfo(std::optional<LastEventInfo>* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getLastEventInfo: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = std::nullopt;
    return unsupportedStubOnlyStatus("getLastEventInfo");
}

android::binder::Status MotionSensorController::getSensitivity(int32_t* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getSensitivity: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = 0;
    return unsupportedStubOnlyStatus("getSensitivity");
}

android::binder::Status MotionSensorController::setSensitivity(int32_t sensitivity, bool* _aidl_return)
{
    (void)sensitivity;

    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: setSensitivity: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;
    return unsupportedStubOnlyStatus("setSensitivity");
}

android::binder::Status MotionSensorController::setAutonomousDuringDeepSleep(bool enabled, bool* _aidl_return)
{
    (void)enabled;

    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: setAutonomousDuringDeepSleep: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;
    return unsupportedStubOnlyStatus("setAutonomousDuringDeepSleep");
}

android::binder::Status MotionSensorController::isAutonomousDuringDeepSleepEnabled(bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: isAutonomousDuringDeepSleepEnabled: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;
    return unsupportedStubOnlyStatus("isAutonomousDuringDeepSleepEnabled");
}

android::binder::Status MotionSensorController::setActiveWindows(const std::vector<TimeWindow>& windows, bool* _aidl_return)
{
    (void)windows;

    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: setActiveWindows: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;
    return unsupportedStubOnlyStatus("setActiveWindows");
}

android::binder::Status MotionSensorController::getActiveWindows(std::vector<TimeWindow>* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getActiveWindows: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    _aidl_return->clear();
    return unsupportedStubOnlyStatus("getActiveWindows");
}

android::binder::Status MotionSensorController::clearActiveWindows(bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: clearActiveWindows: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;
    return unsupportedStubOnlyStatus("clearActiveWindows");
}

} // namespace com::rdk::hal::sensor::motion
