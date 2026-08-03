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

#include "utility/vcomponent_MotionSensorHfpConfigUtils.h"

#include <string>

namespace vcomponent::utility
{

// PUBLIC_INTERFACE
/**
 * @brief Parse the motion sensor HFP YAML into MotionSensorHfpConfig.
 *
 * Parsing remains dependency-free and intentionally unimplemented in this repo.
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
