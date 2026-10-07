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

#include "aidl/vcomponent_MotionSensorManager.h"

#include "aidl/vcomponent_MotionSensor.h"
#include "common/logger.h"

#include <com/rdk/hal/sensor/motion/OperationalMode.h>

#include <utils/String16.h>

namespace com::rdk::hal::sensor::motion
{

namespace
{
constexpr const char* logPrefix = "[VDEVICE_MOTION]<MotionSensorManager>";

std::mutex& configurationMutex()
{
    static std::mutex mutex;
    return mutex;
}

vcomponent::utility::MotionSensorHfpConfig& configuredConfiguration()
{
    static vcomponent::utility::MotionSensorHfpConfig configuration;
    return configuration;
}

OperationalMode operationalModeFromString(const std::string& value)
{
    if (value == "NO_MOTION")
    {
        return OperationalMode::NO_MOTION;
    }

    return OperationalMode::MOTION;
}

StartConfig toStartConfig(const vcomponent::utility::MotionSensorDefaultStartConfig& config)
{
    StartConfig startConfig;
    startConfig.operationalMode = operationalModeFromString(config.operationalMode);
    startConfig.noMotionSeconds = config.noMotionSeconds;
    startConfig.activeStartSeconds = config.activeStartSeconds;
    startConfig.activeStopSeconds = config.activeStopSeconds;
    return startConfig;
}

std::vector<TimeWindow> toTimeWindows(
    const std::vector<vcomponent::utility::MotionSensorActiveWindow>& configuredWindows)
{
    std::vector<TimeWindow> windows;
    windows.reserve(configuredWindows.size());

    for (const auto& configuredWindow : configuredWindows)
    {
        TimeWindow window;
        window.startTimeOfDaySeconds = configuredWindow.startTimeOfDaySeconds;
        window.endTimeOfDaySeconds = configuredWindow.endTimeOfDaySeconds;
        windows.push_back(window);
    }

    return windows;
}

Capabilities toCapabilities(const vcomponent::utility::MotionSensorConfig& sensor)
{
    Capabilities capabilities;
    capabilities.sensorName = android::String16(sensor.sensorName.c_str());
    capabilities.minSensitivity = sensor.minSensitivity;
    capabilities.maxSensitivity = sensor.maxSensitivity;
    capabilities.supportsDeepSleepAutonomy = sensor.supportsDeepSleepAutonomy;
    return capabilities;
}
} // namespace

void MotionSensorManager::setConfiguration(
    const vcomponent::utility::MotionSensorHfpConfig& configuration)
{
    std::lock_guard<std::mutex> lock(configurationMutex());
    configuredConfiguration() = configuration;
}

MotionSensorManager::MotionSensorManager()
{
    vcomponent::utility::MotionSensorHfpConfig configuration;
    {
        std::lock_guard<std::mutex> lock(configurationMutex());
        configuration = configuredConfiguration();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_sensors.reserve(configuration.sensors.size());

    for (const auto& sensorConfig : configuration.sensors)
    {
        IMotionSensor::Id sensorId;
        sensorId.value = sensorConfig.id;

        m_sensors.push_back(
            android::sp<MotionSensor>::make(
                sensorId,
                toCapabilities(sensorConfig),
                toStartConfig(sensorConfig.defaultStartConfig),
                toTimeWindows(sensorConfig.activeWindows)));

        LOGF_INFO(
            "%s: configured motion sensor id=%d name=%s",
            logPrefix,
            sensorConfig.id,
            sensorConfig.sensorName.c_str());
    }

    LOGF_INFO(
        "%s: initialized with %zu configured motion sensor(s)",
        logPrefix,
        m_sensors.size());
}

MotionSensorManager::~MotionSensorManager() = default;

android::binder::Status MotionSensorManager::getMotionSensorIds(
    std::optional<std::vector<std::optional<IMotionSensor::Id>>>* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getMotionSensorIds: null _aidl_return", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::optional<IMotionSensor::Id>> ids;
    ids.reserve(m_sensors.size());
    for (const auto& sensor : m_sensors)
    {
        ids.emplace_back(sensor->id());
    }

    *_aidl_return = std::move(ids);
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorManager::getMotionSensor(
    const IMotionSensor::Id& motionSensorId,
    android::sp<IMotionSensor>* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getMotionSensor: null _aidl_return", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = nullptr;

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& sensor : m_sensors)
    {
        if (sensor != nullptr && sensor->id().value == motionSensorId.value)
        {
            *_aidl_return = sensor;
            LOGF_INFO(
                "%s: getMotionSensor returning sensor id=%d",
                logPrefix,
                static_cast<int>(motionSensorId.value));
            return android::binder::Status::ok();
        }
    }

    LOGF_WARN(
        "%s: getMotionSensor requested unknown id=%d",
        logPrefix,
        static_cast<int>(motionSensorId.value));
    return android::binder::Status::fromExceptionCode(
        android::binder::Status::EX_ILLEGAL_ARGUMENT);
}

} // namespace com::rdk::hal::sensor::motion
