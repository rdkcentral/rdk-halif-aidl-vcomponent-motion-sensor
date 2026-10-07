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

#include <ut_control_plane.h>
#include <ut_kvp.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
constexpr const char* logPrefix = "[VDEVICE_MOTION]<MotionSensorControlPlane>";
constexpr const char* kMotionSensorRootKey = "IMotionSensor";
constexpr const char* kCommandKey = "IMotionSensor/command";
constexpr const char* kSensorIdKey = "IMotionSensor/sensor_id";
constexpr const char* kEventKey = "IMotionSensor/event";
constexpr const char* kPlainCommandKey = "command";
constexpr const char* kPlainSensorIdKey = "sensor_id";
constexpr const char* kPlainEventKey = "event";
constexpr const char* kCommandMotionSensorEvent = "motion_sensor_event";
constexpr const char* kEventMotion = "MOTION";

ut_controlPlane_instance_t* gControlPlaneInstance = nullptr;
com::rdk::hal::sensor::motion::MotionSensorManager* gMotionSensorManager = nullptr;
int gControlPlanePort = 0;

bool kvpGetString(
    ut_kvp_instance_t* kvpInstance,
    const char* key,
    std::string* outValue)
{
    if (kvpInstance == nullptr || key == nullptr || outValue == nullptr)
    {
        return false;
    }

    constexpr size_t kMaxValueLength = 256;
    char value[kMaxValueLength] = {};
    ut_kvp_getStringField(
        kvpInstance,
        key,
        value,
        static_cast<uint32_t>(sizeof(value)));

    if (value[0] == '\0')
    {
        return false;
    }

    *outValue = value;
    return true;
}

bool kvpFieldPresent(ut_kvp_instance_t* kvpInstance, const char* key)
{
    return kvpInstance != nullptr && key != nullptr &&
           ut_kvp_fieldPresent(kvpInstance, const_cast<char*>(key));
}

bool parseNonNegativeInt32(const std::string& value, int32_t* outValue)
{
    if (outValue == nullptr || value.empty())
    {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    const long parsedValue = std::strtol(value.c_str(), &end, 10);
    if (errno == ERANGE || end == value.c_str() || *end != '\0' ||
        parsedValue < 0 || parsedValue > INT_MAX)
    {
        return false;
    }

    *outValue = static_cast<int32_t>(parsedValue);
    return true;
}

template <size_t KeyCount>
bool kvpGetFirstString(
    ut_kvp_instance_t* kvpInstance,
    const char* const (&keys)[KeyCount],
    std::string* outValue,
    const char** matchedKey = nullptr)
{
    if (outValue == nullptr)
    {
        return false;
    }

    if (matchedKey != nullptr)
    {
        *matchedKey = nullptr;
    }

    for (const char* key : keys)
    {
        if (kvpGetString(kvpInstance, key, outValue))
        {
            if (matchedKey != nullptr)
            {
                *matchedKey = key;
            }
            return true;
        }
    }

    return false;
}

template <size_t KeyCount>
bool kvpGetFirstSensorId(
    ut_kvp_instance_t* kvpInstance,
    const char* const (&keys)[KeyCount],
    int32_t* outSensorId,
    const char** matchedKey = nullptr)
{
    if (kvpInstance == nullptr || outSensorId == nullptr)
    {
        return false;
    }

    if (matchedKey != nullptr)
    {
        *matchedKey = nullptr;
    }

    for (const char* key : keys)
    {
        if (!kvpFieldPresent(kvpInstance, key))
        {
            continue;
        }

        std::string sensorIdString;
        if (kvpGetString(kvpInstance, key, &sensorIdString) &&
            parseNonNegativeInt32(sensorIdString, outSensorId))
        {
            if (matchedKey != nullptr)
            {
                *matchedKey = key;
            }
            return true;
        }

        *outSensorId = static_cast<int32_t>(
            ut_kvp_getUInt32Field(kvpInstance, const_cast<char*>(key)));
        if (matchedKey != nullptr)
        {
            *matchedKey = key;
        }
        return true;
    }

    return false;
}

void onControlPlaneMessage(char* key, ut_kvp_instance_t* instance, void* userData)
{
    (void)userData;

    if (instance == nullptr)
    {
        LOGF_ERR("%s: received null KVP message", logPrefix);
        return;
    }

    char* kvpData = ut_kvp_getData(instance);
    if (kvpData == nullptr)
    {
        LOGF_ERR("%s: received message without YAML data", logPrefix);
        return;
    }

    const bool hasRootPayload = kvpFieldPresent(instance, kMotionSensorRootKey);
    const bool hasNamespacedCommand = kvpFieldPresent(instance, kCommandKey);

    if (key != nullptr)
    {
        if (std::strcmp(key, kCommandKey) == 0 && hasRootPayload)
        {
            std::free(kvpData);
            return;
        }

        if (std::strcmp(key, kPlainCommandKey) == 0 &&
            (hasRootPayload || hasNamespacedCommand))
        {
            std::free(kvpData);
            return;
        }
    }

    std::string command;
    const char* matchedCommandKey = nullptr;
    const char* const commandKeys[] = {kCommandKey, kPlainCommandKey};
    if (!kvpGetFirstString(instance, commandKeys, &command, &matchedCommandKey))
    {
        LOGF_ERR(
            "%s: message is missing command "
            "(expected %s or %s) payload=%s",
            logPrefix,
            kCommandKey,
            kPlainCommandKey,
            kvpData);
        std::free(kvpData);
        return;
    }

    if (command != kCommandMotionSensorEvent)
    {
        LOGF_DEBUG(
            "%s: ignoring unsupported command=%s matchedKey=%s payload=%s",
            logPrefix,
            command.c_str(),
            matchedCommandKey == nullptr ? "<unknown>" : matchedCommandKey,
            kvpData);
        std::free(kvpData);
        return;
    }

    std::string eventType;
    const char* matchedEventKey = nullptr;
    const char* const eventKeys[] = {kEventKey, kPlainEventKey};
    if (!kvpGetFirstString(instance, eventKeys, &eventType, &matchedEventKey) ||
        eventType != kEventMotion)
    {
        LOGF_ERR(
            "%s: motion_sensor_event requires "
            "%s or %s with value MOTION; NO_MOTION is generated by StartConfig timing payload=%s",
            logPrefix,
            kEventKey,
            kPlainEventKey,
            kvpData);
        std::free(kvpData);
        return;
    }

    if (gMotionSensorManager == nullptr)
    {
        LOGF_WARN(
            "%s: motion event received before manager initialization",
            logPrefix);
        std::free(kvpData);
        return;
    }

    int32_t sensorIdValue = 0;
    const char* matchedSensorIdKey = nullptr;
    const char* const sensorIdKeys[] = {kSensorIdKey, kPlainSensorIdKey};
    if (!kvpGetFirstSensorId(
            instance,
            sensorIdKeys,
            &sensorIdValue,
            &matchedSensorIdKey))
    {
        LOGF_ERR(
            "%s: motion_sensor_event is missing sensor id "
            "(expected %s or %s) payload=%s",
            logPrefix,
            kSensorIdKey,
            kPlainSensorIdKey,
            kvpData);
        std::free(kvpData);
        return;
    }

    com::rdk::hal::sensor::motion::IMotionSensor::Id sensorId;
    sensorId.value = sensorIdValue;

    android::sp<com::rdk::hal::sensor::motion::IMotionSensor> sensor;
    const android::binder::Status status =
        gMotionSensorManager->getMotionSensor(sensorId, &sensor);
    if (!status.isOk() || sensor == nullptr)
    {
        LOGF_ERR(
            "%s: motion_sensor_event references unknown sensor id=%d payload=%s",
            logPrefix,
            sensorIdValue,
            kvpData);
        std::free(kvpData);
        return;
    }

    const android::sp<com::rdk::hal::sensor::motion::MotionSensor> implementation =
        android::sp<com::rdk::hal::sensor::motion::MotionSensor>::cast(sensor);
    if (implementation == nullptr)
    {
        LOGF_ERR(
            "%s: unable to access sensor implementation id=%d payload=%s",
            logPrefix,
            sensorIdValue,
            kvpData);
        std::free(kvpData);
        return;
    }

    const bool injected = implementation->injectMotionEvent();
    if (!injected)
    {
        LOGF_WARN(
            "%s: ignored motion_sensor_event for inactive "
            "sensor id=%d event=%s sensorKey=%s eventKey=%s payload=%s",
            logPrefix,
            sensorIdValue,
            eventType.c_str(),
            matchedSensorIdKey == nullptr ? "<unknown>" : matchedSensorIdKey,
            matchedEventKey == nullptr ? "<unknown>" : matchedEventKey,
            kvpData);
        std::free(kvpData);
        return;
    }

    LOGF_INFO(
        "%s: injected motion_sensor_event for sensor id=%d "
        "event=%s sensorKey=%s eventKey=%s commandKey=%s",
        logPrefix,
        sensorIdValue,
        eventType.c_str(),
        matchedSensorIdKey == nullptr ? "<unknown>" : matchedSensorIdKey,
        matchedEventKey == nullptr ? "<unknown>" : matchedEventKey,
        matchedCommandKey == nullptr ? "<unknown>" : matchedCommandKey);
    std::free(kvpData);
}
} // namespace

namespace vcomponent::motion
{

// PUBLIC_INTERFACE
/**
 * @brief Start the motion-sensor UT control plane.
 *
 * Registers the motion command callbacks and starts the UT control plane on the
 * configured port. Repeated calls retain the already-running instance.
 *
 * @param[in] manager Running Binder motion-sensor manager.
 * @param[in] port TCP port used by the UT control plane listener.
 *
 * @return true when the control plane is available; otherwise false.
 */
bool startMotionSensorControlPlane(
    com::rdk::hal::sensor::motion::MotionSensorManager* manager,
    int port)
{
    if (gControlPlaneInstance != nullptr)
    {
        LOGF_WARN(
            "%s: control plane is already running on port=%d",
            logPrefix,
            gControlPlanePort);
        return true;
    }

    if (manager == nullptr)
    {
        LOGF_ERR("%s: cannot start with null manager", logPrefix);
        return false;
    }

    if (port < 1 || port > 65535)
    {
        LOGF_ERR(
            "%s: invalid control-plane port=%d",
            logPrefix,
            port);
        return false;
    }

    gMotionSensorManager = manager;
    gControlPlaneInstance = UT_ControlPlane_Init(port);
    if (gControlPlaneInstance == nullptr)
    {
        LOGF_ERR(
            "%s: failed to initialize port=%d",
            logPrefix,
            port);
        gMotionSensorManager = nullptr;
        return false;
    }
    gControlPlanePort = port;

    const ut_control_plane_status_t rootCallbackStatus =
        UT_ControlPlane_RegisterCallbackOnMessage(
            gControlPlaneInstance,
            const_cast<char*>(kMotionSensorRootKey),
            &onControlPlaneMessage,
            const_cast<char*>(kMotionSensorRootKey));
    const ut_control_plane_status_t namespacedCommandCallbackStatus =
        UT_ControlPlane_RegisterCallbackOnMessage(
            gControlPlaneInstance,
            const_cast<char*>(kCommandKey),
            &onControlPlaneMessage,
            const_cast<char*>(kCommandKey));
    const ut_control_plane_status_t plainCommandCallbackStatus =
        UT_ControlPlane_RegisterCallbackOnMessage(
            gControlPlaneInstance,
            const_cast<char*>(kPlainCommandKey),
            &onControlPlaneMessage,
            const_cast<char*>(kPlainCommandKey));

    if (rootCallbackStatus != UT_CONTROL_PLANE_STATUS_OK ||
        namespacedCommandCallbackStatus != UT_CONTROL_PLANE_STATUS_OK ||
        plainCommandCallbackStatus != UT_CONTROL_PLANE_STATUS_OK)
    {
        LOGF_ERR(
            "%s: failed to register callbacks "
            "rootStatus=%d namespacedCommandStatus=%d plainCommandStatus=%d",
            logPrefix,
            static_cast<int>(rootCallbackStatus),
            static_cast<int>(namespacedCommandCallbackStatus),
            static_cast<int>(plainCommandCallbackStatus));
        UT_ControlPlane_Exit(gControlPlaneInstance);
        gControlPlaneInstance = nullptr;
        gMotionSensorManager = nullptr;
        gControlPlanePort = 0;
        return false;
    }

    UT_ControlPlane_Start(gControlPlaneInstance);

    LOGF_INFO(
        "%s: started on port=%d rootKey=%s commandKeys=[%s,%s] "
        "sensorIdKeys=[%s,%s] eventKeys=[%s,%s]",
        logPrefix,
        gControlPlanePort,
        kMotionSensorRootKey,
        kCommandKey,
        kPlainCommandKey,
        kSensorIdKey,
        kPlainSensorIdKey,
        kEventKey,
        kPlainEventKey);
    return true;
}

// PUBLIC_INTERFACE
/**
 * @brief Stop the motion-sensor UT control plane.
 *
 * Stops worker threads, releases UT control-plane resources, and removes the
 * manager reference used by the static message callback.
 */
void stopMotionSensorControlPlane()
{
    if (gControlPlaneInstance == nullptr)
    {
        return;
    }

    UT_ControlPlane_Stop(gControlPlaneInstance);
    UT_ControlPlane_Exit(gControlPlaneInstance);
    gControlPlaneInstance = nullptr;
    gMotionSensorManager = nullptr;
    gControlPlanePort = 0;

    LOGF_INFO("%s: stopped", logPrefix);
}

} // namespace vcomponent::motion
