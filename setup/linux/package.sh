#!/bin/sh
set -e

# Single mode: system Qt only. Artifact names follow each ecosystem's
# conventional layout (nfpm ConventionalFileName):
#   deb:  lmc_<ver>_amd64.deb
#   rpm:  lmc-<ver>-1.<arch>.rpm
#   arch: lmc-<archver>-1-<arch>.pkg.tar.zst
if [ "$1" = "" ]; then
  echo "Usage: package.sh <App Version>"
  echo "eg: package.sh 1.0.0"
  exit 1
fi

if [ -z "$NFPM" ]; then
  NFPM=nfpm
fi

cd "$(dirname "$0")"

ARCH=$(uname -m)
case "$ARCH" in
  x86_64) ;;
  *) echo "unsupported arch: $ARCH (only x86_64 for now)" >&2; exit 1 ;;
esac
# Debian arch name for the filename; nfpm maps x86_64 -> amd64 in metadata
# on its own, here we spell the conventional filename explicitly.
DEB_ARCH=amd64
# Arch pkgver forbids `~` and `-` (nfpm would silently strip them, merging
# e.g. 1.2.40~nightly.N into 1.2.40nightly.N). Map both to `_` explicitly.
# Keep in sync with ver_san in nightly.yml (Normalize asset names).
# Nightly-only caveat: 1.2.40_nightly.N sorts NEWER than release 1.2.40.
ARCH_VER=$(printf '%s' "$1" | sed 's/[~-]/_/g')

# Drop stale artifacts: a rerun with another version would leave two
# files behind, and the build.yml globs plus verify [0]-pickup turn
# ambiguous. Three patterns needed: deb names use underscores,
# rpm/arch names use dashes (ecosystem filename conventions).
rm -f ../lmc_*.deb ../lmc-*.rpm ../lmc-*.pkg.tar.zst

echo "BUILDING DEB..."
LMC_ARCH="$ARCH" LMC_VERSION="$1" LMC_RELEASE="" "$NFPM" package -p deb -f nfpm.yaml -t "../lmc_$1_${DEB_ARCH}.deb"

echo "BUILDING RPM..."
LMC_ARCH="$ARCH" LMC_VERSION="$1" LMC_RELEASE="1" "$NFPM" package -p rpm -f nfpm.yaml -t "../lmc-$1-1.${ARCH}.rpm"

echo "BUILDING ARCH..."
LMC_ARCH="$ARCH" LMC_VERSION="$ARCH_VER" LMC_RELEASE="1" "$NFPM" package -p archlinux -f nfpm.yaml -t "../lmc-${ARCH_VER}-1-${ARCH}.pkg.tar.zst"

echo "DONE."
exit 0
