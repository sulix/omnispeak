#!/usr/bin/env bash
set -euo pipefail

# This script checks out and configures buildroot for RG350.
# This only needs to be run once in order to build omnispeak for RG350.
BUILDROOT_GIT=https://github.com/tonyjih/RG350_buildroot.git
BUILDROOT_DIR=`dirname "$0"`/../buildroot-rg350

echo "Cloning BuildRoot..."
git clone --depth=1 "${BUILDROOT_GIT}" "${BUILDROOT_DIR}"
cd "${BUILDROOT_DIR}"

echo "Configuring BuildRoot..."
make rg350_defconfig BR2_EXTERNAL=board/opendingux

echo "Building toochain..."
BR2_JLEVEL=0 make toolchain sdl2

echo "All done!"
cd -



