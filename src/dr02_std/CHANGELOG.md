# Changelog

## [1.3.0] - 2026-10-27

### Changed

- Updated `action_example` to use `/ACTION_COMMAND` with action IDs 30-33.
- Renamed `action_info_example` to `action_status_example` to report action IDs, execution stages, and error codes from `/ACTION_STATUS`.
- Updated `developer_mode_example` and Developer Mode documentation with complete entry and exit workflows.
- Changed the IMU and `/JOINTS_DATA` subscriptions to Best Effort reliability and Keep Last history with a queue depth of 1.
- Robot software 1.0.1 or later publishes these Topics using Best Effort reliability. Older SDK versions using Reliable subscriptions cannot receive them; use DR02 Std SDK 1.3.0 or later.

## [1.2.0] - 2026-08-28

### Added

- Added `action_info_example` for viewing the currently executing built-in action.

## [1.1.1] - 2026-08-24

### Changed

- Updated the product README and Topic Examples documentation.
- Added joint-control interface documentation covering message fields, units, array order, controllable ranges, and zero-position definitions.
- Improved state-machine safety-control logging to reduce repeated output.

## [1.1.0] - 2026-08-17

### Added

- Added `developer_mode_example` for entering and exiting High-Level Motion, Whole-Body Joint, and Upper-Body Joint Control Modes.
- Added dedicated English and Chinese Developer Mode documentation for gamepad and SDK-based switching.
- Added `action_example` for executing preset actions.

## [1.0.0] - 2026-08-06

### Added

- First official release of the DR02 Std ROS 2 SDK.
- Added the DR02 Std state machine for 21-joint control and RL motion control.
- Added low-level joint, upper-body, high-level motion, monitoring, peripheral, and audio examples.
- Added x86, Arm, and simulation build support.
- Added English and Chinese documentation for building, deployment, SDK modes, and examples.
