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

#include "utility/vcomponent_MotionSensorHelper.h"

#include "common/logger.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace vcomponent::utility
{

std::optional<std::string> readFileToString(const std::string& path)
{
    std::ifstream inputFile(path);
    if (!inputFile.is_open())
    {
        LOGF_ERR("MotionSensorHelper: failed to open file for reading: path=%s", path.c_str());
        return std::nullopt;
    }

    std::ostringstream contentStream;
    contentStream << inputFile.rdbuf();
    if (inputFile.bad())
    {
        LOGF_ERR("MotionSensorHelper: failed while reading file: path=%s", path.c_str());
        return std::nullopt;
    }

    return contentStream.str();
}

std::string trim(const std::string& input)
{
    size_t startIndex = 0;
    while (startIndex < input.size() &&
           std::isspace(static_cast<unsigned char>(input[startIndex])))
    {
        ++startIndex;
    }

    size_t endIndex = input.size();
    while (endIndex > startIndex &&
           std::isspace(static_cast<unsigned char>(input[endIndex - 1])))
    {
        --endIndex;
    }

    return input.substr(startIndex, endIndex - startIndex);
}

} // namespace vcomponent::utility
