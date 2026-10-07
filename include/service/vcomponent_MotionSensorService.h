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

namespace vcomponent::motion
{
/**
 * @brief Default Motion Sensor HFP configuration path.
 *
 * Used when the `--hfp` command-line option is omitted.
 */
inline constexpr const char* kDefaultMotionSensorHfpPath =
    "vcomponent_configurations/hfp-sensor-motion.yaml";

/**
 * @brief Default motion-sensor control-plane TCP port.
 *
 * Used when the `--port` command-line option is omitted.
 */
inline constexpr int kDefaultMotionSensorControlPlanePort = 8084;
} // namespace vcomponent::motion

/**
 * @brief Starts the Motion Sensor Binder service.
 *
 * The service accepts optional named arguments in any order:
 * `--hfp <path>` selects a Motion Sensor HFP YAML configuration, while
 * `--port <1-65535>` selects the control-plane TCP port. When omitted, the
 * HFP path defaults to
 * `vcomponent_configurations/hfp-sensor-motion.yaml` and the port defaults to
 * `8084`.
 *
 * @param[in] argc Number of command-line arguments.
 * @param[in] argv Command-line argument vector.
 *
 * @return 0 when the service starts successfully, or 1 for invalid arguments,
 *         configuration validation failures, or control-plane startup failures.
 */
int main(int argc, char** argv);

#endif  // VCOMPONENT_MOTION_SENSOR_SERVICE_H_
