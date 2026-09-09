# Changelog

## 1. [1.3.0] - 2026-08-28

### 1.1 Added

- Added `action_info_example` for viewing the currently executing built-in action.

## 2. [1.2.1] - 2026-08-24

### 2.1 Changed

- Updated the product README and Topic Examples documentation.
- Added joint-control interface documentation covering message fields, units, array order, controllable ranges, and zero-position definitions.
- Improved state-machine safety-control logging to reduce repeated output.

## 3. [1.2.0] - 2026-08-17

### 3.1 Added

- Added `developer_mode_example` for entering and exiting High-Level Motion, Whole-Body Joint, and Upper-Body Joint Control Modes.
- Added dedicated English and Chinese Developer Mode documentation for gamepad and SDK-based switching.
- Added `action_example` for executing preset actions.

## 4. [1.1.0] - 2026-08-04

### 4.1 Added

- Added the `/fault_aggregator` example for monitoring the current active-fault snapshot.
- Added the monitoring example group and `BUILD_DR02_PRO_MONITORING` build option for battery and fault monitoring.

### 4.2 Changed

- Moved the battery-state example into the monitoring group; its executable name and run command are unchanged.

## 5. [1.0.0] - 2026-07-15

### 5.1 Added

- First official release of the DR02 Pro ROS 2 SDK.
- Added the DR02 Pro state machine for joint control and RL motion control.
- Added low-level joint-control and high-level motion-control Topic examples.
- Added battery, IMU, gamepad key, and audio Topic examples.
- Added x86, Arm, and simulation build support.
- Added English and Chinese documentation for building, deployment, SDK modes, and examples.
