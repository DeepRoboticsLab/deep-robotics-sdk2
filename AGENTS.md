# AGENTS.md — instructions for a coding agent working with this SDK

**English** | [简体中文](AGENTS_CN.md)

Read this before writing code that talks to a DEEPRobotics robot. It is written to be
followed literally.

This SDK commands machines heavy enough to injure someone. The rules below are not style
preferences. **Each one is here because breaking it produces a robot that does the wrong
thing without any error message.** That is the shape of almost every entry in this file: not
a crash, not an exception, not a rejected message — a topic that lists, reports a type and
delivers nothing; an array that is accepted in full and moves a joint you did not mean; a
command that is published, received, and quietly ignored.

Those failures are expensive for a coding agent specifically. An agent reads a successful
return value as success, and there is nothing in the return value to read. **When a robot
does not do what you commanded, the first hypothesis to test is not that your value was
wrong. It is that your command never took effect.**

---

## Contents

**Common to every robot**

- [1. Scope and provenance — how much to trust this file](#1-scope-and-provenance--how-much-to-trust-this-file)
- [2. The five that cost the most](#2-the-five-that-cost-the-most)
- [3. Common rules — every DEEPRobotics robot](#3-common-rules--every-deeprobotics-robot)
- [4. Transport: the failures that produce no error](#4-transport-the-failures-that-produce-no-error)

**Per robot**

- [5. DR02 (Pro and Std)](#5-dr02-pro-and-std) —
  [modes](#51-developer-modes-decide-what-your-program-can-do-at-all) ·
  [arm control while walking](#52-arm-control-while-the-robot-still-walks-and-balances) ·
  [zero gain](#53-the-legs-are-kept-by-zero-gain-not-by-not-naming-them) ·
  [gains](#54-the-gains-are-a-table-and-so-is-the-damping) ·
  [variants and indices](#55-read-the-document-for-the-variant-in-front-of-you) ·
  [`/STEER`](#56-steer-preconditions-and-a-header-finding-we-cannot-fully-explain) ·
  [feedback](#57-feedback-joints_data_10hz-decoded-where-joints_data-did-not) ·
  [sensors](#58-sensors) · [build](#59-build)
- [6. Lite3](#6-lite3) —
  [what carries across](#61-what-carries-across-from-3-and-what-does-not) ·
  [balance controller](#62-taking-the-low-level-sdk-means-taking-the-balance-controller) ·
  [two velocity interfaces](#63-there-are-two-velocity-interfaces-and-the-wrong-one-fails-silently) ·
  [sign-only velocity](#64-a-sign-only-velocity-mapping-silently-invalidates-every-speed-measurement) ·
  [dead sender](#65-two-command-interfaces-two-different-answers-to-a-dead-sender) ·
  [state frames](#66-the-state-stream-check-the-frame-length-and-where-it-is-being-sent) ·
  [addressing](#67-two-robots-one-address-never-default-to-another-robots-ip) ·
  [the stop](#68-a-network-change-can-silently-disable-the-emergency-stop) ·
  [RGB only](#69-rgb-only-what-the-robot-cannot-see) ·
  [mode authority](#610-mode-authority-stays-with-the-operator)

**Extending this file**

- [7. Adding another robot to this file](#7-adding-another-robot-to-this-file)

---

## 1. Scope and provenance — how much to trust this file

This file has two kinds of content and they should be trusted differently.

**Quoted from this repository.** Joint index tables, gain arrays, mode numbers, motion-state
values and topic names are cited to the file they come from, in this repository, and can be
checked in a diff. If a vendor document and this file ever disagree, **the vendor document is
right and this file is stale** — fix this file.

**Measured on a robot.** Everything else was observed on hardware and is marked with the
date. Two bodies of observation feed this file:

| Marker | What it was | How much of it there is |
| --- | --- | --- |
| *[measured 2026-09-03]* | A one-day DR02 Pro bring-up session | One robot, one afternoon, one network |
| *[measured 2026-08/09]* | Integration work on two Lite3 Venture units, August–September 2026 | Two robots, several sessions, and a stack that is not the vendor's |

Each such statement gives the observation rather than a conclusion, so that it can be re-run
and disagreed with.

**Read the second kind as a place to look, not as a specification.** One robot on one day is
not a population. Several of these findings involve robot-side software that is outside this
SDK's documented surface — the DDS profile on the robot image, a `10 Hz` feedback topic, an
RTSP camera stream — and the SDK could not reasonably have anticipated them. They are
recorded here because an agent *will* meet them and will otherwise spend hours attributing
them to its own code.

**Nothing here has been verified on a DR02 Std.** Where a Std statement appears, it is quoted
from `src/dr02_std/` and is labelled as unverified on hardware.

---

## 2. The five that cost the most

If you read nothing else, read these. Each is expanded below.

| # | The trap | Where |
| --- | --- | --- |
| 1 | **Arm work requires Upper-Body Joint Control Mode.** High-Level mode's `/ACTION` vocabulary is canned social gestures with no grasp, lift or carry, and modes cannot be switched from inside a running program | [§5.1](#51-developer-modes-decide-what-your-program-can-do-at-all) |
| 2 | **The legs are kept by ZERO GAIN, not by not naming them.** `/JOINTS_CMD` is full length; a position entry with a non-zero `kp` is an active command that fights the walking policy | [§5.3](#53-the-legs-are-kept-by-zero-gain-not-by-not-naming-them) |
| 3 | **Gains are a table, and so is damping.** The Pro example uses `2800` on `waist_x_joint` and `90` on a wrist — a factor of thirty-one inside one array. `kd` is quoted, not derived from `kp` | [§5.4](#54-the-gains-are-a-table-and-so-is-the-damping) |
| 4 | **Read the document for the variant in front of you.** `dr02_pro/` and `dr02_std/` are siblings with different joint counts, arm degrees of freedom and gains, and neither says which robot you are holding | [§5.5](#55-read-the-document-for-the-variant-in-front-of-you) |
| 5 | **A velocity command is renewed, not set.** And on the wire, several things are silently discarded rather than rejected | [§3.5](#35-a-velocity-command-is-renewed-not-set), [§4](#4-transport-the-failures-that-produce-no-error) |

---

## 3. Common rules — every DEEPRobotics robot

These apply whatever robot is in front of you and are the parts of this file least likely to
go stale.

### 3.1 Identify the robot and the variant before you write a line

Product directories in this repository are siblings with the same filenames. A document is
correct for exactly one product and says nothing about the others, and **no document tells
you which robot is in front of you.** Copying a value across the boundary is not caught by
anything: the message formats are shared, the topic names are shared, and only the numbers
differ.

Establish the variant **by measurement**, not by assumption, and make the answer explicit in
your code. On a DR02 the joint count is the discriminator — see [§5.5](#55-read-the-document-for-the-variant-in-front-of-you).
Refuse to run when the measurement matches no known variant rather than guessing the nearer
one.

### 3.2 Developer Mode is a mode of the robot, not a flag in your program

Entering Developer Mode is an operator action on the gamepad, or a service call, and it
changes what the robot's own controller is doing. Two consequences that a program cannot
see:

- **After entering any Developer Mode, perception, localisation and obstacle avoidance are
  disabled** (`src/dr02_pro/docs/DEVELOPER_MODE.md`). The robot is no longer watching for
  people. Whatever your code does is then the only thing that is.
- **The modes cannot be switched directly.** Changing mode means stopping your program,
  fully exiting the current mode, and driving the gamepad menu. Do not design a run that
  needs a mid-run mode change; it is an operator sequence, not an API call.

Never write a program that assumes it is in a mode. Read the mode, and refuse to publish if
it is not the one you need.

### 3.3 Never write a joint index as a literal

```python
cmd[6] = 0.4                           # FORBIDDEN
targets = {"left_elbow_joint": 0.4}    # correct
```

`/JOINTS_CMD` carries a bare array with **no model field**. A wrong index is not rejected and
logs nothing — the robot simply moves a different joint. On a DR02, index 9 is a wrist on a
Pro and a **hip** on a Std ([§5.5](#55-read-the-document-for-the-variant-in-front-of-you)),
which means the same literal that nudges an arm on one robot commands a leg the walking
policy is standing on, on the other.

Address joints **by name**, against an index table selected by the measured variant. Keep
exactly one place in your code where a name becomes an index, and let everything else pass
names.

### 3.4 Gains are tables. Quote them; never derive them.

Do not invent a gain, do not reuse one joint's gain for another, and do not compute `kd` from
`kp`. Copy the vendor array for the variant you are on, verbatim, from the example that ships
in this repository, and cite the file you copied it from in a comment. If a joint you need is
not in that array, that is a question for the vendor, not a value to interpolate.

A single number applied across a whole array is the specific failure this rule exists to
prevent, and it is not a small error: see [§5.4](#54-the-gains-are-a-table-and-so-is-the-damping).

### 3.5 A velocity command is renewed, not set

A velocity intent is not a setting. It has to be re-published on every tick of a live control
loop, and your loop must publish zeros when it has nothing to say rather than falling silent.

This is not a quirk to work around — it *is* the deadman, and it is the right design. It
means a control loop that has died, blocked or lost its link stops the robot instead of
leaving a velocity latched. Write the loop so that stopping the sender is a stop.

```python
# WRONG — sets the intent once, then spins
commander.set_velocity(x=0.35)
while time.monotonic() < end:
    commander.tick()
    time.sleep(period)

# RIGHT — the intent is renewed every tick
while time.monotonic() < end:
    commander.set_velocity(x=0.35)
    commander.tick()
    time.sleep(period)
```

**Silence is not a stop.** To stop, publish a burst of zeros — not one message, and not
nothing.

**And establish, per robot and per interface, what happens when the sender dies. Do not
assume, and do not carry the answer across.** The two we have looked at differ, and they
differ in the direction that matters:

| Interface | What happens when the sending process is killed outright |
| --- | --- |
| DR02 `/STEER` *[measured 2026-09-03]* | The robot came to rest on its own — but travelled roughly a further 5 cm over about 5 seconds. Judged by eye; re-measure rather than quoting these numbers. Stopping distance is not zero. |
| Lite3 legacy velocity UDP *[see §6]* | The command **latches**. There is no receiver-side timeout on that interface, so a killed sender leaves the robot walking with nothing left to stop it. |

So the standard advice "prove the deadman with `SIGKILL`, not `Ctrl-C`" is correct on a DR02
and **actively dangerous as a habit on a Lite3's legacy interface**, where `SIGKILL` is the
one thing that guarantees your zero is never sent. Find out which case you are in on a gantry,
with a person on the stop button, before it matters. Where the interface latches, send zeros
in a cleanup path *and* keep the operator's stop as the real answer.

### 3.6 Check the payload, not the request

**This is the general lesson of the two falls behind this file, and it outlives any one
robot.** A guard that inspects what the caller *asked for* is not a guard on what leaves your
process.

`/JOINTS_CMD` is full length. Every joint gets an entry whether you named it or not, so a
seven-joint arm request becomes a 31-entry array — and the twenty-four entries you did not
write are the ones that dropped a robot ([§5.3](#53-the-legs-are-kept-by-zero-gain-not-by-not-naming-them)).
The check has to run **after** the expansion, on the array itself, per index.

Anything that expands a caller's intent into a wider structure — a full-length array, a
broadcast, a batch write — needs its safety check on the expanded thing. Assert on the
payload. Test on the payload.

### 3.7 Never wrap a publish in a bare `try`/`except`

A publish that fails while your node is running is **a stop that did not happen**. It must
propagate and it must be visible. Swallowing it converts a transport fault into a robot that
keeps moving while your logs look clean.

There is one defensible exception — skipping the publish when the ROS context is already torn
down, because it is then genuinely impossible. Write that one narrowly and do not generalise
it.

### 3.8 Never weaken a safety constant to make something work

Timeouts, stop-repeat counts, staleness limits, confidence thresholds and drift thresholds
are load-bearing. If one fires too often, **measure the real rate and report the mismatch**;
do not raise the limit until it stops complaining. A threshold that has been tuned until it
is quiet is a threshold that no longer protects anything.

The same applies to a failing test. A failing test is information about the code or about the
world. Deleting it, loosening its assertion or skipping it produces a green suite that means
nothing.

### 3.9 Nothing in your program is an emergency stop

The red stop button on the gamepad is the emergency stop, and a person whose only job is
holding it must be present for every run on a real robot. Software that publishes a stop can
only stop the robot while the software is still running — which is exactly the condition that
fails first.

Also, from this repository's own safety notes, and worth restating because a program cannot
enforce them:

- Only **one** `/JOINTS_CMD` publisher may run at a time. Running the state machine and a
  joint example together means two processes fighting for the robot.
- Test with a reliable safety suspension in place, and know that **suspended and standing are
  different states with different safe transitions** — the vendor requires both feet firmly on
  the ground before entering `RLControl`, and warns that switching while suspended may cause
  sudden motion.

---

## 4. Transport: the failures that produce no error

**Scope: this section is about the ROS 2 / DDS interface**, so it applies to the DR02 family
and to anything else using the vendor's `drdds` messages. The Lite3 speaks UDP and has its own
equivalents in [§6](#6-lite3).

Everything below is a way for a message to be published successfully and have no effect, or
for a topic to exist and deliver nothing. Check these **before** you start doubting your
control logic — on the day this file records, most of the lost time went to debugging correct
code.

### 4.1 Source the robot's DDS bootstrap first, in every shell, on every host

*[measured 2026-09-03]* On the DR02 Pro used for this session the robot image carried a DDS
profile script that had to be sourced **before** the ROS 2 environment:

```bash
set +u
source /opt/robot/scripts/setup_ros2.sh    # sets FASTRTPS_DEFAULT_PROFILES_FILE
source /opt/ros/humble/setup.bash
```

It points `FASTRTPS_DEFAULT_PROFILES_FILE` at the robot's Fast DDS profile, which carries a
transport whitelist containing the host's own address. **Without it, several robot topics
list, report a type, and deliver zero bytes** — which is indistinguishable from a dead sensor.
Sourcing it took one topic from `0` bytes to `1219` bytes per message with no other change.

`set +u` is required because the ROS setup scripts read undefined variables and abort under
`nounset` before ROS is reached.

The exact path is robot-image specific and is not part of this SDK; look for the equivalent
on the machine in front of you and source it. If topics list but deliver nothing, this is the
first thing to check.

### 4.2 Match the vendor's QoS: publishers are BEST_EFFORT

*[measured 2026-09-03]* The robot's publishers are BEST_EFFORT. **A RELIABLE subscriber never
connects to them, and nothing is logged on either side.** Your subscriber shows zero messages
and every diagnostic looks healthy.

```bash
ros2 topic echo /SOME_TOPIC --once --qos-reliability best_effort
```

In code: subscribe BEST_EFFORT to vendor topics. Note also that `ros2 topic hz` and
`ros2 topic bw` subscribe RELIABLE and **do not accept `--qos-reliability`** on Humble, so
they report zero on every best-effort topic. Only `echo` takes the flag. A zero from `hz` is
not evidence of a silent topic.

### 4.3 Judge a topic by bytes, not by publisher count

*[measured 2026-09-03]* Robot-side publishers appear as bare DDS applications rather than ROS
nodes, so ROS tooling does not count them. One topic reported `Publisher count: 0` while
delivering 1219 bytes in the same second. `ros2 node list` will not show them either.

The converse also misleads: **two publishers on a topic does not mean two robots.** Compare
the first 12 bytes of each endpoint GID — if the participant prefixes match, it is one process:

```bash
ros2 topic info --verbose /JOINTS_DATA
```

### 4.4 The `name` field is reserved and arrives empty

`src/dr02_pro/docs/JOINT_CONTROL.md` states that `name` is a reserved field and that SDK
applications should not rely on it. *[measured 2026-09-03]* On the robot, all 31 `name`
entries arrived empty.

Take this literally when designing: **the robot will not tell you what its joints are
called.** The name-to-index map comes from the vendor table, selected by the measured joint
count. That is why the count is the first thing to measure.

### 4.5 One robot can carry two hosts, two ROS distributions, and an asymmetric topic view

*[measured 2026-09-03]* On this robot the control host ran Ubuntu 24.04 / ROS 2 Jazzy and the
perception host ran Ubuntu 22.04 / ROS 2 Humble, on one DDS domain. They saw each other's
topics, **but not symmetrically**: joint topics were usable only from the perception host, and
the LiDAR topic delivered data only from the control host until §4.1 was applied.

Two consequences. Build and install the message package for the distribution on the host you
are building on. And **check a topic from the host you intend to use it from** — a topic
listing on the other host proves nothing about yours.

### 4.6 If your program runs off-robot, its clock is not the robot's clock

*[measured 2026-09-03]* See [§5.6](#56-steer-preconditions-and-a-header-finding-we-cannot-fully-explain).
A message that carries a timestamp from a host whose clock disagrees with the robot's is a
candidate explanation for commands that are received and ignored. This is a hypothesis, not
an established mechanism, but it is cheap to rule out: synchronise the clocks, or reproduce
the failure from a robot-side host and see whether it persists.

---

## 5. DR02 (Pro and Std)

Applies to `src/dr02_pro/` and `src/dr02_std/`. Everything marked *[measured 2026-09-03]* was
observed on a **DR02 Pro**; the Std statements are quoted from `src/dr02_std/` and have not
been verified on hardware.

### 5.1 Developer Modes decide what your program can do at all

**Choose the mode before you choose the approach.** Three modes exist
(`src/dr02_pro/docs/DEVELOPER_MODE.md`), and for most SDK work only two of them are usable:

| Mode | `set_mode` value | What the SDK owns | What the robot owns |
| --- | --- | --- | --- |
| **High-Level Motion Control** | `3` | `/MOTION_STATE`, `/GAIT`, `/STEER`, `/ACTION` | **all joints** |
| **Whole-Body Joint Control** | `2` | every joint, via `/JOINTS_CMD` | nothing — including balance |
| **Upper-Body Joint Control** | `1` | waist and both arms, via `/JOINTS_CMD` | the legs, under its own policy |

**High-Level mode cannot manipulate anything.** Its `/ACTION` interface takes a single
integer and its entire vocabulary is canned social gestures — twelve on the Pro
(`0x3000` `greeting` … `0x300b` `clap`) and five on the Std (`0x3000`–`0x3004`). There is no
grasp, no lift and no carry, and in High-Level mode the robot's internal policy owns the
joints. **If your task involves the arms doing anything other than a preset gesture, High-Level
is the wrong mode and no amount of work in it will get you there.**

**Whole-Body mode is not a shortcut to arm control either.** It hands you the legs as well,
which means you are now responsible for balancing a standing humanoid. This repository warns
directly that running an upper-body example in Whole-Body mode may cause the legs to lose
support and the robot to fall, because the leg joints receive no valid command.

**So arm work requires Upper-Body Joint Control Mode.** That is the whole of
[§5.2](#52-arm-control-while-the-robot-still-walks-and-balances).

**Modes cannot be switched from inside a running program.** They cannot be switched directly
at all: you stop the SDK program, confirm its publishers have stopped, fully exit the current
mode, and re-enter through the gamepad menu. Design the run as a single mode from start to
finish. A plan that says "walk in High-Level, then switch to Upper-Body and grasp" is a plan
with an operator, a gamepad and a full stop in the middle of it — and if the robot must stay
standing across that boundary, it does not work at all.

Read the current mode from `/DEVELOPER_MODE_STATUS`, which publishes JSON such as
`{"Enabled":true,"Mode":2,"Frequency":500}`, and refuse to publish if it is not the mode you
need.

### 5.2 Arm control while the robot still walks and balances

**This is the most useful thing in this file.** It is the configuration in which an autonomous
manipulation task is actually possible on a DR02, and it needs no mid-run mode change.

`/STEER` is accepted in **Upper-Body Joint Control Mode** as well as High-Level — this
repository's own example table lists `steer_example` under both
(`src/dr02_pro/docs/EXAMPLES.md`). So in one mode, with no switch:

- the robot walks and balances on its **own leg policy**, driven by `/STEER`;
- your program owns the **waist and both arms** through `/JOINTS_CMD`.

*[measured 2026-09-03]* In this configuration on a DR02 Pro, a single-arm reach tracked its
commanded angle to **0.11°** while the walking policy held the legs underneath it, and the
knees moved 0.07° and 0.57° across a 1076-frame, eight-second ramp with no abort.

Two things make it work, and both are easy to get wrong:

1. **The mode must be entered correctly.** Upper-Body mode entry expects the robot in the
   suspended-standing posture, so being on a gantry is correct *for mode entry*. It is not
   correct for `RLControl`, which the vendor requires be entered with **both feet firmly on the
   ground**. These are different steps in the same sequence; do not collapse them.
2. **The legs must be given zero gain.** Not "left out" — zero gain. This is §5.3, and getting
   it wrong is what dropped the robot.

### 5.3 The legs are kept by ZERO GAIN, not by not naming them

**This one dropped a robot. It is the least obvious thing in this file.**

`/JOINTS_CMD` is a **full-length array**: on a Pro, all 31 entries are sent whether you named
the joint or not. The natural safe-looking choice is to fill the unnamed entries with the
robot's own measured positions — the robot is being told to stay exactly where it already is.

**That is not the safe choice. A position entry with a non-zero `kp` is an ACTIVE command.**
The joint controller will hold that angle against anything — including against the robot's own
walking policy, which in Upper-Body mode is the thing keeping the legs underneath it.

*[measured 2026-09-03]* On a DR02 Pro: a right-arm reach naming seven joints, with `kp=40`,
`kd=2` applied uniformly to all 31 entries, ramped over eight seconds. The arm moved a little
and **the legs collapsed.** A guard asserting that the caller had named only upper-body joints
passed, and could not have caught it — the legs were never named. They arrived in the array
anyway.

**The gains are the guard.** This repository's own example already does the right thing
(`src/dr02_pro/low_level/arm_joint_example.cpp`): the array is full length, every one of the 31
entries is populated, and **every index at or beyond the upper-body count carries `kp = 0` and
`kd = 0`.**

```
kUpperBodyKp[17..30] = 0.0f      // legs and neck: present in the array, exerting no torque
kUpperBodyKd[17..30] = 0.0f
```

**Read that carefully, because the load-bearing part is the gain and not the position.** The
example writes `position = 0` into those leg entries, and that is harmless *only* because the
gains beside them are zero. The same entries with a non-zero `kp` would command every leg joint
to its calibrated zero at once. So:

- **The value in an uncommanded entry is irrelevant while its gain is zero, and catastrophic
  the moment it is not.** Whether you fill unnamed entries with `0` as the example does, or with
  the robot's measured positions, the thing keeping the robot up is the zero gain. Assert on the
  gains, per index, in a test.
- **Never treat position `0` as "no command".** `0` is the product's calibrated joint zero
  (`src/dr02_pro/docs/JOINT_CONTROL.md`), which is a specific pose, not an absence of one.
- **A measured position re-read every tick follows; it does not hold.** *[measured
  2026-09-03]* If you fill unnamed entries from live feedback, they track rather than resist.
  During a successful reach the torso leaned `waist_x` by `-0.177 rad` to counterbalance the
  extended arm, because the waist was unnamed. That is harmless unloaded, and it means "return
  to rest" has to command the whole upper body — all three waist joints included — or it leaves
  the torso wherever it drifted.

A leg-drift watchdog is worth having: snapshot every policy-owned joint at the start of a ramp
and abort if one moves too far. Its threshold is a judgement, not a constant to copy. *[measured
2026-09-03]* 0.05 rad tripped on a hip moving 2.9° that was almost certainly the policy
balancing correctly; 0.12 rad caught a fold without aborting on compensation. In Upper-Body
mode the hips and knees **should** move while an arm extends.

### 5.4 The gains are a table, and so is the damping

The authoritative source is **`src/dr02_pro/low_level/arm_joint_example.cpp`** for the Pro and
**`src/dr02_std/low_level/arm_joint_example.cpp`** for the Std. Copy from the file; do not
retype from here without checking.

DR02 Pro, upper body (indices `0`–`16`):

| Joints | `kp` | `kd` |
| --- | --- | --- |
| `waist_z_joint` | 600 | 6 |
| `waist_x_joint` | **2800** | **15** |
| `waist_y_joint` | 2300 | 20 |
| shoulders and elbows, both arms | 600 | 6 |
| wrists, both arms | **90** | **2** |
| legs and neck (`17`–`30`) | **0** | **0** |

**There is a factor of thirty-one between two joints in the same array.** A waist asked to
hold a torso upright and a wrist are not remotely the same actuator, and there is no single
number that serves both.

*[measured 2026-09-03]* A uniform `kp=40` across all joints folded the robot twice, and
produced **two symptoms that looked unrelated**: the waist sagged — 40 against a specified
2800, seventy times too little, and on a standing humanoid the legs follow the torso down —
while the arm "barely moved", because 40 was also pushing against a shoulder that wants 600.
One wrong number, two symptoms, and the second one sends you looking at the pose rather than
the gains.

**`kd` is quoted, not derived.** With the gains corrected, `kd` was then scaled as `kp/20` — a
guess. Against the vendor's array that guess is up to **9.3× too high** (`waist_x_joint`: 140
against a specified 15), and *[measured 2026-09-03]* the waist bent sideways during a two-armed
reach, on exactly the joint that was furthest out. Excessive derivative gain in a discrete
high-rate loop does not simply damp; it drives drift and oscillation.

**There is no `kp`/`kd` ratio to discover here. Both are tables.** If you keep these values in
your own code, add a test that asserts the waist's ratio differs from a wrist's, so that a
future well-meaning refactor cannot re-derive one from the other.

DR02 Std, upper body (indices `0`–`8`), quoted from `src/dr02_std/low_level/arm_joint_example.cpp`
and **not verified on hardware**:

| Joints | `kp` | `kd` |
| --- | --- | --- |
| `waist_z_joint` | 600 | 6 |
| shoulders and elbows, both arms | 200 | 5 |
| legs (`9`–`20`) | 0 | 0 |

Note that a Std shoulder is `200` where a Pro shoulder is `600`. **The tables are not
interchangeable in either direction.**

### 5.5 Read the document for the variant in front of you

**This is what caused both falls.** The gains that folded the robot came from
`dr02_std/low_level/arm_joint_example.cpp`; the robot was a Pro.

`src/dr02_pro/` and `src/dr02_std/` are sibling directories with identical filenames and
different contents, and **neither says which robot is in front of you.** They differ in joint
count, arm degrees of freedom, gains, `/ACTION` vocabulary and available IMU topics. The
message types and topic names are identical, so nothing rejects a cross-variant value.

**Measure the joint count first.** It is the discriminator, and it comes straight off the wire:

```bash
ros2 topic echo /JOINTS_DATA_10HZ --once --qos-reliability best_effort | grep -c data_id
```

*[measured 2026-09-03]* Returned `31` on three independent samples → a Pro. **31 is a Pro, 21
is a Std, and anything else means stop and ask** — do not round to the nearer variant.

The two index tables, from `src/dr02_pro/docs/JOINT_CONTROL.md` and
`src/dr02_std/docs/JOINT_CONTROL.md`:

| Group | Pro (31 joints) | Std (21 joints) |
| --- | --- | --- |
| Waist | `0`–`2` (`waist_z`, `waist_x`, `waist_y`) | `0` (`waist_z` only) |
| Left arm | `3`–`9` (7 DoF, incl. 3 wrist) | `1`–`4` (4 DoF, no wrist) |
| Right arm | `10`–`16` (7 DoF, incl. 3 wrist) | `5`–`8` (4 DoF, no wrist) |
| Left leg | `17`–`22` | `9`–`14` |
| Right leg | `23`–`28` | `15`–`20` |
| Neck | `29`–`30` (locked, not controllable) | none |
| Upper-body controllable range | `0`–`16` | `0`–`8` |

**The two variants share 21 joint names, and 20 of them sit at different indices.** Only
`waist_z_joint` at index `0` agrees. Index `9` is `left_wrist_x_joint` on a Pro and
`left_hip_y_joint` on a Std. On a Std, the Pro's arm indices `9`–`16` are **all legs the
walking policy is standing on.**

Because `/JOINTS_CMD` carries a bare array with no model field, a Pro-shaped command sent to a
Std is not rejected, logs nothing, and moves legs.

### 5.6 `/STEER` preconditions, and a header finding we cannot fully explain

`/STEER` is the correct topic for programmatic walking in both High-Level and Upper-Body modes.
`x`, `y` and `yaw` are **normalised ratios in `[-1.0, 1.0]`, not physical velocities**
(`src/dr02_pro/docs/EXAMPLES.md`), and `steer_example` uses a fixed `0.6`.

**Precondition: the robot must already be standing in `RLControl` with a gait selected.**
Nothing reports the absence of this as an error — the publish succeeds, the topic shows
traffic, the subscribers show as connected, and the robot stands still. *[measured 2026-09-03]*
104 `/STEER` messages at 20 Hz with `x=0.15` produced no motion at all.

Read the robot's actual state from **`/MOTION_INFO`**. `/MOTION_STATE` and `/GAIT` are
*command* topics and are silent — echoing them tells you nothing, which reads exactly like a
dead link.

```bash
ros2 topic echo /MOTION_INFO --once --qos-reliability best_effort
```

`/MOTION_INFO` reports these in **decimal** while `src/dr02_pro/docs/EXAMPLES.md` lists them in
**hex**, which is its own small trap: `17` and `0x11` are the same state, and `131078` is
`SuspendedStand` rather than anything wrong.

| State | Hex | Decimal |
| --- | --- | --- |
| `Idle` | `0x0` | 0 |
| `JointDamping` | `0x2` | 2 |
| **`RLControl`** | `0x11` | **17** — `/STEER` only means anything here |
| `SuspendedStand` | `0x20006` | 131078 |
| `HumanWALKAMP` (gait) | `0x21001` | 135169 |
| `HumanWALKTERRAIN` (gait) | `0x21006` | 135174 |

**The order is: lower the robot, both feet firmly on the ground, then `RLControl`, then a
gait, then `/STEER`.**

**Two open observations, offered as questions rather than claims.**

*[measured 2026-09-03]* **Small magnitudes produced no motion.** `x=0.15` moved nothing;
`0.35` walked. `steer_example` uses `0.6`. Whether there is a deadband on the normalised input
was not established.

*[measured 2026-09-03]* **`/STEER` was ignored until the header was left unset, and this cost
more time than anything else in the session.** With the robot in `RLControl` with a gait, feet
on the ground, QoS matched, no competing publisher, and `ros2 topic echo` showing the correct
values on the wire, **293 commands across three attempts produced no motion and no log line of
any kind.** A capture of the handheld controller driving the robot successfully showed every
message carrying `frame_id: 0` and an unset `stamp`. Publishing with `frame_id = 0` and
`header.stamp` left at its default moved the robot on the first attempt.

Both fields were changed together, so **we cannot say whether the stamp, the frame id, or both
are responsible.** Our best guess is that the consumer validates the stamp against the robot's
own clock and discards what disagrees — and our publisher was on a development host whose clock
did. That would be consistent with `steer_example` working, since it does set both fields
(`msg.header.frame_id = frame_id++; msg.header.stamp = node->now();`) and is typically run
where the clocks agree. **We would value a correction here** — if the consumer's behaviour is
documented somewhere we did not find, this whole entry can be replaced with a link.

Until then, the practical advice is: if `/STEER` is being received and ignored, try
`frame_id = 0` with an unset stamp before you go looking at gaits, deadbands and mode
arbitration — all of which were correct the whole time here.

### 5.7 Feedback: `/JOINTS_DATA_10HZ` decoded where `/JOINTS_DATA` did not

*[measured 2026-09-03]* Both topics carry `drdds/msg/Joints`. Over five attempts each, from
`ros2 topic echo` on the perception host:

| Topic | Decoded | Joints | Rate |
| --- | --- | --- | --- |
| `/JOINTS_DATA` | **0 / 5** | — | — |
| `/JOINTS_DATA_10HZ` | **5 / 5** | 31 | 10.000 Hz |

`/JOINTS_DATA` failed inside the DDS reader with `sequence size exceeds remaining buffer`,
before any ROS message conversion; `--raw` failed identically. It persisted after the DDS
bootstrap (§4.1), so it is not a transport or QoS problem, and it looks like a wire-format
disagreement between a native publisher and the ROS-generated IDL rather than anything a
subscriber can configure.

**Reported honestly and with a caveat.** This was observed only through `ros2 topic echo` from
a Python-side subscriber. **We did not run this repository's own `arm_joint_example`, which
subscribes `/JOINTS_DATA` in C++**, so we cannot say whether it is affected, and it may well
not be. If it is not, then the difference between the two paths is itself worth knowing and we
would be glad to be told what it is.

The workaround we used was to read `/JOINTS_DATA_10HZ`. **The cost is real and should be
stated:** the measured positions used for the unnamed entries of a `/JOINTS_CMD` array then
refresh every 100 ms rather than at the control rate. Note this at the constant in your own
code; it is a reduction in feedback bandwidth, not a free substitution.

### 5.8 Sensors

*[measured 2026-09-03]* These concern robot-image sensors rather than this SDK, and are
recorded because they change what a safety gate can promise.

**The LiDAR is blind behind the robot.** A bearing histogram of one sweep in the body frame
showed returns from roughly −120° to +120° and **exactly zero beyond ±120°** in either
direction. That is the field of view, not a filtering artefact — the distribution was
symmetric about forward. The consequence is direct: **a proximity stop gate built on this
LiDAR cannot see anyone standing behind the robot.** Disable backward motion in code rather
than merely avoiding it, and do not let anyone stand in the rear cone expecting the robot to
detect them. During this session the operators sat 1.5–2 m behind the robot and produced no
returns at all.

A self-return floor is needed regardless — a humanoid's LiDAR sees its own structure, and
returns closer than about 0.35 m were the robot itself. Without a floor the nearest range
always reads zero.

**Cameras: the USB descriptor serial is not the camera's firmware serial.**
`src/dr02_pro/docs/CAMERA.md` already warns that the serial-to-position mapping cannot be
determined from the serials and must be established empirically — that warning is correct and
should be followed. Two things to add:

- The serial in `/sys/.../serial` (a value like `350423023842`) is **not** the firmware serial
  the driver wants (a value like `254622074040`). Launching with the wrong one **does not
  fail** — the driver falls back to another device, so you silently open a different camera
  than you asked for. Prefer `usb_port_id`, which is positional and stable across reflashes.
- The empirical method is weaker than it sounds. A downward-angled camera showing bare floor
  looks much like any other downward-angled camera showing bare floor. **Someone who can see
  where each unit is bolted outranks any inference from the images**, and a mislabelled rear
  camera is a blind spot with a reassuring name on it — see the LiDAR note above for why the
  rear view specifically matters.
- If a colour pixel is to index the depth image, `align_depth.enable:=true` is required.
  Without it, sampling the depth image at a colour coordinate returns the range of whatever
  is at that pixel in a **different** camera: a plausible wrong answer rather than an error.

**An H.265 RTSP stream shows grey frames before it shows a picture.** The robot also serves a
forward-facing RGB camera over RTSP. Opened with default settings it connects, reports
1280x720, and returns a uniform grey frame that looks exactly like a covered lens. It is not:
the stream is H.265 and its parameter sets are sent infrequently, so no decoder can produce a
picture until one arrives, and UDP transport loses fragments while it waits. Set TCP transport
before the capture is opened, and **judge a frame by its variance, not by whether the read
succeeded** — the read returns success for the grey frames. Measured across one capture: frame
0 had `std=1.06`; from frame 20 onward `std≈68`.

Also, the lens is strongly wide-angle — a straight ceiling beam renders as a pronounced arc.
That matters for fiducial detection, which fits straight-edged quads: barrel distortion can
defeat the quad test while the marker stays perfectly legible to a person.

### 5.9 Build

`src/dr02_pro/CMakeLists.txt` sets `BUILD_PLATFORM` to `x86` by default with no detection, and
`BUILD_PLATFORM` selects the `third_party/onnxruntime/<platform>/` binaries. The robot-side
hosts are aarch64, so a bare `colcon build` there fails in the linker. `src/dr02_pro/docs/REAL_ROBOT.md`
gives the right invocation per host; if you are scripting it, derive the value from `uname -m`
rather than hardcoding a constant, so the same script works on a development host and on the
robot.

`BUILD_SIM=ON` must not be enabled for real-robot builds.

---

## 6. Lite3

**The Lite3 is not in this repository, and almost nothing above transfers to it.** Its
interfaces are the vendor's separate `Lite3_MotionSDK` and `Lite3_ROS` projects, and the
transport is **UDP datagrams to the motion host, not ROS 2 topics**. A coding agent that has
read [§5](#5-dr02-pro-and-std) and then meets a Lite3 has the wrong model of the machine in
almost every particular: no `/JOINTS_CMD`, no `/STEER`, no Developer Mode, a different joint
count, and a different answer to what happens when the sender dies.

This section is here so that the boundary is explicit rather than discovered.

**Provenance.** These observations come from two Lite3 Venture units during integration work
in **August–September 2026**, and are marked *[measured 2026-08/09]*. They are less complete
than the DR02 material and involve a stack that is not the vendor's: read them as places to
look. Where a value was never measured, that is said, because an unmeasured value is exactly
what this file exists to stop anyone inventing.

### 6.1 What carries across from §3, and what does not

**Carries across unchanged:** identify the unit before you write a line ([§3.1](#31-identify-the-robot-and-the-variant-before-you-write-a-line));
never invent a value ([§3.4](#34-gains-are-tables-quote-them-never-derive-them)); check the
payload, not the request ([§3.6](#36-check-the-payload-not-the-request)); never wrap a send in
a bare `try`/`except` ([§3.7](#37-never-wrap-a-publish-in-a-bare-tryexcept)); never weaken a
safety constant ([§3.8](#38-never-weaken-a-safety-constant-to-make-something-work)); the
operator's handset is the emergency stop ([§3.9](#39-nothing-in-your-program-is-an-emergency-stop)).

**Does not carry across:** every topic name, QoS note and joint table in
[§4](#4-transport-the-failures-that-produce-no-error) and [§5](#5-dr02-pro-and-std). And most
importantly the deadman behaviour — see [§6.5](#65-two-command-interfaces-two-different-answers-to-a-dead-sender).

### 6.2 Taking the low-level SDK means taking the balance controller

`Lite3_MotionSDK` exposes the twelve leg joints with position targets and gains. **Using it
replaces the vendor's own gait and balance controller** — the thing keeping the robot upright.
That is the same trade as DR02 Whole-Body mode ([§5.1](#51-developer-modes-decide-what-your-program-can-do-at-all)),
and the same answer applies: unless writing a balance controller is the actual task, stay on
the high-level velocity interface and let the vendor's policy have the legs.

The corollary is a real limitation and should be planned around rather than worked around:
**motor temperatures are not exposed on the high-level interface at all.** Software on that
interface cannot see accumulated heat. Let the robot cool physically between runs, and cap the
duration of any run you cannot thermally monitor.

### 6.3 There are two velocity interfaces, and the wrong one fails silently

*[measured 2026-08/09]* The Lite3 accepts velocity commands on UDP port `43893` in two
different framings, and **which one works depends on the robot's current mode, with no
distinguishing symptom when you pick wrong:**

- The **legacy velocity** framing (a fixed-size datagram carrying one `double` per axis)
  requires the vendor's **autonomous mode**. A unit in AI Motion Mode cannot enter autonomous
  mode. *[measured 2026-08]* A correctly formed `vx = 0.10 m/s` pulse arrived at the motion
  host, `error_state` stayed `0`, and the **world-pose delta was zero.** Nothing reported a
  rejection.
- The **simple-axis** framing (mode plus per-axis integer commands, driven at ≥20 Hz with a
  documented 250 ms receiver-side timeout) is the one that moved a robot.

**A packet that arrives and is accepted is not a packet that will be acted on.** As with
`/STEER` on a DR02 ([§5.6](#56-steer-preconditions-and-a-header-finding-we-cannot-fully-explain)),
the precondition lives in the robot's mode, and the failure is silence. Read the state stream
and confirm the mode before concluding your command was wrong.

### 6.4 A sign-only velocity mapping silently invalidates every speed measurement

*[measured 2026-08/09]* **This is a trap in the integration rather than in the robot, and it
is recorded because it is the Lite3 equivalent of the zero-gain trap: a whole class of
measurement that looks like it worked and measured nothing.**

The vendor's axis interface carries an integer magnitude per axis, so it *can* express speed.
But a client can only send a magnitude it has evidence for, and if exactly one raw value per
direction has ever been observed to produce a known displacement, then that is the only value
the client can honestly emit. The mapping collapses to the **sign** of the requested velocity.
A request for 0.05 m/s and a request for 0.30 m/s then put the identical value on the wire,
and the robot walks at whatever speed that one primitive produces.

None of that is wrong as a conservative design — inventing intermediate magnitudes would be
worse. **What is dangerous is forgetting it upstream**, because nothing downstream reports that
the requested magnitude was discarded.

Three consequences that each produce a plausible wrong answer rather than an error:

- **A speed ladder measures the ladder, not the robot.** Every rung above the dead zone walks
  at the same speed, so the probe "passes" at its lowest rung and reports that rung as the
  robot's minimum gait speed. The number describes the script.
- **A planner that assumes commanded velocity equals executed velocity commits the wrong
  distance.** A policy creeping at 0.05 m/s validated 0.125 m of travel while the legs
  actually covered 0.75 m in the same window. The robot goes where the primitive takes it, not
  where the plan says.
- **A dead zone must be applied to the velocity vector, not per axis.** Gate `hypot(vx, vy)`
  and snap to a bearing. Applied per axis, a diagonal request just below the forward dead zone
  and just above the lateral one zeroes forward and fires a **full-speed 90° strafe**.

Also: *[measured 2026-08/09]* on the vendor axis convention, **positive is right** for lateral
and yaw. If your navigation stack uses positive-left, invert exactly once, at the transport
boundary, and test it — a sign error here is a robot walking confidently the wrong way with no
diagnostic.

**What was never measured, and must not be invented.** Forward and lateral speed ceilings, the
yaw rate in physical units, and the minimum gait speed were not established on these units.
Requiring these as explicit arguments with **no default** is the right design: an earlier
version silently defaulted them to *a different robot's* published figures, which is how a
number with no relationship to the machine in front of you ends up bounding its motion.

### 6.5 Two command interfaces, two different answers to a dead sender

*[measured 2026-08/09]* **The legacy velocity interface has no heartbeat and no receiver-side
timeout: a command latches until something explicitly sends zero.** The axis interface has a
documented 250 ms timeout and does stop.

So on the latching interface, killing the controlling process outright is **the opposite of a
stop** — it is the one action that guarantees your cleanup never runs and your zero is never
sent. This inverts the standard advice, and it inverts what the DR02 measurement in
[§3.5](#35-a-velocity-command-is-renewed-not-set) would lead you to expect. Send zeros
repeatedly on the way out, for a bounded interval, in a path that runs on every exit; and treat
the operator's handset, not any software path, as the stop that is guaranteed.

### 6.6 The state stream: check the frame length, and where it is being sent

*[measured 2026-08/09]* Two Lite3 units in the same fleet emitted **state frames of different
lengths — 220 bytes on one, 212 on the other.** The shorter frame omits one field, which
shifts every subsequent field by eight bytes.

**Decoding the short frame with the long layout does not crash.** It shears every field into
its neighbour: battery reads a calm, plausible `0.0`, and gravity lands on the wrong axis.
Key the decoder on the **exact** frame length and refuse an unknown one; a tolerant parse here
is a parse that lies. And decode a field the firmware never sent as *absent*, never as a
substituted `0` — a downstream gate that reads `0` as "measured and fine" will let the robot
move on a value that does not exist.

*[measured 2026-08/09]* The state stream also goes to **exactly one destination address**,
configured on the robot (`~/jy_exe/conf/network.toml`, port `43897`). If that address is not
the host you are debugging from, every tool reports the robot as silent for a reason that has
nothing to do with the robot. This was the first trap of the first deployment, and it is worth
checking before anything else.

*[measured 2026-08/09]* One more, and it is a trap of the same family as §5.6's decimal-vs-hex
confusion: **the angular-rate unit is ambiguous.** The ROS bridge copies the vendor's
`rpy_vel` into a ROS field with no stated conversion, while the low-level SDK documents
angular rate in **degrees per second**. Resolve it empirically on the firmware in front of
you — divide the observed yaw-pose change by the reported rate: about `1` means degrees per
second, about `57.3` means radians per second. Do not use a measured yaw rate downstream until
that check has been done.

### 6.7 Two robots, one address: never default to another robot's IP

*[measured 2026-09-02]* Units ship with the same factory address on the motion host. In one
fleet, a default motion-host address was left at the first robot's IP. On robot 1 that
"worked", because it pointed at itself — which is exactly what hid the problem. On robot 2 the
same default pointed **across the network at robot 1**: robot 2 reported `37/37 ticks driven,
moved 0.00 m` while **robot 1 walked in response**.

Two rules fall straight out of that:

- Anything that **runs on** the robot it drives should target `127.0.0.1`, not the robot's own
  LAN address.
- Anything that runs **off** the robot must name its target explicitly, with no default. A
  wrong explicit address fails loudly. A wrong default reaches a live robot.

Readdress units before putting more than one on a shared network.

### 6.8 A network change can silently disable the emergency stop

**This is the most serious item in this section.** *[measured 2026-08/09]* The vendor handset
— which is the emergency stop — connects through an access point on a virtual interface that
shares radio hardware with the robot's station interface. That AP profile ships with
autoconnect disabled, so **applying a network configuration change deactivates the AP and it
does not come back.**

Every service stays running. The robot stays reachable over Ethernet and WiFi. The only
symptom is that **the handset can no longer find its SSID**, which reads as a hardware radio
fault rather than as a consequence of a network change made an hour earlier. It cost an hour
on each of two robots, a day apart — and for that hour the operator's stop was not there.

Check the AP interface is up and in AP mode **before and after** any network change, and never
make one between the pre-run check and the run. Note also that the radio permits a **single
channel across the AP and the station connection combined**, so the handset's channel and the
site network's channel are not independent — a "tidy" split of the two bands breaks handset
pairing outright.

### 6.9 RGB only: what the robot cannot see

The Lite3 Venture used here carried **one forward RGB camera: no LiDAR, no depth, no onboard
map.** Range came from apparent size against known object dimensions.

**There is no lateral or rear sensing at all.** Where the DR02's LiDAR at least sees ±120°
([§5.8](#58-sensors)), this robot sees a single forward cone and nothing else. Clear both
sides of the lane, not only the ends, and treat any object outside that cone — including a
second, powered-off robot parked nearby — as something the robot cannot see rather than
something it will avoid.

Three camera-path traps *[measured 2026-08/09]*, all of which present as something other than
what they are:

- **The camera device is owned by the vendor's own publisher.** Do not compete for the device
  node; consume the local stream it already serves.
- **Decode-time timestamps make stale frames look fresh.** OpenCV stamps a frame when it is
  decoded, not when the shutter fired, so a buffered stream can hand you a several-second-old
  frame that reports as current. Serve only the newest frame from a background reader, and
  measure end-to-end frame age per installation rather than trusting the timestamp.
- **A tight retry loop on a dead stream starves everything else in the process.** Re-opening a
  capture in a loop holds the interpreter lock across each blocking call: the port keeps
  listening, every request times out, and the log stays empty. It reads as a network fault and
  is a missing sleep.

### 6.10 Mode authority stays with the operator

AI Motion Control Mode is enabled by the vendor on a specific physical unit. **Do not attempt
to bypass, alter or reverse-engineer that activation.** There is also no documented high-level
"stand down": a program's cleanup can only send zeros, and returning the robot to a resting
posture goes through the vendor's own interface, operated by a person.

Write code that **refuses** an unexpected mode rather than correcting it. On this platform the
robot's posture and mode are the operator's, and a program that quietly transitions the robot
is a program that moved it while nobody expected motion.

---

## 7. Adding another robot to this file

Keep the shape. **[§3](#3-common-rules--every-deeprobotics-robot) is common to every robot**
and should hold whatever arrives next.
**[§4](#4-transport-the-failures-that-produce-no-error) is common to the ROS 2 / DDS
products** and applies to any robot that speaks `drdds`. Everything else belongs in a section
of its own, holding only the values that differ.

A product section is worth writing when it can answer these, with a citation to a file in this
repository or a measurement with a date:

1. **How do I tell this robot apart from its siblings, from the wire?**
2. **What are the control modes, and which one can do the task I have?**
3. **What is the joint index table, and what is the full command payload?**
4. **What are the gains, and where is the authoritative array?**
5. **What must already be true before a motion command means anything?**
6. **What fails silently — accepted, or delivered, and ignored?**

Two conventions that keep the file honest:

- **Cite the file, or date the measurement.** Every number here is either quoted from a path in
  this repository or marked with the date it was observed on a robot. A number with neither is
  a number nobody can check.
- **Say what you did not test.** An unverified statement clearly labelled is useful. An
  unverified statement presented as fact is the exact failure this whole file is about.
