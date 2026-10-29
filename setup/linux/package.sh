#!/bin/sh
set -e

# Single mode: system Qt only. The _min suffix in artifact names is legacy,
# kept for artifact stability (see build.yml globs and verify workflows).
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

# Drop stale artifacts: a rerun with another version would leave two
# files behind, and the build.yml globs plus verify [0]-pickup turn
# ambiguous. Two patterns needed: deb names use underscores,
# rpm names use dashes (ecosystem filename conventions).
rm -f ../lmc_*_min.deb ../lmc-*_min.rpm

echo "BUILDING DEB..."
LMC_ARCH="$ARCH" LMC_VERSION="$1" LMC_RELEASE="" "$NFPM" package -p deb -f nfpm.yaml -t "../lmc_$1_${ARCH}_min.deb"

echo "BUILDING RPM..."
LMC_ARCH="$ARCH" LMC_VERSION="$1" LMC_RELEASE="1" "$NFPM" package -p rpm -f nfpm.yaml -t "../lmc-$1.${ARCH}_min.rpm"

echo "DONE."
exit 0
