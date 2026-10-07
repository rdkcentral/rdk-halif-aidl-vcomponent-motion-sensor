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

#include <optional>
#include <string>

namespace vcomponent::utility
{

// PUBLIC_INTERFACE
/**
 * @brief Read an entire file into a string.
 *
 * @param[in] path Path to the file.
 *
 * @return File contents or std::nullopt on failure.
 */
std::optional<std::string> readFileToString(const std::string& path);

// PUBLIC_INTERFACE
/**
 * @brief Trim leading and trailing whitespace from a string.
 *
 * @param[in] input Source string.
 *
 * @return Trimmed copy.
 */
std::string trim(const std::string& input);

} // namespace vcomponent::utility
