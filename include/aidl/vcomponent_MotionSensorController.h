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

namespace com::rdk::hal::sensor::motion
{

class MotionSensor;

/**
 * @brief Exclusive controller surface for a motion sensor opened by a client.
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
    /**
     * @brief Start the motion sensor with the supplied configuration.
     *
     * @param[in] config Start configuration.
     *
     * @return Successful Binder status, EX_ILLEGAL_ARGUMENT, or EX_ILLEGAL_STATE.
     */
    android::binder::Status start(const StartConfig& config) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Stop the motion sensor.
     *
     * @return Successful Binder status or EX_ILLEGAL_STATE.
     */
    android::binder::Status stop() override;

    // PUBLIC_INTERFACE
    /**
     * @brief Return the active start configuration.
     *
     * @param[out] _aidl_return Active session start configuration.
     *
     * @return Successful Binder status, EX_NULL_POINTER, or EX_ILLEGAL_STATE.
     */
    android::binder::Status getStartConfig(StartConfig* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Return diagnostic information for the most recent motion event.
     *
     * @param[out] _aidl_return Optional last-event diagnostic snapshot.
     *
     * @return Successful Binder status, EX_NULL_POINTER, or EX_ILLEGAL_STATE.
     */
    android::binder::Status getLastEventInfo(std::optional<LastEventInfo>* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Return the current sensitivity setting.
     *
     * @param[out] _aidl_return Current sensitivity.
     *
     * @return Successful Binder status or EX_NULL_POINTER.
     */
    android::binder::Status getSensitivity(int32_t* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Set the sensitivity while the sensor is stopped.
     *
     * @param[in] sensitivity Desired sensitivity.
     * @param[out] _aidl_return true when set, false when unsupported.
     *
     * @return Successful Binder status, EX_NULL_POINTER, EX_ILLEGAL_ARGUMENT, or EX_ILLEGAL_STATE.
     */
    android::binder::Status setSensitivity(int32_t sensitivity, bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Enable or disable autonomous deep-sleep detection while stopped.
     *
     * @param[in] enabled Desired mode.
     * @param[out] _aidl_return true when applied, false when unsupported.
     *
     * @return Successful Binder status, EX_NULL_POINTER, or EX_ILLEGAL_STATE.
     */
    android::binder::Status setAutonomousDuringDeepSleep(bool enabled, bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Query autonomous deep-sleep detection state.
     *
     * @param[out] _aidl_return Current autonomous deep-sleep setting.
     *
     * @return Successful Binder status or EX_NULL_POINTER.
     */
    android::binder::Status isAutonomousDuringDeepSleepEnabled(bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Replace active event-delivery windows while stopped.
     *
     * @param[in] windows Daily active windows.
     * @param[out] _aidl_return true when accepted.
     *
     * @return Successful Binder status, EX_NULL_POINTER, EX_ILLEGAL_ARGUMENT, or EX_ILLEGAL_STATE.
     */
    android::binder::Status setActiveWindows(const std::vector<TimeWindow>& windows, bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Return configured active event-delivery windows.
     *
     * @param[out] _aidl_return Configured active windows.
     *
     * @return Successful Binder status or EX_NULL_POINTER.
     */
    android::binder::Status getActiveWindows(std::vector<TimeWindow>* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Clear active windows, enabling 24-hour monitoring.
     *
     * @param[out] _aidl_return true when cleared.
     *
     * @return Successful Binder status, EX_NULL_POINTER, or EX_ILLEGAL_STATE.
     */
    android::binder::Status clearActiveWindows(bool* _aidl_return) override;

private:
    MotionSensor* m_parent{nullptr};
    android::sp<IMotionSensorControllerListener> m_listener;
};

} // namespace com::rdk::hal::sensor::motion
