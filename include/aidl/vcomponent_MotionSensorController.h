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

#include <com/rdk/hal/sensor/motion/BnMotionSensorController.h>
#include <com/rdk/hal/sensor/motion/IMotionSensorController.h>
#include <com/rdk/hal/sensor/motion/IMotionSensorControllerListener.h>

#include <mutex>

namespace com::rdk::hal::sensor::motion
{

class MotionSensor;

/**
 * @brief Stub-only controller surface whose operational APIs are unsupported.
 */
class MotionSensorController final : public BnMotionSensorController
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Construct a controller session for the given motion sensor.
     *
     * @param[in] parent Non-owning parent pointer.
     * @param[in] listener Controller listener owned by the opening client.
     */
    MotionSensorController(
        MotionSensor* parent,
        const android::sp<IMotionSensorControllerListener>& listener);

    MotionSensorController(const MotionSensorController&) = delete;
    MotionSensorController& operator=(const MotionSensorController&) = delete;

    // PUBLIC_INTERFACE
    android::binder::Status start(const StartConfig& config) override;

    // PUBLIC_INTERFACE
    android::binder::Status stop() override;

    // PUBLIC_INTERFACE
    android::binder::Status getStartConfig(StartConfig* _aidl_return) override;

    // PUBLIC_INTERFACE
    android::binder::Status getLastEventInfo(std::optional<LastEventInfo>* _aidl_return) override;

    // PUBLIC_INTERFACE
    android::binder::Status getSensitivity(int32_t* _aidl_return) override;

    // PUBLIC_INTERFACE
    android::binder::Status setSensitivity(int32_t sensitivity, bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    android::binder::Status setAutonomousDuringDeepSleep(bool enabled, bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    android::binder::Status isAutonomousDuringDeepSleepEnabled(bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    android::binder::Status setActiveWindows(const std::vector<TimeWindow>& windows, bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    android::binder::Status getActiveWindows(std::vector<TimeWindow>* _aidl_return) override;

    // PUBLIC_INTERFACE
    android::binder::Status clearActiveWindows(bool* _aidl_return) override;

private:
    MotionSensor* m_parent{nullptr};
    android::sp<IMotionSensorControllerListener> m_listener;
    mutable std::mutex m_mutex;
};

} // namespace com::rdk::hal::sensor::motion
