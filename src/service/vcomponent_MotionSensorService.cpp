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

#include "service/vcomponent_MotionSensorService.h"

#include "aidl/vcomponent_MotionSensorManager.h"
#include "common/logger.h"
#include "controller/vcomponent_MotionSensorControlPlane.h"
#include "utility/vcomponent_MotionSensorHelper.h"
#include "utility/vcomponent_MotionSensorParseConfig.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/String16.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>

namespace
{
constexpr const char* logPrefix = "[VDEVICE_MOTION]<MotionSensorService>";
constexpr const char* kHfpArgument = "--hfp";
constexpr const char* kPortArgument = "--port";

void logUsage()
{
    LOGF_ERR(
        "%s: usage: vcomponent_MotionSensorService "
        "[--hfp <path>] [--port <1-65535>]",
        logPrefix);
}

bool parsePort(const char* value, int* outPort)
{
    if (value == nullptr || outPort == nullptr || value[0] == '\0')
    {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    const long parsedPort = std::strtol(value, &end, 10);
    if (errno == ERANGE || end == value || *end != '\0' ||
        parsedPort < 1 || parsedPort > 65535 || parsedPort > INT_MAX)
    {
        return false;
    }

    *outPort = static_cast<int>(parsedPort);
    return true;
}
} // namespace

/**
 * @brief Motion sensor service entrypoint.
 *
 * Parses optional `--hfp` and `--port` arguments, validates the Motion Sensor
 * HFP YAML, starts the UT control plane, and publishes the Binder service.
 *
 * @param[in] argc Argument count.
 * @param[in] argv Argument vector.
 *
 * @return 0 on success, or 1 when arguments, configuration, or control-plane
 *         initialization are invalid.
 */
int main(int argc, char** argv)
{
    std::string configPath = vcomponent::motion::kDefaultMotionSensorHfpPath;
    int controlPlanePort = vcomponent::motion::kDefaultMotionSensorControlPlanePort;

    if (argv == nullptr)
    {
        LOGF_ERR("%s: argument vector is null", logPrefix);
        return 1;
    }

    for (int argumentIndex = 1; argumentIndex < argc; ++argumentIndex)
    {
        const char* argument = argv[argumentIndex];
        if (argument == nullptr)
        {
            LOGF_ERR("%s: argument %d is null", logPrefix, argumentIndex);
            return 1;
        }

        if (std::string(argument) == kHfpArgument)
        {
            if (++argumentIndex >= argc || argv[argumentIndex] == nullptr)
            {
                LOGF_ERR("%s: --hfp requires a configuration path", logPrefix);
                logUsage();
                return 1;
            }

            configPath = vcomponent::utility::trim(argv[argumentIndex]);
            if (configPath.empty())
            {
                LOGF_ERR("%s: --hfp configuration path must not be empty", logPrefix);
                return 1;
            }
        }
        else if (std::string(argument) == kPortArgument)
        {
            if (++argumentIndex >= argc ||
                !parsePort(argv[argumentIndex], &controlPlanePort))
            {
                LOGF_ERR(
                    "%s: --port requires an integer in the range 1 through 65535",
                    logPrefix);
                logUsage();
                return 1;
            }
        }
        else
        {
            LOGF_ERR("%s: unrecognized argument: %s", logPrefix, argument);
            logUsage();
            return 1;
        }
    }

    LOGF_INFO(
        "%s: starting motion sensor Binder service "
        "(serviceName=%s, configPath=%s, controlPlanePort=%d)",
        logPrefix,
        com::rdk::hal::sensor::motion::MotionSensorManager::getServiceName(),
        configPath.c_str(),
        controlPlanePort);

    vcomponent::utility::MotionSensorHfpConfig configuration;
    std::string parseError;
    if (!vcomponent::utility::loadMotionSensorHfpConfigFromYaml(
            configPath, &configuration, &parseError))
    {
        LOGF_ERR(
            "%s: Motion Sensor HFP YAML validation failed; service will not start. "
            "path=%s error=%s",
            logPrefix,
            configPath.c_str(),
            parseError.empty() ? "unknown parser error" : parseError.c_str());
        return 1;
    }

    LOGF_INFO(
        "%s: Motion Sensor HFP YAML validation succeeded. path=%s sensors=%zu",
        logPrefix,
        configPath.c_str(),
        configuration.sensors.size());

    com::rdk::hal::sensor::motion::MotionSensorManager::setConfiguration(configuration);
    auto manager = android::sp<com::rdk::hal::sensor::motion::MotionSensorManager>::make();

    if (!vcomponent::motion::startMotionSensorControlPlane(
            manager.get(),
            controlPlanePort))
    {
        LOGF_ERR(
            "%s: failed to start motion-sensor control plane on port=%d",
            logPrefix,
            controlPlanePort);
        return 1;
    }

    // Publish the same manager instance that the control plane references.
    // Using BinderService::publishAndJoinThreadPool() would allocate a second
    // MotionSensorManager, causing injected events and Binder state changes to
    // target different MotionSensor objects.
    const android::status_t addServiceStatus = android::defaultServiceManager()->addService(
        android::String16(
            com::rdk::hal::sensor::motion::MotionSensorManager::getServiceName()),
        manager);
    if (addServiceStatus != android::OK)
    {
        LOGF_ERR(
            "%s: failed to publish motion sensor Binder service status=%d",
            logPrefix,
            static_cast<int>(addServiceStatus));
        vcomponent::motion::stopMotionSensorControlPlane();
        return 1;
    }

    android::sp<android::ProcessState> processState(android::ProcessState::self());
    processState->startThreadPool();
    processState->giveThreadPoolName();
    android::IPCThreadState::self()->joinThreadPool();

    vcomponent::motion::stopMotionSensorControlPlane();
    return 0;
}
