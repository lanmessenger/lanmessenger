#!/bin/sh
set -e

# Run relative to this script so ../../release and ./package resolve no
# matter where the caller invokes us from (mirrors package.sh).
cd "$(dirname "$0")"

echo "COPYING FILES..."
# ELF goes to /usr/bin, data to /usr/share/lmc. Trees are reset first:
# a .qm/sound removed from release/ would survive in package/ and leak
# into all packages on re-staging. The stale /usr/lib/lmc tree from
# pre-FHS packages is dropped as well.
rm -rf ./package/usr/share/lmc ./package/usr/bin/lmc ./package/usr/lib/lmc
rmdir ./package/usr/lib 2>/dev/null || true
mkdir -p ./package/usr/bin ./package/usr/share/lmc
install -m755 ../../release/lmc ./package/usr/bin/lmc
cp -v ../../release/lmc.rcc ./package/usr/share/lmc/lmc.rcc
cp -v ../../release/COPYING ./package/usr/share/lmc/COPYING
cp -v -r ../../release/lang ./package/usr/share/lmc
cp -v -r ../../release/sounds ./package/usr/share/lmc

echo "DONE."
exit 0
