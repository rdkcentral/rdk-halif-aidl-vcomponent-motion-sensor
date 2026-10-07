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

namespace com::rdk::hal::sensor::motion
{
class MotionSensorManager;
}

namespace vcomponent::motion
{

// PUBLIC_INTERFACE
/**
 * @brief Start the motion-sensor UT control plane.
 *
 * Registers the motion command callback and starts the control plane on the
 * supplied TCP port. Repeated calls keep the already-running instance.
 *
 * @param[in] manager Running Binder motion-sensor manager. Must not be null.
 * @param[in] port TCP port in the range 1 through 65535.
 *
 * @return true when the control plane is running; otherwise false.
 */
bool startMotionSensorControlPlane(
    com::rdk::hal::sensor::motion::MotionSensorManager* manager,
    int port);

// PUBLIC_INTERFACE
/**
 * @brief Stop the motion-sensor UT control plane.
 *
 * Stops the control-plane worker threads, releases its resources, and clears
 * the non-owning motion-sensor manager reference. This operation is safe when
 * the control plane is not running.
 */
void stopMotionSensorControlPlane();

} // namespace vcomponent::motion
