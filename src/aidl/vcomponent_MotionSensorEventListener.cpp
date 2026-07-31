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

#include "aidl/vcomponent_MotionSensorEventListener.h"

#include "common/logger.h"

namespace com::rdk::hal::sensor::motion
{

namespace
{
constexpr const char* componentName = "MotionSensorEventListener";
}

android::binder::Status MotionSensorEventListener::onEvent(const MotionEvent& event)
{
    (void)event;

    // The AIDL contract is one-way, so acknowledge receipt without blocking
    // the Binder callback thread or applying hardware-specific behavior.
    LOGF_INFO("%s: onEvent received by stub-only listener", componentName);
    return android::binder::Status::ok();
}

} // namespace com::rdk::hal::sensor::motion
