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
#include "utility/vcomponent_MotionSensor_helper.h"

#include <string>

/**
 * @brief Motion sensor stub-only service entrypoint.
 *
 * Accepts an optional configuration path argument, validates that the string is
 * non-empty after trimming, logs the selected stub-only motion-sensor service
 * name, and then publishes the Binder service threadpool.
 *
 * @param[in] argc Argument count.
 * @param[in] argv Argument vector. argv[1] may specify an alternate YAML path.
 *
 * @return 0 on success, or 2 when argument validation fails.
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
        return 2;
    }

    configPath = vcomponent::utility::trim(configPath);
    if (configPath.empty())
    {
        LOGF_ERR("%s: empty config path after trimming input", componentName);
        return 2;
    }

    LOGF_INFO(
        "%s: starting stub-only motion sensor binder service (serviceName=%s, configPath=%s)",
        componentName,
        com::rdk::hal::sensor::motion::MotionSensorManager::getServiceName(),
        configPath.c_str());

    const auto fileContentsOpt = vcomponent::utility::readFileToString(configPath);
    if (!fileContentsOpt.has_value())
    {
        LOGF_WARN(
            "%s: config file could not be read for stub-only service (continuing). path=%s",
            componentName,
            configPath.c_str());
    }
    else
    {
        LOGF_INFO(
            "%s: config file OK for stub-only service. path=%s bytes=%llu",
            componentName,
            configPath.c_str(),
            static_cast<unsigned long long>(fileContentsOpt->size()));
    }

    com::rdk::hal::sensor::motion::MotionSensorManager::publishAndJoinThreadPool();
    return 0;
}
