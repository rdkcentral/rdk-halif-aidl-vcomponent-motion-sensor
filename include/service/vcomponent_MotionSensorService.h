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

#ifndef VCOMPONENT_MOTION_SENSOR_SERVICE_H_
#define VCOMPONENT_MOTION_SENSOR_SERVICE_H_

/**
 * @brief Starts the Motion Sensor Binder service.
 *
 * Accepts an optional path to the Motion Sensor YAML configuration file,
 * validates it, and publishes the Binder service threadpool.
 *
 * @param[in] argc Number of command-line arguments.
 * @param[in] argv Command-line argument vector; argv[1] may provide a
 *                 configuration path.
 *
 * @return 0 when the service starts successfully, or 2 for invalid arguments.
 */
int main(int argc, char** argv);

#endif  // VCOMPONENT_MOTION_SENSOR_SERVICE_H_
