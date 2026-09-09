#!/usr/bin/env bash
# Build and install the upstream drdds Debian package on the current host.
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: scripts/install_deep_robotics_msg.sh [--ref TAG_OR_COMMIT] [--jobs N]

Install deep-robotics-msg (ROS package drdds) from upstream source.
Supports Ubuntu 22.04 / ROS 2 Humble and Ubuntu 24.04 / ROS 2 Jazzy,
on amd64 and arm64. The matching ROS 2 installation and its apt repository
must already be configured. This is a native build, not cross-compilation.

  --ref REF   Build a branch, tag, or full commit SHA (default: main)
  --jobs N    Maximum parallel compiler jobs (default: 2)
  -h, --help  Show this help

Uses sudo for apt dependencies and Debian package installation only. Sources,
build outputs, and the generated .deb are temporary and removed on exit.
Installs into /opt/ros/<distro>; does not edit shell startup files.
EOF
}

die() { printf 'Error: %s\n' "$*" >&2; exit 1; }

ref=main
jobs=2
while (($#)); do
    case "$1" in
        --ref|--jobs)
            (($# >= 2)) || die "$1 requires a value"
            case "$1" in
                --ref) ref=$2 ;;
                --jobs) jobs=$2 ;;
            esac
            shift 2
            ;;
        -h|--help) usage; exit 0 ;;
        *) die "Unknown argument: $1 (see --help)" ;;
    esac
done
[[ -n "$ref" && "$ref" != -* ]] || die 'Invalid source reference'
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || die '--jobs must be a positive integer'

[[ -r /etc/os-release ]] || die 'Cannot identify the operating system'
# shellcheck disable=SC1091
source /etc/os-release
[[ "${ID:-}" == ubuntu ]] || die 'Only Ubuntu 22.04 and 24.04 are supported'
case "${VERSION_ID:-}" in
    22.04) distro=humble ;;
    24.04) distro=jazzy ;;
    *) die "Unsupported Ubuntu version: ${VERSION_ID:-unknown}" ;;
esac
arch=$(dpkg --print-architecture)
case "$arch" in
    amd64|arm64) ;;
    *) die "Unsupported architecture: $arch (expected amd64 or arm64)" ;;
esac
ros_setup="/opt/ros/$distro/setup.bash"
[[ -r "$ros_setup" ]] || die "Install ROS 2 $distro first; missing $ros_setup"

privilege=()
if ((EUID != 0)); then
    command -v sudo >/dev/null || die 'sudo is required to install system packages'
    privilege=(sudo)
fi

printf 'Installing deep-robotics-msg for Ubuntu %s, ROS 2 %s, %s (ref: %s)\n' \
    "$VERSION_ID" "$distro" "$arch" "$ref"
"${privilege[@]}" apt-get update
"${privilege[@]}" apt-get install -y --no-install-recommends \
    ca-certificates git build-essential cmake python3-colcon-common-extensions \
    "ros-$distro-ament-cmake" "ros-$distro-rosidl-default-generators" \
    "ros-$distro-rosidl-default-runtime" "ros-$distro-builtin-interfaces" \
    "ros-$distro-ros2cli" "ros-$distro-ros2interface"

work_dir=$(mktemp -d /tmp/deep-robotics-msg.XXXXXXXX)
trap 'rm -rf -- "$work_dir"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
git init -q "$work_dir/src"
git -C "$work_dir/src" remote add origin https://github.com/DeepRoboticsLab/deep-robotics-msg.git
git -C "$work_dir/src" fetch --depth 1 origin "$ref"
git -C "$work_dir/src" checkout --detach FETCH_HEAD
printf 'Building upstream commit: %s\n' "$(git -C "$work_dir/src" rev-parse HEAD)"

# Isolate the build from activated SDK overlays, other ROS distributions, and
# virtual environments. ROS setup scripts are not guaranteed to be nounset-safe.
env -i HOME="$HOME" PATH=/usr/bin:/bin LANG=C.UTF-8 \
    CMAKE_BUILD_PARALLEL_LEVEL="$jobs" MAKEFLAGS="-j$jobs" \
    /bin/bash --noprofile --norc -s -- "$ros_setup" "$work_dir" <<'BUILD'
set -eo pipefail
source "$1"
cd "$2"
colcon --log-base log build --base-paths src --packages-select drdds \
    --executor sequential --cmake-args -DBUILD_DEB=ON -DBUILD_TESTING=OFF \
    -DPython3_EXECUTABLE=/usr/bin/python3 -DPYTHON_EXECUTABLE=/usr/bin/python3
cpack --config build/drdds/CPackConfig.cmake -B build
BUILD

shopt -s nullglob
packages=("$work_dir"/build/deep-robotics-msgs_*_"$arch".deb)
((${#packages[@]} == 1)) || die 'Expected exactly one Debian package for this architecture'
[[ $(dpkg-deb -f "${packages[0]}" Architecture) == "$arch" ]] || die 'Package architecture mismatch'
[[ $(dpkg-deb -f "${packages[0]}" Package) == deep-robotics-msgs ]] || die 'Unexpected Debian package name'
"${privilege[@]}" dpkg -i "${packages[0]}"

env -i HOME="$HOME" PATH=/usr/bin:/bin LANG=C.UTF-8 \
    /bin/bash --noprofile --norc -s -- "$ros_setup" <<'VERIFY'
set -eo pipefail
source "$1"
ros2 interface show drdds/msg/Joints >/dev/null
ros2 interface show drdds/srv/StdSrvInt32 >/dev/null
VERIFY

printf '\nInstalled and verified drdds. Before building or running the SDK, run:\n  source %s\n' "$ros_setup"
