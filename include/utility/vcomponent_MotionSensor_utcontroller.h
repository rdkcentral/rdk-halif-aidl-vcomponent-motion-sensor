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

#include <string>

namespace vcomponent::motionsensor::controller
{

/**
 * @brief Minimal UT-controller facade for the motion sensor vcomponent.
 */
class MotionSensorUtController
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Construct the controller.
     */
    MotionSensorUtController() = default;

    ~MotionSensorUtController() = default;

    MotionSensorUtController(const MotionSensorUtController&) = delete;
    MotionSensorUtController& operator=(const MotionSensorUtController&) = delete;

    // PUBLIC_INTERFACE
    /**
     * @brief Load motion sensor configuration from the HFP YAML.
     *
     * @param[in] hfpYamlPath Path to hfp-sensor-motion.yaml.
     * @param[out] outError Optional output error string.
     *
     * @return true on success, false on error.
     */
    bool loadConfiguration(const std::string& hfpYamlPath, std::string* outError)
    {
        m_hfpYamlPath = hfpYamlPath;
        if (outError != nullptr)
        {
            *outError = "YAML parsing not implemented.";
        }
        return false;
    }

    // PUBLIC_INTERFACE
    /**
     * @brief Build a text inventory for the motion sensor component.
     *
     * @param[out] outInventory Output inventory string.
     * @param[out] outError Optional output error string.
     *
     * @return true on success.
     */
    bool buildInventory(std::string* outInventory, std::string* outError) const
    {
        if (outInventory == nullptr)
        {
            if (outError != nullptr)
            {
                *outError = "outInventory is null";
            }
            return false;
        }

        *outInventory = "MotionSensor inventory (hfpYamlPath=" + m_hfpYamlPath + ")";
        return true;
    }

private:
    std::string m_hfpYamlPath;
};

} // namespace vcomponent::motionsensor::controller
