# Motion Sensor vcomponent (stub)

This repository contains a **basic stub** for an RDK **Motion Sensor** component wrapper/service that is intended to work with the RDK HALIF motion-sensor AIDL interfaces.

The current codebase is focused on the motion-sensor domain and includes Binder-facing manager, sensor, controller, listener, helper, and service scaffolding. It is **not yet a hardware-backed implementation**.

## What this repository currently provides

- A Binder service entrypoint for the motion sensor component
- Stub C++ classes for:
  - `IMotionSensorManager`
  - `IMotionSensor`
  - `IMotionSensorController`
  - controller/event listener support
- Build integration for RDK HALIF AIDL and Binder SDK artifacts
- A sample motion-sensor HFP YAML configuration
- Utility helpers for configuration parsing and file handling

## Current implementation status

This repository is presently a **stub-only motion sensor implementation**.

In the current code:

- `MotionSensorManager` initializes in stub-only mode
- `getMotionSensorIds()` returns an empty list
- `getMotionSensor()` returns `nullptr`
- the service starts, validates a YAML path, attempts to read the config file, and then publishes the Binder service threadpool

That means:

- the project builds the motion-sensor component structure
- the service can be started
- the real hardware-backed motion detection behavior is **not implemented yet**

## Authoritative interface reference

The motion-sensor interfaces and documentation are aligned to the RDK HALIF sensor motion model in the bundled `rdk-halif-aidl` workspace.

Useful references in this repository:

- `rdk-halif-aidl/sensor/0.1.0.0/docs/motion/motion_sensor.md`
- `include/aidl/`
- `src/aidl/`
- `src/service/vcomponent_MotionSensorService.cpp`

## Repository layout

This repository follows the motion-sensor-oriented layout below:

- `include/aidl/`  
  Motion sensor Binder-facing headers and component classes
- `src/aidl/`  
  Motion sensor manager/controller/listener implementation stubs
- `src/service/`  
  Motion sensor service entrypoint
- `include/common/`  
  Logging support
- `include/utility/` and `src/utility/`  
  Helper and configuration-parsing utilities
- `vcomponent_configurations/`  
  Example HFP YAML for the motion sensor
- `aidl_lib/Makefile`  
  Placeholder support for AIDL-related packaging/generation
- `ut-core/`  
  Unit-test support dependency used by the build flow

## Motion sensor configuration

The default configuration used by the service is:

- `vcomponent_configurations/hfp-sensor-motion.yaml`

The sample YAML describes motion-sensor-related properties such as:

- sensor identifier
- sensor name
- supported sensitivity range
- deep-sleep autonomy support
- operational modes
- default start configuration
- active time windows
- timing requirements and notes

Example values currently included in the sample configuration:

- sensor id: `0`
- sensor name: `PIR-Front-1`
- supported modes:
  - `MOTION`
  - `NO_MOTION`
- `supportsDeepSleepAutonomy: true`

## Build

### Local build

```sh
./build.sh Target=linux
```

### Other supported target

```sh
./build.sh Target=arm
```

## How the build works

The build script is motion-sensor specific and performs the following high-level steps:

1. Checks out `rdk-halif-aidl` if it is not already present
2. Builds Binder tooling from `rdk-halif-aidl`
3. Builds the `sensor` HALIF module for the configured version
4. Exports Binder/HALIF include and library paths
5. Builds `ut-core`
6. Copies shared/common headers
7. Configures and builds this Motion Sensor project with CMake
8. Installs outputs under `build/out/`

The CMake configuration expects Binder and HALIF-generated artifacts to be available. If those dependencies are missing, configuration fails fast.

## Build outputs

The CMake project builds:

- `motionsensor_core` shared library
- `RDKMotionSensorService` executable

It also copies `vcomponent_configurations/` into the build output as part of the build.

## Service behavior

The motion-sensor service entrypoint is implemented in:

- `src/service/vcomponent_MotionSensorService.cpp`

Current behavior:

- accepts an optional YAML config path argument
- defaults to `vcomponent_configurations/hfp-sensor-motion.yaml`
- validates the argument count and trimmed config path
- logs startup details
- attempts to read the YAML file
- continues even if the config file cannot be read
- publishes the motion-sensor Binder manager and joins the threadpool

## Notes about the current stub

Although the repository contains:

- a sample motion-sensor configuration
- sensor/controller/listener classes
- HALIF/Binder wiring

the exposed runtime behavior is still intentionally minimal.

At present, the code does **not** provide:

- real motion sensor enumeration
- hardware event delivery
- actual start/stop motion detection
- operational sensor state transitions driven by hardware
- hardware-backed sensitivity or active-window enforcement

## Cleaning build artifacts

```sh
./build.sh clean
```

To remove build products and checked-out dependencies:

```sh
./build.sh dist_clean
```

## Summary

This repository is a **Motion Sensor vcomponent stub**, not an HDMI Input component.

It is intended as the starting point for a real RDK motion-sensor Binder service implementation, with:

- motion-sensor-specific source layout
- sample motion HFP configuration
- Binder/HALIF build integration
- stub service and manager/controller classes ready for hardware integration
