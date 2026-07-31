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

#include "aidl/vcomponent_MotionSensorManager.h"

#include "aidl/vcomponent_MotionSensor.h"
#include "common/logger.h"

namespace com::rdk::hal::sensor::motion
{

namespace
{
constexpr const char* componentName = "MotionSensorManager";
}

MotionSensorManager::MotionSensorManager()
{
    LOGF_INFO(
        "%s: initialized in stub-only mode; no hardware-backed motion sensors are exposed",
        componentName);
}

MotionSensorManager::~MotionSensorManager() = default;

android::binder::Status MotionSensorManager::getMotionSensorIds(
    std::optional<std::vector<std::optional<IMotionSensor::Id>>>* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getMotionSensorIds: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    *_aidl_return = std::vector<std::optional<IMotionSensor::Id>>{};

    LOGF_INFO(
        "%s: getMotionSensorIds returning an empty list because the implementation is stub-only",
        componentName);
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorManager::getMotionSensor(
    const IMotionSensor::Id& motionSensorId,
    android::sp<IMotionSensor>* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: getMotionSensor: null _aidl_return", componentName);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = nullptr;

    LOGF_WARN(
        "%s: getMotionSensor requested id=%d but no sensors are exposed because the implementation is stub-only",
        componentName,
        static_cast<int>(motionSensorId.value));
    return android::binder::Status::ok();
}

} // namespace com::rdk::hal::sensor::motion
