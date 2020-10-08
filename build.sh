#!/bin/bash -e

function print_help() {
  echo """
usage: build.sh -t <L4T toolchain>

Creates the CBoot (lk; little kernel) binary.

  -t : path to the Jetson Driver Linux toolchain (i.e. parent directory to bin/aarch64-linux-gnu-)
  -z : (optional) leave the make output directory instead of cleaning it up
"""
}

# Clean-up the temporary working directory on any exit
function cleanup() {
  if [ -n "${ARG_CLEAN}" ]; then
    echo "<< Cleaning up... >>"
    rm -rf "${C_WORK}"
    echo "<< Cleaning up...Done >>"
  else
    echo "<< Cleaning up...Skipped >>"
  fi
}

trap cleanup EXIT

# input arguments
ARG_TOOLCHAIN=
ARG_CLEAN=1
while getopts "t:z?" opt; do
  case $opt in
    t) ARG_TOOLCHAIN=$OPTARG
      ;;
    z) ARG_CLEAN=
      ;;
    ?|*)
      print_help
      exit 1
      ;;
  esac
done

# constant string definitions
C_TC_BIN_PATH="/bin/aarch64-linux-gnu-"
C_WORK=`pwd`/"out"
C_WORK_LK="${C_WORK}/build-t194/lk.bin"
C_OUT_CBOOT=`pwd`/cboot_t194.bin
C_L4T_CBOOT=Linux_for_Tegra/bootloader/cboot_t194.bin
C_MAKE_PATH=./bootloader/partner/t18x/cboot
C_MAKE_DEBUG=2
C_MAKE_PROJECT=t194
C_MAKE_BOARD=t194ref
C_MAKE_TYPE=l4t
C_MAKE_TEGRA_TOP=`pwd`

CROSS_COMPILE_AARCH64="${ARG_TOOLCHAIN}${C_TC_BIN_PATH}"
if [ ! -f "${CROSS_COMPILE_AARCH64}gcc" ]; then
  echo "Error: Toolchain option (-t) must be a directory path containing aarch64-linux-gnu-* files"
  print_help
  exit 1
fi

if [ -n "${ARG_CLEAN}" ]; then
  cleanup
fi

echo "<< Build CBoot >>"
make -C "${C_MAKE_PATH}" PROJECT="${C_MAKE_PROJECT}" NOECHO=@ \
  TOOLCHAIN_PREFIX="${CROSS_COMPILE_AARCH64}" DEBUG="${C_MAKE_DEBUG}" \
  BUILDROOT="${C_WORK}" NV_TARGET_BOARD="${C_MAKE_BOARD}" \
  NV_BUILD_SYSTEM_TYPE="${C_MAKE_TYPE}" TEGRA_TOP="${C_MAKE_TEGRA_TOP}"

echo "<< Copy ${C_WORK_LK} -> ${C_OUT_CBOOT} >>"
cp ${C_WORK_LK} ${C_OUT_CBOOT}

echo
echo "How to flash this CBoot:"
echo " 1) Copy ${C_OUT_CBOOT} to the flash directory (i.e. Linux_for_Tegra/bootloader/cboot_t194.bin)"
echo " 2) Flash the bootloader partition (i.e. sudo ./flash.sh -r -k cpu-bootloader cti/Xavier-NX/photon mmcblk0p1)"
echo
