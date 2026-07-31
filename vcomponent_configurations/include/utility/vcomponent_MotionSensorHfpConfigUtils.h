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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vcomponent::utility
{

/**
 * @brief Factory-default start configuration for a motion sensor.
 */
struct MotionSensorDefaultStartConfig
{
    std::string operationalMode{"MOTION"};
    int32_t noMotionSeconds{5};
    int32_t activeStartSeconds{0};
    int32_t activeStopSeconds{0};
};

/**
 * @brief Daily active window configuration for motion sensor reporting.
 */
struct MotionSensorActiveWindow
{
    int32_t startTimeOfDaySeconds{0};
    int32_t endTimeOfDaySeconds{0};
};

/**
 * @brief Minimal per-sensor HFP configuration model.
 */
struct MotionSensorConfig
{
    int32_t id{0};
    std::string sensorName{"PIR-Front-1"};
    int32_t minSensitivity{1};
    int32_t maxSensitivity{10};
    bool supportsDeepSleepAutonomy{true};
    MotionSensorDefaultStartConfig defaultStartConfig{};
    std::vector<MotionSensorActiveWindow> activeWindows{};
};

/**
 * @brief Top-level motion sensor HFP configuration model.
 */
struct MotionSensorHfpConfig
{
    std::string interfaceVersion{"current"};
    std::vector<MotionSensorConfig> sensors{};
};

/**
 * @brief Load motion sensor HFP configuration from a YAML file.
 *
 * @param[in] path YAML file path.
 * @param[out] outConfig Output config. Must not be nullptr.
 * @param[out] outError Optional error string.
 *
 * @return true on success, false on error.
 */
bool loadMotionSensorHfpConfigFromYaml(
    const std::string& path,
    MotionSensorHfpConfig* outConfig,
    std::string* outError);

} // namespace vcomponent::utility
