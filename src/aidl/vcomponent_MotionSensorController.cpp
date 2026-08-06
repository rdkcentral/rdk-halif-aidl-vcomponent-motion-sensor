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

#include <com/rdk/hal/sensor/motion/OperationalMode.h>

namespace com::rdk::hal::sensor::motion
{

namespace
{
constexpr const char* componentName = "MotionSensorController";
constexpr int32_t secondsPerDay = 24 * 60 * 60;

bool isValidOperationalMode(OperationalMode mode)
{
    return mode == OperationalMode::MOTION || mode == OperationalMode::NO_MOTION;
}

bool isValidDurationSeconds(int32_t value)
{
    return value >= 0 && value <= secondsPerDay;
}

bool isValidWindow(const TimeWindow& window)
{
    return window.startTimeOfDaySeconds >= 0 &&
           window.startTimeOfDaySeconds < secondsPerDay &&
           window.endTimeOfDaySeconds >= 0 &&
           window.endTimeOfDaySeconds < secondsPerDay;
}

android::binder::Status nullPointerStatus(const char* action)
{
    LOGF_ERR("%s: %s: null _aidl_return", componentName, action);
    return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
}
} // namespace

MotionSensorController::MotionSensorController(
    MotionSensor* parent,
    const android::sp<IMotionSensorControllerListener>& listener)
    : m_parent(parent)
    , m_listener(listener)
{
    LOGF_INFO("%s: created controller", componentName);
}

android::binder::Status MotionSensorController::start(const StartConfig& config)
{
    if (m_parent == nullptr)
    {
        LOGF_ERR("%s: start: missing parent sensor", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (!isValidOperationalMode(config.operationalMode) ||
        !isValidDurationSeconds(config.noMotionSeconds) ||
        !isValidDurationSeconds(config.activeStartSeconds) ||
        !isValidDurationSeconds(config.activeStopSeconds))
    {
        LOGF_WARN(
            "%s: start rejected invalid config mode=%d noMotion=%d activeStart=%d activeStop=%d",
            componentName,
            static_cast<int32_t>(config.operationalMode),
            config.noMotionSeconds,
            config.activeStartSeconds,
            config.activeStopSeconds);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }

    std::unique_lock<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this)
    {
        LOGF_WARN("%s: start rejected because controller is not current owner", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (m_parent->m_state != State::STOPPED)
    {
        LOGF_WARN(
            "%s: start rejected because sensor state=%d",
            componentName,
            static_cast<int32_t>(m_parent->m_state));
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    m_parent->m_startConfig = config;
    m_parent->m_lastEventInfo.reset();

    m_parent->changeStateLocked(lock, State::STARTING);
    m_parent->changeStateLocked(lock, State::STARTED);

    LOGF_INFO("%s: start completed", componentName);
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::stop()
{
    if (m_parent == nullptr)
    {
        LOGF_ERR("%s: stop: missing parent sensor", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::unique_lock<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this)
    {
        LOGF_WARN("%s: stop rejected because controller is not current owner", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (m_parent->m_state != State::STARTED)
    {
        LOGF_WARN(
            "%s: stop rejected because sensor state=%d",
            componentName,
            static_cast<int32_t>(m_parent->m_state));
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    m_parent->changeStateLocked(lock, State::STOPPING);
    m_parent->changeStateLocked(lock, State::STOPPED);

    LOGF_INFO("%s: stop completed", componentName);
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::getStartConfig(StartConfig* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        return nullPointerStatus("getStartConfig");
    }

    if (m_parent == nullptr)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this ||
        m_parent->m_state == State::STOPPED ||
        m_parent->m_state == State::ERROR)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    *_aidl_return = m_parent->m_startConfig;
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::getLastEventInfo(std::optional<LastEventInfo>* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        return nullPointerStatus("getLastEventInfo");
    }

    if (m_parent == nullptr)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this ||
        m_parent->m_state == State::STOPPED ||
        m_parent->m_state == State::ERROR)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    *_aidl_return = m_parent->m_lastEventInfo;
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::getSensitivity(int32_t* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        return nullPointerStatus("getSensitivity");
    }

    if (m_parent == nullptr)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    *_aidl_return = m_parent->m_sensitivity;
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::setSensitivity(int32_t sensitivity, bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        return nullPointerStatus("setSensitivity");
    }

    *_aidl_return = false;

    if (m_parent == nullptr)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this || m_parent->m_state != State::STOPPED)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (m_parent->m_capabilities.minSensitivity == 0 &&
        m_parent->m_capabilities.maxSensitivity == 0)
    {
        return android::binder::Status::ok();
    }

    if (sensitivity < m_parent->m_capabilities.minSensitivity ||
        sensitivity > m_parent->m_capabilities.maxSensitivity)
    {
        LOGF_WARN(
            "%s: setSensitivity rejected out-of-range value=%d range=[%d,%d]",
            componentName,
            sensitivity,
            m_parent->m_capabilities.minSensitivity,
            m_parent->m_capabilities.maxSensitivity);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }

    m_parent->m_sensitivity = sensitivity;
    *_aidl_return = true;
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::setAutonomousDuringDeepSleep(bool enabled, bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        return nullPointerStatus("setAutonomousDuringDeepSleep");
    }

    *_aidl_return = false;

    if (m_parent == nullptr)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this || m_parent->m_state != State::STOPPED)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (!m_parent->m_capabilities.supportsDeepSleepAutonomy)
    {
        return android::binder::Status::ok();
    }

    m_parent->m_autonomousDuringDeepSleepEnabled = enabled;
    *_aidl_return = true;
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::isAutonomousDuringDeepSleepEnabled(bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        return nullPointerStatus("isAutonomousDuringDeepSleepEnabled");
    }

    if (m_parent == nullptr)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    *_aidl_return = m_parent->m_autonomousDuringDeepSleepEnabled &&
                    m_parent->m_capabilities.supportsDeepSleepAutonomy;
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::setActiveWindows(
    const std::vector<TimeWindow>& windows,
    bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        return nullPointerStatus("setActiveWindows");
    }

    *_aidl_return = false;

    if (m_parent == nullptr)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    for (const auto& window : windows)
    {
        if (!isValidWindow(window))
        {
            LOGF_WARN(
                "%s: setActiveWindows rejected invalid window start=%d end=%d",
                componentName,
                window.startTimeOfDaySeconds,
                window.endTimeOfDaySeconds);
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
        }
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this || m_parent->m_state != State::STOPPED)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    m_parent->m_activeWindows = windows;
    *_aidl_return = true;
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::getActiveWindows(std::vector<TimeWindow>* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        return nullPointerStatus("getActiveWindows");
    }

    if (m_parent == nullptr)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    *_aidl_return = m_parent->m_activeWindows;
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::clearActiveWindows(bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        return nullPointerStatus("clearActiveWindows");
    }

    *_aidl_return = false;

    if (m_parent == nullptr)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this || m_parent->m_state != State::STOPPED)
    {
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    m_parent->m_activeWindows.clear();
    *_aidl_return = true;
    return android::binder::Status::ok();
}

} // namespace com::rdk::hal::sensor::motion
