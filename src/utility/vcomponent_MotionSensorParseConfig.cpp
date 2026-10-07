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

#include "utility/vcomponent_MotionSensorParseConfig.h"

#include "common/logger.h"

#include <ut_kvp_profile.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace vcomponent::utility
{
namespace
{

constexpr const char* MOTION_SENSOR_ROOT = "sensor.motion";
constexpr const char* MOTION_SENSOR_LIST = "sensor.motion";
constexpr const char* logPrefix = "[VDEVICE_MOTION]<MotionSensorParseConfig>";
constexpr size_t KVP_BUFFER_SIZE = UT_KVP_MAX_ELEMENT_SIZE;

void setError(std::string* outError, const std::string& message)
{
    if (outError != nullptr)
    {
        *outError = message;
    }
}

bool fieldPresent(ut_kvp_instance_t* instance, const std::string& fieldKey)
{
    return instance != nullptr && ut_kvp_fieldPresent(instance, fieldKey.c_str());
}

bool readStringField(
    ut_kvp_instance_t* instance,
    const std::string& fieldKey,
    std::string* outValue,
    std::string* outError,
    bool required)
{
    if (outValue == nullptr)
    {
        setError(outError, "internal parser error: null string output for key " + fieldKey);
        return false;
    }

    if (!fieldPresent(instance, fieldKey))
    {
        if (required)
        {
            setError(outError, "required Motion Sensor HFP field is missing: " + fieldKey);
            return false;
        }
        return true;
    }

    char resultKvp[KVP_BUFFER_SIZE] = {0};
    const ut_kvp_status_t status =
        ut_kvp_getStringField(instance, fieldKey.c_str(), resultKvp, sizeof(resultKvp));
    if (status != UT_KVP_STATUS_SUCCESS)
    {
        setError(outError, "failed to read Motion Sensor HFP string field: " + fieldKey);
        return false;
    }

    *outValue = resultKvp;
    return true;
}

bool parseInt32(const std::string& value, int32_t* outValue)
{
    if (outValue == nullptr || value.empty())
    {
        return false;
    }

    errno = 0;
    char* endPtr = nullptr;
    const long parsed = std::strtol(value.c_str(), &endPtr, 0);
    if (errno != 0 || endPtr == value.c_str() || (endPtr != nullptr && *endPtr != '\0') ||
        parsed < std::numeric_limits<int32_t>::min() ||
        parsed > std::numeric_limits<int32_t>::max())
    {
        return false;
    }

    *outValue = static_cast<int32_t>(parsed);
    return true;
}

bool parseBool(const std::string& value, bool* outValue)
{
    if (outValue == nullptr)
    {
        return false;
    }

    if (value == "true" || value == "True" || value == "TRUE" || value == "1")
    {
        *outValue = true;
        return true;
    }

    if (value == "false" || value == "False" || value == "FALSE" || value == "0")
    {
        *outValue = false;
        return true;
    }

    return false;
}

bool readInt32Field(
    ut_kvp_instance_t* instance,
    const std::string& fieldKey,
    int32_t* outValue,
    std::string* outError,
    bool required)
{
    std::string value;
    if (!readStringField(instance, fieldKey, &value, outError, required))
    {
        return false;
    }

    if (!fieldPresent(instance, fieldKey) && !required)
    {
        return true;
    }

    if (!parseInt32(value, outValue))
    {
        setError(
            outError,
            "failed to parse Motion Sensor HFP integer field: " + fieldKey + " value=" + value);
        return false;
    }

    return true;
}

bool readBoolField(
    ut_kvp_instance_t* instance,
    const std::string& fieldKey,
    bool* outValue,
    std::string* outError,
    bool required)
{
    std::string value;
    if (!readStringField(instance, fieldKey, &value, outError, required))
    {
        return false;
    }

    if (!fieldPresent(instance, fieldKey) && !required)
    {
        return true;
    }

    if (!parseBool(value, outValue))
    {
        setError(
            outError,
            "failed to parse Motion Sensor HFP boolean field: " + fieldKey + " value=" + value);
        return false;
    }

    return true;
}

bool getMotionSensorInfo(
    ut_kvp_instance_t* instance,
    uint32_t index,
    MotionSensorConfig& sensor,
    std::string* outError)
{
    sensor = MotionSensorConfig{};

    const std::string prefix =
        std::string(MOTION_SENSOR_LIST) + "." + std::to_string(index) + ".";

    if (!readInt32Field(instance, prefix + "id", &sensor.id, outError, true) ||
        !readStringField(instance, prefix + "sensorName", &sensor.sensorName, outError, true) ||
        !readInt32Field(
            instance,
            prefix + "sensitivity_range.minSensitivity",
            &sensor.minSensitivity,
            outError,
            true) ||
        !readInt32Field(
            instance,
            prefix + "sensitivity_range.maxSensitivity",
            &sensor.maxSensitivity,
            outError,
            true) ||
        !readBoolField(
            instance,
            prefix + "supportsDeepSleepAutonomy",
            &sensor.supportsDeepSleepAutonomy,
            outError,
            true))
    {
        return false;
    }

    if (sensor.minSensitivity < 0 || sensor.maxSensitivity < 0 ||
        sensor.minSensitivity > sensor.maxSensitivity)
    {
        setError(
            outError,
            "invalid Motion Sensor sensitivity range for sensor index " + std::to_string(index));
        return false;
    }

    const std::string defaultConfigPrefix = prefix + "defaultStartConfig.";
    if (!readStringField(
            instance,
            defaultConfigPrefix + "operationalMode",
            &sensor.defaultStartConfig.operationalMode,
            outError,
            true) ||
        !readInt32Field(
            instance,
            defaultConfigPrefix + "noMotionSeconds",
            &sensor.defaultStartConfig.noMotionSeconds,
            outError,
            true) ||
        !readInt32Field(
            instance,
            defaultConfigPrefix + "activeStartSeconds",
            &sensor.defaultStartConfig.activeStartSeconds,
            outError,
            true) ||
        !readInt32Field(
            instance,
            defaultConfigPrefix + "activeStopSeconds",
            &sensor.defaultStartConfig.activeStopSeconds,
            outError,
            true))
    {
        return false;
    }

    if (sensor.defaultStartConfig.operationalMode != "MOTION" &&
        sensor.defaultStartConfig.operationalMode != "NO_MOTION")
    {
        setError(
            outError,
            "unsupported Motion Sensor operational mode: " +
                sensor.defaultStartConfig.operationalMode);
        return false;
    }

    const std::string activeWindowsKey = prefix + "active_windows";
    const uint32_t activeWindowCount = ut_kvp_getListCount(instance, activeWindowsKey.c_str());
    sensor.activeWindows.clear();
    sensor.activeWindows.reserve(activeWindowCount);

    for (uint32_t windowIndex = 0; windowIndex < activeWindowCount; ++windowIndex)
    {
        MotionSensorActiveWindow window;
        const std::string windowPrefix =
            activeWindowsKey + "." + std::to_string(windowIndex) + ".";

        if (!readInt32Field(
                instance,
                windowPrefix + "startTimeOfDaySeconds",
                &window.startTimeOfDaySeconds,
                outError,
                true) ||
            !readInt32Field(
                instance,
                windowPrefix + "endTimeOfDaySeconds",
                &window.endTimeOfDaySeconds,
                outError,
                true))
        {
            return false;
        }

        constexpr int32_t SECONDS_PER_DAY = 24 * 60 * 60;
        if (window.startTimeOfDaySeconds < 0 ||
            window.startTimeOfDaySeconds >= SECONDS_PER_DAY ||
            window.endTimeOfDaySeconds < 0 ||
            window.endTimeOfDaySeconds >= SECONDS_PER_DAY)
        {
            setError(
                outError,
                "Motion Sensor active-window time must be between 0 and 86399 seconds");
            return false;
        }

        sensor.activeWindows.push_back(window);
    }
    LOGF_INFO(
        "%s: Motion Sensor parsed (index=%u, id=%d, name='%s', sensitivity=[%d,%d], "
        "deepSleepAutonomy=%s, mode='%s', noMotionSeconds=%d, activeStartSeconds=%d, "
        "activeStopSeconds=%d, activeWindows=%zu)",
        logPrefix,
        index,
        sensor.id,
        sensor.sensorName.c_str(),
        sensor.minSensitivity,
        sensor.maxSensitivity,
        sensor.supportsDeepSleepAutonomy ? "true" : "false",
        sensor.defaultStartConfig.operationalMode.c_str(),
        sensor.defaultStartConfig.noMotionSeconds,
        sensor.defaultStartConfig.activeStartSeconds,
        sensor.defaultStartConfig.activeStopSeconds,
        sensor.activeWindows.size());

    return true;
}

ut_kvp_instance_t* createKvpInstance(const char* fileName)
{
    if (fileName == nullptr || std::strlen(fileName) == 0)
    {
        return nullptr;
    }

    ut_kvp_instance_t* instance = ut_kvp_createInstance();
    if (instance == nullptr)
    {
        return nullptr;
    }

    std::vector<char> mutableFileName(std::strlen(fileName) + 1, '\0');
    std::memcpy(mutableFileName.data(), fileName, mutableFileName.size());

    if (ut_kvp_open(instance, mutableFileName.data()) != UT_KVP_STATUS_SUCCESS)
    {
        ut_kvp_destroyInstance(instance);
        return nullptr;
    }

    return instance;
}

void destroyKvpInstance(ut_kvp_instance_t* instance)
{
    if (instance != nullptr)
    {
        ut_kvp_close(instance);
        ut_kvp_destroyInstance(instance);
    }
}

bool parseMotionSensorConfig(
    const char* configurationFile,
    MotionSensorHfpConfig& configuration,
    std::string* outError)
{
    configuration = MotionSensorHfpConfig{};

    if (configurationFile == nullptr || std::strlen(configurationFile) == 0)
    {
        setError(outError, "Motion Sensor HFP YAML path is empty");
        return false;
    }

    ut_kvp_instance_t* instance = createKvpInstance(configurationFile);
    if (instance == nullptr)
    {
        setError(
            outError,
            std::string("failed to open Motion Sensor HFP YAML: ") + configurationFile);
        return false;
    }

    const uint32_t sensorCount = ut_kvp_getListCount(instance, MOTION_SENSOR_LIST);
    if (sensorCount == 0)
    {
        destroyKvpInstance(instance);
        setError(
            outError,
            std::string("missing or empty Motion Sensor HFP profile: ") + MOTION_SENSOR_ROOT);
        return false;
    }

    configuration.sensors.reserve(sensorCount);
    bool success = true;
    for (uint32_t sensorIndex = 0; sensorIndex < sensorCount; ++sensorIndex)
    {
        MotionSensorConfig sensor;
        if (!getMotionSensorInfo(instance, sensorIndex, sensor, outError))
        {
            success = false;
            break;
        }

        configuration.sensors.push_back(std::move(sensor));
    }

    destroyKvpInstance(instance);

    if (!success)
    {
        configuration = MotionSensorHfpConfig{};
    }

    return success;
}

} // namespace

// PUBLIC_INTERFACE
bool loadMotionSensorHfpConfigFromYaml(
    const std::string& path,
    MotionSensorHfpConfig* outConfig,
    std::string* outError)
{
    if (outConfig == nullptr)
    {
        setError(outError, "outConfig is null");
        return false;
    }

    auto fail = [&](const std::string& message) {
        *outConfig = MotionSensorHfpConfig{};
        setError(outError, message);
        LOGF_ERROR("%s: Motion Sensor YAML parsing failed (path='%s'): %s", logPrefix, path.c_str(), message.c_str());
        return false;
    };

    LOGF_INFO("%s: Motion Sensor YAML parsing started (path='%s')", logPrefix, path.c_str());

    MotionSensorHfpConfig parsedConfig;
    if (!parseMotionSensorConfig(path.c_str(), parsedConfig, outError))
    {
        const std::string errorMessage =
            (outError != nullptr && !outError->empty()) ? *outError : "unknown parse error";
        return fail(errorMessage);
    }

    *outConfig = std::move(parsedConfig);
    if (outError != nullptr)
    {
        outError->clear();
    }

    LOGF_INFO(
        "%s: Motion Sensor YAML parsing succeeded (path='%s', sensors=%zu)",
        logPrefix,
        path.c_str(),
        outConfig->sensors.size());
    return true;
}

} // namespace vcomponent::utility

