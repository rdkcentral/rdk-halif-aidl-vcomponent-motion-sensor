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

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace com::rdk::hal::sensor::motion
{

class MotionSensorController;

/**
 * @brief Binder implementation for a single configured motion sensor instance.
 *
 * This implementation follows the AIDL controller ownership model. One client
 * may open the sensor for exclusive control while multiple clients may register
 * event listeners.
 */
class MotionSensor final : public BnMotionSensor, public android::IBinder::DeathRecipient
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Construct a motion sensor instance with immutable capabilities and defaults.
     *
     * @param[in] id AIDL motion sensor identifier.
     * @param[in] capabilities Immutable capability values for this sensor.
     * @param[in] defaultStartConfig Factory default start configuration.
     * @param[in] defaultActiveWindows Factory default active event windows.
     */
    MotionSensor(
        const IMotionSensor::Id& id,
        const Capabilities& capabilities,
        const StartConfig& defaultStartConfig,
        const std::vector<TimeWindow>& defaultActiveWindows);

    // PUBLIC_INTERFACE
    /**
     * @brief Destroy the sensor after cancelling and joining its lifecycle timer.
     *
     * The destructor prevents delayed lifecycle work from accessing a sensor
     * after the manager releases its final Binder reference.
     */
    ~MotionSensor() override;

    MotionSensor(const MotionSensor&) = delete;
    MotionSensor& operator=(const MotionSensor&) = delete;

    // PUBLIC_INTERFACE
    /**
     * @brief Return immutable capabilities for this sensor.
     *
     * @param[out] _aidl_return Capability parcelable.
     *
     * @return Successful Binder status or EX_NULL_POINTER for a null return pointer.
     */
    android::binder::Status getCapabilities(Capabilities* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Return the current lifecycle state.
     *
     * @param[out] _aidl_return Current state value.
     *
     * @return Successful Binder status or EX_NULL_POINTER for a null return pointer.
     */
    android::binder::Status getState(State* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Open the sensor for exclusive controller ownership.
     *
     * @param[in] listener Controller lifecycle listener.
     * @param[out] _aidl_return Newly opened controller, or nullptr on failure.
     *
     * @return Successful Binder status, EX_NULL_POINTER, or EX_ILLEGAL_STATE.
     */
    android::binder::Status open(
        const android::sp<IMotionSensorControllerListener>& listener,
        android::sp<IMotionSensorController>* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Close a previously opened controller session.
     *
     * @param[in] controller Controller returned by open().
     * @param[out] _aidl_return true when the supplied controller was closed.
     *
     * @return Successful Binder status, EX_NULL_POINTER, or EX_ILLEGAL_STATE.
     */
    android::binder::Status close(
        const android::sp<IMotionSensorController>& controller,
        bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Register a motion event listener.
     *
     * @param[in] motionSensorEventListener Listener to register.
     * @param[out] _aidl_return true when newly registered, false when already registered.
     *
     * @return Successful Binder status or EX_NULL_POINTER.
     */
    android::binder::Status registerEventListener(
        const android::sp<IMotionSensorEventListener>& motionSensorEventListener,
        bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Unregister a motion event listener.
     *
     * @param[in] motionSensorEventListener Listener to unregister.
     * @param[out] _aidl_return true when removed, false when not found.
     *
     * @return Successful Binder status or EX_NULL_POINTER.
     */
    android::binder::Status unregisterEventListener(
        const android::sp<IMotionSensorEventListener>& motionSensorEventListener,
        bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Inject a motion event through registered event listeners.
     *
     * The event is delivered only while the sensor is started and the local
     * time-of-day falls within a configured active window. An empty window list
     * and a window with equal endpoints permit 24-hour monitoring. Listener
     * callbacks are invoked without holding the sensor mutex. In NO_MOTION
     * mode, a detected motion instead restarts the configured inactivity timer.
     *
     * @return true when the physical motion was accepted; otherwise false.
     */
    bool injectMotionEvent();

    // PUBLIC_INTERFACE
    /**
     * @brief Return the sensor identifier.
     *
     * @return Immutable sensor identifier.
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

    struct EventListenerRegistration
    {
        android::sp<android::IBinder> binder;
        android::sp<IMotionSensorEventListener> listener;
    };

    void binderDied(const ::android::wp<::android::IBinder>& who) override;
    bool controllerMatchesLocked(const android::sp<IMotionSensorController>& controller) const;
    bool isWithinActiveWindowLocked(int32_t timeOfDaySeconds) const;
    void startLifecycleTimerLocked(
        uint64_t lifecycleGeneration,
        int32_t activationDelaySeconds,
        int32_t activeStopSeconds);
    void cancelLifecycleTimerLocked();
    void startNoMotionTimerLocked();
    void cancelNoMotionTimerLocked();
    void invalidateLifecycleTimersLocked();
    void releaseControllerLocked();

    mutable std::mutex m_mutex;
    std::condition_variable m_lifecycleTimerCondition;
    std::thread m_lifecycleTimerThread;
    bool m_lifecycleTimerCancelled{false};
    std::condition_variable m_noMotionTimerCondition;
    std::thread m_noMotionTimerThread;
    bool m_noMotionTimerCancelled{false};

    IMotionSensor::Id m_id{};
    Capabilities m_capabilities{};
    State m_state{State::STOPPED};

    int32_t m_sensitivity{0};
    bool m_autonomousDuringDeepSleepEnabled{false};

    StartConfig m_startConfig{};
    // Restore HFP defaults for each controller session.
    std::vector<TimeWindow> m_defaultActiveWindows{};
    std::vector<TimeWindow> m_activeWindows{};
    std::optional<LastEventInfo> m_lastEventInfo{};
    uint64_t m_lifecycleGeneration{0};
    uint64_t m_noMotionTimerGeneration{0};

    android::sp<IMotionSensorControllerListener> m_controllerListener;
    android::sp<IMotionSensorController> m_controller;
    android::sp<android::IBinder> m_ownerBinder;

    std::vector<EventListenerRegistration> m_eventListeners;
};

} // namespace com::rdk::hal::sensor::motion
