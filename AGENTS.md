# SDK Application Guidance for Coding Agents

Use this guide when developing applications with the DEEPRobotics ROS 2 SDK.
The current product documentation and examples define the supported interfaces.

## Confirm the Product and Runtime

- Identify the target product, the execution host, and the target: robot or simulator.
- Read the [repository README](README.md) or [Chinese README](README_CN.md) for
  supported products, then follow the selected product's documentation below.
- Use the selected product's joint layout, sensors, and supported actions.
  Do not reuse joint arrays or indices across products without checking them.
- Confirm simulation support for the chosen example. Simulator behavior does not
  establish real-robot mode requirements or interface availability.

## Product Documentation

| Product | Product Guide | Developer Mode | Joint Interfaces | Topic Examples | State Machine | Simulation |
| --- | --- | --- | --- | --- | --- | --- |
| DR02 Pro | [Guide](src/dr02_pro/README.md) | [Guide](src/dr02_pro/docs/DEVELOPER_MODE.md) | [Guide](src/dr02_pro/docs/JOINT_CONTROL.md) | [Guide](src/dr02_pro/docs/EXAMPLES.md) | [Guide](src/dr02_pro/docs/STATE_MACHINE.md) | [Guide](src/dr02_pro/docs/SIMULATION.md) |
| DR02 Std | [Guide](src/dr02_std/README.md) | [Guide](src/dr02_std/docs/DEVELOPER_MODE.md) | [Guide](src/dr02_std/docs/JOINT_CONTROL.md) | [Guide](src/dr02_std/docs/EXAMPLES.md) | [Guide](src/dr02_std/docs/STATE_MACHINE.md) | [Guide](src/dr02_std/docs/SIMULATION.md) |

Chinese counterparts use the `_CN` suffix. Read the relevant version and source;
report discrepancies rather than guessing which behavior applies.
Add the corresponding documentation entry when a new product is introduced.
Supported interfaces and capabilities are defined by that product's documentation and code.

## Common Development Guidelines

- Verify Topics, message types, enum values, and QoS against the selected product's
  current code and documentation. Do not assume that products share an interface.
- If data or motion is missing, first check the ROS environment and target, runtime
  prerequisites, Topic publication conditions, and publisher/subscriber QoS.
- Discovering a Topic, publishing a command, and completing execution are different
  outcomes. Verify actual received data and the relevant execution feedback.
- Do not blindly increase speed, gains, or timeout limits to make a command work.
  Investigate the unmet prerequisite or failure reported by the feedback first.
- Treat field observations as investigation leads until their cause is verified;
  do not turn an unverified workaround into an interface requirement.
- Obtain explicit user authorization before sending real-robot motion commands or
  switching modes. Follow the product's safety and entry/exit procedures.
- Use the documented build steps and supported simulation path before robot testing.
  If the build environment is unavailable, report the limitation.
- Distinguish static checks, compilation, simulation, and real-robot validation in reports.
- Follow existing example style, keep control flow direct, and make minimal changes.
  Preserve user edits; do not stage or commit without authorization.
- Avoid speculative abstractions and dependencies. Change public protocols only
  when requested, and check affected documentation links and `git diff --check`.
- Keep parameter tables and operating procedures in product documentation;
  link to them rather than duplicating or inventing their values here.

## DR02 Pro and DR02 Std

The following guidance applies to these two products. Use each product's own
joint layout, gains, and supported action list.

- For robot-side built-in actions, start with `action_example`.
- For custom waist and arm trajectories, start with `arm_joint_example` or
  `arm_action_example`; these generate joint commands in the application.
- For whole-body joint control, start with `joints_example`. For custom policy
  execution, read the state-machine guide and its implementation.
- Developer Mode and its selected development mode are distinct from motion states
  such as `Idle` and `RLControl`. Check each prerequisite using its documented interface.
- Check the mode's controllable joints and feedback availability before running.
  Real-robot feedback availability can differ from simulation.
- Upper-Body Development Mode supports waist and arm commands together with `/STEER`;
  follow the selected product's example prerequisites before combining them.
- Construct the complete joint-command array required by the selected product.
  Check every final entry's index, position, velocity, torque, and gains, including
  entries the caller did not explicitly request, for unintended commands.
- Follow the corresponding example's initialization, gain configuration, and smooth
  interpolation from measured positions. Zero position is not the startup position.
- Do not apply one gain to every joint or infer that zero gains guarantee balance.
- Only one `/JOINTS_CMD` publisher may control the robot at a time.
- When diagnosing missing data or motion, check the development mode and motion
  state as well as the publication conditions documented for the selected interface.
- Use `motion_info_example` to observe motion states and `action_status_example`
  to observe built-in action results; follow the documented subscription timing.
