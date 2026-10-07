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

#include "aidl/vcomponent_MotionSensorControllerListener.h"

#include "common/logger.h"

namespace com::rdk::hal::sensor::motion
{

namespace
{
constexpr const char* logPrefix = "[VDEVICE_MOTION]<MotionSensorControllerListener>";
}

android::binder::Status MotionSensorControllerListener::onStateChanged(State oldState, State newState)
{
    LOGF_INFO(
        "%s: onStateChanged: oldState=%d newState=%d",
        logPrefix,
        static_cast<int32_t>(oldState),
        static_cast<int32_t>(newState));
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorControllerListener::onActiveWindowEntered()
{
    LOGF_INFO("%s: onActiveWindowEntered", logPrefix);
    return android::binder::Status::ok();
}

android::binder::Status MotionSensorControllerListener::onActiveWindowExited()
{
    LOGF_INFO("%s: onActiveWindowExited", logPrefix);
    return android::binder::Status::ok();
}

} // namespace com::rdk::hal::sensor::motion
