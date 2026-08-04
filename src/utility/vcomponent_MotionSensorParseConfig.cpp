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
#include "utility/vcomponent_MotionSensorHelper.h"

namespace vcomponent::utility
{

bool loadMotionSensorHfpConfigFromYaml(
    const std::string& path,
    MotionSensorHfpConfig* outConfig,
    std::string* outError)
{
    if (outConfig == nullptr)
    {
        if (outError != nullptr)
        {
            *outError = "outConfig is null";
        }
        return false;
    }

    *outConfig = MotionSensorHfpConfig{};

    const auto fileContentsOpt = readFileToString(path);
    if (!fileContentsOpt.has_value())
    {
        if (outError != nullptr)
        {
            *outError = "failed to read file: " + path;
        }
        return false;
    }

    (void)fileContentsOpt;

    if (outError != nullptr)
    {
        *outError =
            "YAML parsing not implemented. Add a YAML parser and map fields to MotionSensorHfpConfig.";
    }
    return false;
}

} // namespace vcomponent::utility
