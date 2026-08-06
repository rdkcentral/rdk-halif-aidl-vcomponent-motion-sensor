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

#include <com/rdk/hal/sensor/motion/BnMotionSensorManager.h>
#include <com/rdk/hal/sensor/motion/IMotionSensor.h>
#include <com/rdk/hal/sensor/motion/IMotionSensorManager.h>

#include <binder/BinderService.h>
#include <utils/StrongPointer.h>

#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace com::rdk::hal::sensor::motion
{

class MotionSensor;

/**
 * @brief Binder service that exposes configured motion sensor instances.
 */
class MotionSensorManager final
    : public android::BinderService<MotionSensorManager>
    , public BnMotionSensorManager
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Return the well-known service name for the motion sensor manager.
     *
     * @return Binder service name from the generated AIDL interface.
     */
    static char const* getServiceName()
    {
        static const auto kServiceName = IMotionSensorManager::serviceName();
        return kServiceName.c_str();
    }

    // PUBLIC_INTERFACE
    /**
     * @brief Set the HFP YAML path used by the next manager instance.
     *
     * @param[in] configPath Path to the motion sensor HFP YAML profile.
     */
    static void setConfigPath(const std::string& configPath);

    // PUBLIC_INTERFACE
    /**
     * @brief Construct the manager and initialize deterministic sensor state.
     */
    MotionSensorManager();

    ~MotionSensorManager() override;

    MotionSensorManager(const MotionSensorManager&) = delete;
    MotionSensorManager& operator=(const MotionSensorManager&) = delete;

    // PUBLIC_INTERFACE
    /**
     * @brief Return all configured motion sensor identifiers.
     *
     * @param[out] _aidl_return Optional vector of configured sensor IDs.
     *
     * @return Successful Binder status or EX_NULL_POINTER for a null return pointer.
     */
    android::binder::Status getMotionSensorIds(
        std::optional<std::vector<std::optional<IMotionSensor::Id>>>* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Return the motion sensor instance for a configured identifier.
     *
     * @param[in] motionSensorId Requested sensor identifier.
     * @param[out] _aidl_return Sensor instance or nullptr if the ID is unknown.
     *
     * @return Successful Binder status, EX_ILLEGAL_ARGUMENT for an unknown sensor ID,
     *         or EX_NULL_POINTER for a null return pointer.
     */
    android::binder::Status getMotionSensor(
        const IMotionSensor::Id& motionSensorId,
        android::sp<IMotionSensor>* _aidl_return) override;

private:
    mutable std::mutex m_mutex;
    std::vector<android::sp<MotionSensor>> m_sensors;
};

} // namespace com::rdk::hal::sensor::motion
