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

#include <com/rdk/hal/sensor/motion/BnMotionSensorControllerListener.h>
#include <com/rdk/hal/sensor/motion/State.h>

#include <binder/Status.h>

namespace com::rdk::hal::sensor::motion
{

/**
 * @brief Minimal controller-listener implementation for motion sensor callbacks.
 */
class MotionSensorControllerListener final : public BnMotionSensorControllerListener
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Construct a motion sensor controller listener.
     */
    MotionSensorControllerListener() = default;

    ~MotionSensorControllerListener() override = default;

    MotionSensorControllerListener(const MotionSensorControllerListener&) = delete;
    MotionSensorControllerListener& operator=(const MotionSensorControllerListener&) = delete;

    // PUBLIC_INTERFACE
    android::binder::Status onStateChanged(State oldState, State newState) override;

    // PUBLIC_INTERFACE
    android::binder::Status onActiveWindowEntered() override;

    // PUBLIC_INTERFACE
    android::binder::Status onActiveWindowExited() override;
};

} // namespace com::rdk::hal::sensor::motion
