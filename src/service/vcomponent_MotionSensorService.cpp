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
#include "utility/vcomponent_MotionSensorParseConfig.h"
#include "utility/vcomponent_MotionSensorHelper.h"

#include <string>

/**
 * @brief Motion sensor service entrypoint.
 *
 * Accepts an optional configuration path argument, validates the configured
 * Motion Sensor HFP YAML using the motion-sensor parser, logs validation
 * results, and then publishes the Binder service threadpool.
 *
 * @param[in] argc Argument count.
 * @param[in] argv Argument vector. argv[1] may specify an alternate YAML path.
 *
 * @return 0 on success, or 1 when validation fails.
 */
int main(int argc, char** argv)
{
    constexpr const char* componentName = "MotionSensorService";
    constexpr const char* defaultConfigPath = "vcomponent_configurations/hfp-sensor-motion.yaml";

    std::string configPath = defaultConfigPath;
    if (argc > 1 && argv != nullptr && argv[1] != nullptr)
    {
        configPath = argv[1];
    }

    if (argc > 2)
    {
        LOGF_ERR("%s: too many arguments. Usage: vcomponent_MotionSensorService [config.yaml]", componentName);
        return 1;
    }

    configPath = vcomponent::utility::trim(configPath);
    if (configPath.empty())
    {
        LOGF_ERR("%s: empty config path after trimming input", componentName);
        return 1;
    }

    LOGF_INFO(
        "%s: starting motion sensor binder service (serviceName=%s, configPath=%s)",
        componentName,
        com::rdk::hal::sensor::motion::MotionSensorManager::getServiceName(),
        configPath.c_str());

    vcomponent::utility::MotionSensorHfpConfig configuration;
    std::string parseError;
    if (!vcomponent::utility::loadMotionSensorHfpConfigFromYaml(
            configPath, &configuration, &parseError))
    {
        LOGF_ERR(
            "%s: Motion Sensor HFP YAML validation failed; service will not start. "
            "path=%s error=%s",
            componentName,
            configPath.c_str(),
            parseError.empty() ? "unknown parser error" : parseError.c_str());
        return 1;
    }
    else
    {
        LOGF_INFO(
            "%s: Motion Sensor HFP YAML validation succeeded. path=%s sensors=%zu",
            componentName,
            configPath.c_str(),
            configuration.sensors.size());
    }

    com::rdk::hal::sensor::motion::MotionSensorManager::setConfiguration(configuration);
    com::rdk::hal::sensor::motion::MotionSensorManager::publishAndJoinThreadPool();
    return 0;
}

