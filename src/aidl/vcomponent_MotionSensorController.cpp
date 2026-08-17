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
constexpr const char* logPrefix = "[VDEVICE_MOTION]<MotionSensorController>";
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
    LOGF_ERR("%s: %s: null _aidl_return", logPrefix, action);
    return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
}
} // namespace

MotionSensorController::MotionSensorController(
    MotionSensor* parent,
    const android::sp<IMotionSensorControllerListener>& /* listener */)
    : m_parent(parent)
{
    LOGF_INFO("%s: created controller", logPrefix);
}

android::binder::Status MotionSensorController::start(const StartConfig& config)
{
    if (m_parent == nullptr)
    {
        LOGF_ERR("%s: start: missing parent sensor", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (!isValidOperationalMode(config.operationalMode) ||
        !isValidDurationSeconds(config.noMotionSeconds) ||
        !isValidDurationSeconds(config.activeStartSeconds) ||
        !isValidDurationSeconds(config.activeStopSeconds))
    {
        LOGF_WARN(
            "%s: start rejected invalid config mode=%d noMotion=%d activeStart=%d activeStop=%d",
            logPrefix,
            static_cast<int32_t>(config.operationalMode),
            config.noMotionSeconds,
            config.activeStartSeconds,
            config.activeStopSeconds);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }

    std::unique_lock<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this)
    {
        LOGF_WARN("%s: start rejected because controller is not current owner", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (m_parent->m_state != State::STOPPED)
    {
        LOGF_WARN(
            "%s: start rejected because sensor state=%d",
            logPrefix,
            static_cast<int32_t>(m_parent->m_state));
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    m_parent->m_startConfig = config;
    m_parent->m_lastEventInfo.reset();
    const uint64_t lifecycleGeneration = ++m_parent->m_lifecycleGeneration;

    m_parent->changeStateLocked(lock, State::STARTING);

    if (config.activeStartSeconds > 0)
    {
        LOGF_INFO(
            "%s: delaying sensor activation by %d seconds",
            logPrefix,
            config.activeStartSeconds);
        m_parent->startLifecycleTimerLocked(
            lifecycleGeneration,
            config.activeStartSeconds,
            config.activeStopSeconds);
    }
    else
    {
        m_parent->changeStateLocked(lock, State::STARTED);

        if (config.activeStopSeconds > 0)
        {
            m_parent->startLifecycleTimerLocked(
                lifecycleGeneration,
                0,
                config.activeStopSeconds);
        }
    }

    LOGF_INFO("%s: start completed", logPrefix);
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorController::stop()
{
    if (m_parent == nullptr)
    {
        LOGF_ERR("%s: stop: missing parent sensor", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::unique_lock<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this)
    {
        LOGF_WARN("%s: stop rejected because controller is not current owner", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (m_parent->m_state != State::STARTING && m_parent->m_state != State::STARTED)
    {
        LOGF_WARN(
            "%s: stop rejected because sensor state=%d",
            logPrefix,
            static_cast<int32_t>(m_parent->m_state));
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    m_parent->invalidateLifecycleTimersLocked();
    m_parent->changeStateLocked(lock, State::STOPPING);
    m_parent->changeStateLocked(lock, State::STOPPED);

    LOGF_INFO("%s: stop completed", logPrefix);
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
        LOGF_ERR("%s: getStartConfig: missing parent sensor", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this ||
        m_parent->m_state == State::STOPPED ||
        m_parent->m_state == State::ERROR)
    {
        LOGF_WARN(
            "%s: getStartConfig rejected because controller is inactive or not the current owner (state=%d)",
            logPrefix,
            static_cast<int32_t>(m_parent->m_state));
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
        LOGF_ERR("%s: getLastEventInfo: missing parent sensor", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this ||
        m_parent->m_state == State::STOPPED ||
        m_parent->m_state == State::ERROR)
    {
        LOGF_WARN(
            "%s: getLastEventInfo rejected because controller is inactive or not the current owner (state=%d)",
            logPrefix,
            static_cast<int32_t>(m_parent->m_state));
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
        LOGF_ERR("%s: getSensitivity: missing parent sensor", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this)
    {
        LOGF_WARN("%s: getSensitivity rejected because controller is not the current owner", logPrefix);
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
        LOGF_ERR("%s: setSensitivity: missing parent sensor", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this || m_parent->m_state != State::STOPPED)
    {
        LOGF_WARN(
            "%s: setSensitivity rejected because controller is not the current owner or sensor state=%d",
            logPrefix,
            static_cast<int32_t>(m_parent->m_state));
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (m_parent->m_capabilities.minSensitivity == 0 &&
        m_parent->m_capabilities.maxSensitivity == 0)
    {
        LOGF_WARN("%s: setSensitivity rejected because sensitivity control is unsupported", logPrefix);
        return android::binder::Status::ok();
    }

    if (sensitivity < m_parent->m_capabilities.minSensitivity ||
        sensitivity > m_parent->m_capabilities.maxSensitivity)
    {
        LOGF_WARN(
            "%s: setSensitivity rejected out-of-range value=%d range=[%d,%d]",
            logPrefix,
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
        LOGF_ERR("%s: setAutonomousDuringDeepSleep: missing parent sensor", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this || m_parent->m_state != State::STOPPED)
    {
        LOGF_WARN(
            "%s: setAutonomousDuringDeepSleep rejected because controller is not the current owner or sensor state=%d",
            logPrefix,
            static_cast<int32_t>(m_parent->m_state));
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    if (!m_parent->m_capabilities.supportsDeepSleepAutonomy)
    {
        LOGF_WARN(
            "%s: setAutonomousDuringDeepSleep rejected because deep-sleep autonomy is unsupported",
            logPrefix);
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
        LOGF_ERR(
            "%s: isAutonomousDuringDeepSleepEnabled: missing parent sensor",
            logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this)
    {
        LOGF_WARN(
            "%s: isAutonomousDuringDeepSleepEnabled rejected because controller is not the current owner",
            logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    *_aidl_return = m_parent->m_autonomousDuringDeepSleepEnabled;
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
        LOGF_ERR("%s: setActiveWindows: missing parent sensor", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    for (const auto& window : windows)
    {
        if (!isValidWindow(window))
        {
            LOGF_WARN(
                "%s: setActiveWindows rejected invalid window start=%d end=%d",
                logPrefix,
                window.startTimeOfDaySeconds,
                window.endTimeOfDaySeconds);
            return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
        }
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this || m_parent->m_state != State::STOPPED)
    {
        LOGF_WARN(
            "%s: setActiveWindows rejected because controller is not the current owner or sensor state=%d",
            logPrefix,
            static_cast<int32_t>(m_parent->m_state));
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
        LOGF_ERR("%s: getActiveWindows: missing parent sensor", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this)
    {
        LOGF_WARN("%s: getActiveWindows rejected because controller is not the current owner", logPrefix);
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
        LOGF_ERR("%s: clearActiveWindows: missing parent sensor", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    std::lock_guard<std::mutex> lock(m_parent->m_mutex);
    if (m_parent->m_controller.get() != this || m_parent->m_state != State::STOPPED)
    {
        LOGF_WARN(
            "%s: clearActiveWindows rejected because controller is not the current owner or sensor state=%d",
            logPrefix,
            static_cast<int32_t>(m_parent->m_state));
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    m_parent->m_activeWindows.clear();
    *_aidl_return = true;
    return android::binder::Status::ok();
}

} // namespace com::rdk::hal::sensor::motion
