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

#include <com/rdk/hal/sensor/motion/BnMotionSensorEventListener.h>
#include <com/rdk/hal/sensor/motion/MotionEvent.h>

#include <binder/Status.h>

namespace com::rdk::hal::sensor::motion
{

/**
 * @brief Stub-only recipient for asynchronous motion sensor event callbacks.
 *
 * The implementation acknowledges incoming AIDL callbacks and logs their
 * receipt without performing hardware-specific event processing.
 */
class MotionSensorEventListener final : public BnMotionSensorEventListener
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Construct a motion sensor event listener stub.
     */
    MotionSensorEventListener() = default;

    ~MotionSensorEventListener() override = default;

    MotionSensorEventListener(const MotionSensorEventListener&) = delete;
    MotionSensorEventListener& operator=(const MotionSensorEventListener&) = delete;

    // PUBLIC_INTERFACE
    /**
     * @brief Receive a motion sensor event sent through the one-way AIDL callback.
     *
     * @param[in] event The detected motion event payload.
     *
     * @return Successful Binder status after acknowledging the callback.
     */
    android::binder::Status onEvent(const MotionEvent& event) override;
};

} // namespace com::rdk::hal::sensor::motion
