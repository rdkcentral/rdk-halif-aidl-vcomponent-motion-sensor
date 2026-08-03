# rdk-halif-aidl-vcomponent-sensor

The Motion Sensor vDevice module provides an emulated Motion Sensor HAL implementation backed by a Binder/AIDL service. The service uses an HFP (HAL Feature Profile) YAML configuration to define the Motion Sensor interface version and sensor capabilities. During startup, the service loads the HFP YAML, starts a UT ControlPlane endpoint for receiving YAML/KVP control messages, publishes the Binder service under the AIDL-defined Motion Sensor service name, and then joins the Binder thread pool.

## Table of Contents

- [Motion Sensor (vDevice) README](#rdk-halif-aidl-vcomponent-sensor)
  - [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
  - [Build RDKMotionSensorService](#build-rdkmotionsensorservice)
  - [Run Motion Sensor](#run-motion-sensor)

## Acronyms, Terms and Abbreviations

| Acronym / Term | Description |
|----------------|-------------|
| **AIDL** | Android Interface Definition Language |
| **HAL** | Hardware Abstraction Layer |
| **HFP** | HAL Feature Profile (YAML profile used to configure the vComponent) |
| **RDK** | Reference Design Kit |
| **UT-Core** | RDK Unified Test Core Framework |
| **UT ControlPlane** | UT-Core / UT-Control control plane for receiving YAML/KVP control messages |
| **VTS** | Vendor Test Suite |
| **YAML** | Yet Another Markup Language (configuration format) |

## Build RDKMotionSensorService

### Prerequisites for UT-Core

This module relies on UT-Core / UT-Control headers and libraries. Please ensure all required packages for UT-Core are installed. See:
[Packages for ut-core](https://github.com/rdkcentral/ut-core/wiki/UT-Core-Building-using-Docker-or-Vagrant#script-for-installing-basic-packages-for-ut-core)

### Clone the Repository

```bash
git clone https://github.com/rdkcentral/rdk-halif-aidl-vcomponent-sensor.git

cd rdk-halif-aidl-vcomponent-sensor
```

### Environment variables

The build is driven by `./build.sh` in this repository. It uses (or defaults) the following environment variables:

- `UT_CORE_VERSION`: Specific version of UT-Core to build. If not set, the script checks out the latest tag.
- `RDK_HALIF_AIDL_VERSION`: Git ref used if the script must clone `rdk-halif-aidl`. The script defaults to `main`.

Example:

```bash
export UT_CORE_VERSION=5.1.0
export RDK_HALIF_AIDL_VERSION=0.22.0
```

### Build command (Target Linux)

From the repository root:

```bash
./build.sh Target=linux
```

At a high level, the `build.sh` script:

1. Stages Linux binder service-manager binaries, headers, and libraries into `build/usr`.
2. Generates AIDL C++ headers for the Motion Sensor interface (AIDL “current”).
3. Builds the AIDL support library via `aidl_lib/Makefile`.
4. Clones and builds `ut-core` (checked out to `UT_CORE_VERSION`) and stages required headers.
5. Builds the Motion Sensor service using CMake. The service executable is named `RDKMotionSensorService`.

## Run Motion Sensor

### Run the service on a target device

To run the Motion Sensor Binder service:

1. Copy the repository’s `build/` folder onto the target device (for example using `scp`), or otherwise ensure the built binary and `vcomponent_configurations/` directory are present on the target filesystem.
2. Run the Motion Sensor service binary.

The service executable is named:

- `RDKMotionSensorService`

### Command line interface

The Motion Sensor service supports the following command line flags:

- `--hfp <path>`: Optional HFP YAML path.
  Default: `vcomponent_configurations/hfp-sensor.yaml`
- `--port <port>`: Optional UT ControlPlane port to listen on.
  Default: `8081`

Example:

```bash
./RDKMotionSensorService --hfp vcomponent_configurations/hfp-sensor.yaml --port 8081
```

If `--help` (or `-h`) is provided, or if an unknown argument is provided, the service prints usage and exits with failure.

