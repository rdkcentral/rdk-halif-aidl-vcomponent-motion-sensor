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

#include <com/rdk/hal/sensor/motion/BnMotionSensor.h>
#include <com/rdk/hal/sensor/motion/Capabilities.h>
#include <com/rdk/hal/sensor/motion/IMotionSensor.h>
#include <com/rdk/hal/sensor/motion/IMotionSensorController.h>
#include <com/rdk/hal/sensor/motion/IMotionSensorControllerListener.h>
#include <com/rdk/hal/sensor/motion/IMotionSensorEventListener.h>
#include <com/rdk/hal/sensor/motion/LastEventInfo.h>
#include <com/rdk/hal/sensor/motion/StartConfig.h>
#include <com/rdk/hal/sensor/motion/State.h>
#include <com/rdk/hal/sensor/motion/TimeWindow.h>

#include <binder/IBinder.h>
#include <binder/IInterface.h>

#include <map>
#include <mutex>
#include <optional>
#include <vector>

namespace com::rdk::hal::sensor::motion
{

class MotionSensorController;

/**
 * @brief Explicit stub-only binder implementation for a single motion sensor instance.
 *
 * The implementation intentionally provides no hardware-backed behavior and
 * rejects operational APIs as unsupported.
 */
class MotionSensor final : public BnMotionSensor, public android::IBinder::DeathRecipient
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Construct a motion sensor stub instance with the given sensor ID.
     *
     * @param[in] id AIDL motion sensor identifier.
     */
    explicit MotionSensor(const IMotionSensor::Id& id);

    MotionSensor(const MotionSensor&) = delete;
    MotionSensor& operator=(const MotionSensor&) = delete;

    // PUBLIC_INTERFACE
    /**
     * @brief Return immutable capabilities for this sensor.
     */
    android::binder::Status getCapabilities(Capabilities* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Return the current lifecycle state.
     */
    android::binder::Status getState(State* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Open the sensor for exclusive controller ownership.
     */
    android::binder::Status open(
        const android::sp<IMotionSensorControllerListener>& listener,
        android::sp<IMotionSensorController>* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Close a previously opened controller session.
     */
    android::binder::Status close(
        const android::sp<IMotionSensorController>& controller,
        bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Register a motion event listener.
     */
    android::binder::Status registerEventListener(
        const android::sp<IMotionSensorEventListener>& motionSensorEventListener,
        bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Unregister a motion event listener.
     */
    android::binder::Status unregisterEventListener(
        const android::sp<IMotionSensorEventListener>& motionSensorEventListener,
        bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Return the sensor identifier.
     */
    const IMotionSensor::Id& id() const
    {
        return m_id;
    }

    /**
     * @brief Change lifecycle state and notify the controller listener.
     *
     * @param[in,out] lock Caller-owned unique lock for the sensor mutex.
     * @param[in] newState New state value.
     */
    void changeStateLocked(std::unique_lock<std::mutex>& lock, State newState);

private:
    friend class ::com::rdk::hal::sensor::motion::MotionSensorController;

    void binderDied(const ::android::wp<::android::IBinder>& who) override;
    bool controllerMatchesLocked(const android::sp<IMotionSensorController>& controller) const;

    mutable std::mutex m_mutex;

    IMotionSensor::Id m_id{};
    Capabilities m_capabilities{};
    State m_state{State::STOPPED};

    int32_t m_sensitivity{1};
    bool m_autonomousDuringDeepSleepEnabled{false};

    StartConfig m_startConfig{};
    std::vector<TimeWindow> m_activeWindows{};
    std::optional<LastEventInfo> m_lastEventInfo{};

    android::sp<IMotionSensorControllerListener> m_controllerListener;
    android::sp<IMotionSensorController> m_controller;
    android::sp<android::IBinder> m_ownerBinder;

    std::map<::android::wp<::android::IBinder>, ::android::sp<IMotionSensorEventListener>> m_eventListeners;
};

} // namespace com::rdk::hal::sensor::motion
