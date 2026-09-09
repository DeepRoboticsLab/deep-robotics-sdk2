#!/usr/bin/env bash
# Build and install the upstream drdds Debian package on the current host.
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: scripts/install_deep_robotics_msg.sh [--ref REF | --source-dir DIR] [--jobs N]

Install deep-robotics-msg (ROS package drdds) from upstream source.
Supports Ubuntu 22.04 / ROS 2 Humble and Ubuntu 24.04 / ROS 2 Jazzy,
on amd64 and arm64. The matching ROS 2 installation must already be present.
This is a native build, not cross-compilation.

  --ref REF   Build a branch, tag, or full commit SHA (default: main)
  --source-dir DIR  Build a local source tree instead of downloading from GitHub
  --jobs N    Maximum parallel compiler jobs (default: 2)
  -h, --help  Show this help

Checks dependencies and reports any that are missing; never installs them.
Uses sudo only to install the built Debian package. Downloaded sources,
build outputs, and the generated .deb are temporary and removed on exit.
With --source-dir, the original source tree is preserved; a copy is built.
With --source-dir, no download or internet access is needed.
Installs into /opt/ros/<distro>; does not edit shell startup files.
EOF
}

die() { printf 'Error: %s\n' "$*" >&2; exit 1; }

ref=main
ref_set=false
source_dir=
jobs=2
while (($#)); do
    case "$1" in
        --ref|--jobs|--source-dir)
            (($# >= 2)) || die "$1 requires a value"
            case "$1" in
                --ref) ref=$2; ref_set=true ;;
                --jobs) jobs=$2 ;;
                --source-dir) [[ -n "$2" ]] || die '--source-dir requires a directory'; source_dir=$2 ;;
            esac
            shift 2
            ;;
        -h|--help) usage; exit 0 ;;
        *) die "Unknown argument: $1 (see --help)" ;;
    esac
done
[[ -n "$ref" && "$ref" != -* ]] || die 'Invalid source reference'
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || die '--jobs must be a positive integer'
if [[ -n "$source_dir" ]]; then
    [[ "$ref_set" == false ]] || die '--source-dir and --ref cannot be combined'
    source_dir=$(cd -- "$source_dir" && pwd -P) || die 'Cannot access source directory'
    for required in package.xml CMakeLists.txt packaging.cmake msg srv; do
        [[ -e "$source_dir/$required" ]] || die "Missing source file or directory: $required"
    done
fi

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
    command -v sudo >/dev/null || die 'sudo is required to install the built Debian package'
    privilege=(sudo)
fi

printf 'Installing deep-robotics-msg for Ubuntu %s, ROS 2 %s, %s\n' \
    "$VERSION_ID" "$distro" "$arch"
if [[ -z "$source_dir" ]]; then
    command -v git >/dev/null || die 'Missing dependency: git (needed to download the source)'
fi

# Report missing prerequisites in the same clean environment used for the build.
env -i HOME="$HOME" PATH=/usr/bin:/bin LANG=C.UTF-8 \
    /bin/bash --noprofile --norc -s -- "$ros_setup" <<'CHECK'
set -eo pipefail
source "$1"
missing=()
for tool in cc c++ make cmake cpack colcon python3 ros2 tar dpkg-deb; do
    command -v "$tool" >/dev/null || missing+=("$tool")
done
if ((${#missing[@]})); then
    printf 'Error: required tools are missing: %s\n' "${missing[*]}" >&2
    printf 'Provision these tools on the target before running the installer.\n' >&2
    exit 1
fi
python3 - <<'PY'
import sys
try:
    from ament_index_python.packages import get_package_prefix, PackageNotFoundError
except ImportError:
    sys.exit('Error: ament_index_python is missing from the ROS installation.')
missing = []
for package in ('ament_cmake', 'rosidl_default_generators', 'rosidl_default_runtime',
                'builtin_interfaces', 'ros2cli', 'ros2interface'):
    try:
        get_package_prefix(package)
    except PackageNotFoundError:
        missing.append(package)
if missing:
    sys.exit('Error: required ROS packages are missing: ' + ', '.join(missing)
             + '. Provision them on the target before running the installer.')
PY
CHECK

work_dir=$(mktemp -d /tmp/deep-robotics-msg.XXXXXXXX)
trap 'rm -rf -- "$work_dir"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
if [[ -n "$source_dir" ]]; then
    printf 'Building local source: %s\n' "$source_dir"
    mkdir "$work_dir/src"
    # Copy sources without changing the original or reusing host build outputs.
    tar -C "$source_dir" --exclude='./.git' --exclude='./build' \
        --exclude='./install' --exclude='./log' -cf - . | tar -C "$work_dir/src" -xf -
else
    git init -q "$work_dir/src"
    git -C "$work_dir/src" remote add origin https://github.com/DeepRoboticsLab/deep-robotics-msg.git
    git -C "$work_dir/src" fetch --depth 1 origin "$ref"
    git -C "$work_dir/src" checkout --detach FETCH_HEAD
    printf 'Building upstream commit: %s\n' "$(git -C "$work_dir/src" rev-parse HEAD)"
fi

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
