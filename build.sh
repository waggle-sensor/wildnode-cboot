#!/bin/bash -e

function print_help() {
  echo """
usage: build.sh -t <L4T toolchain> [-f] [-r]

Creates the CBoot (lk; little kernel) binary.

  -t : path to the Jetson Driver Linux toolchain (i.e. parent directory to bin/aarch64-linux-gnu-)
  -z : (optional) leave the make output directory instead of cleaning it up
  -r : (optional) create a release using `release_manifest`. Ignores all other input parameters.
  -f : force the build to proceed (debugging only) without checking for tagged commit
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
ARG_CREATE_RELEASE=
FORCE=
while getopts "t:zrf?" opt; do
  case $opt in
    t) ARG_TOOLCHAIN=$(realpath $OPTARG)
      ;;
    z) ARG_CLEAN=
      ;;
    r) ARG_CREATE_RELEASE=1
      echo "** CREATING RELEASE **"
      C_RELEASE_FILE=$(realpath ./release_manifest)
      ARG_TOOLCHAIN=
      ;;
    f) # force build
      echo "** Force build: ignore tag depth check **"
      FORCE=1
      ;;
    ?|*)
      print_help
      exit 1
      ;;
  esac
done

# create version string
PROJ_VERSION=$(git describe --tags --long --dirty | cut -c2-)

TAG_DEPTH=$(echo ${PROJ_VERSION} | cut -d '-' -f 2)
if [[ -z "${FORCE}" && "${TAG_DEPTH}_" != "0_" ]]; then
  echo "Error:"
  echo "  The current git commit has not been tagged. Please create a new tag first to ensure a proper unique version number."
  echo "  Use -f to ignore error (for debugging only)."
  exit 1
fi

# Peform steps to create release using the release manifest
if [ -n "${ARG_CREATE_RELEASE}" ]; then
    source "${C_RELEASE_FILE}"

    echo "Download build toolchain (${R_TOOLCHAIN} [${R_TOOLCHAIN_MD5}])"
    wget -c "${R_TOOLCHAIN}"
    DMD5=$(md5sum $(basename $R_TOOLCHAIN) | cut -d' ' -f1)
    if [ "${DMD5}" != "${R_TOOLCHAIN_MD5}" ]; then
        echo "Error: Unable to process release. Download [${R_TOOLCHAIN}]" \
              "checksum [md5: ${DMD5}] does not match expected value [md5: ${R_TOOLCHAIN_MD5}]." \
              "Exiting."
        exit 1
    fi
    tar xf $(basename $R_TOOLCHAIN)
    ARG_TOOLCHAIN=$(realpath $(basename $R_TOOLCHAIN .tar.xz))

    echo "Creating release [${C_RELEASE_FILE}]"
    echo " - Toolchain:  ${ARG_TOOLCHAIN}"
fi

# constant string definitions
C_TC_BIN_PATH="/bin/aarch64-linux-gnu-"
C_WORK=`pwd`/"out"
C_WORK_LK="${C_WORK}/build-t194/lk.bin"
C_OUT_CBOOT=`pwd`/cboot_t194_"${PROJ_VERSION}".bin
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
