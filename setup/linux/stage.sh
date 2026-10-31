#!/bin/sh
set -e

# Run relative to this script so ../../release and ./package resolve no
# matter where the caller invokes us from (mirrors package.sh).
cd "$(dirname "$0")"

echo "COPYING FILES..."
# Copy the shared payload; the lmc.sh launcher stays tracked in package/
# (it differs from the portable lan-messenger.sh in release/).
# cp -r merges dirs: a .qm/sound removed from release/ would survive in
# package/ and leak into both packages on re-staging. Single files below
# are overwritten by cp, only the trees need a reset.
rm -rf ./package/usr/lib/lmc/lang ./package/usr/lib/lmc/sounds
cp -v ../../release/lmc.rcc ./package/usr/lib/lmc/lmc.rcc
cp -v ../../release/lan-messenger ./package/usr/lib/lmc/lan-messenger
cp -v ../../release/COPYING ./package/usr/lib/lmc/COPYING
cp -v -r ../../release/lang ./package/usr/lib/lmc
cp -v -r ../../release/sounds ./package/usr/lib/lmc
chmod 755 ./package/usr/lib/lmc/lmc.sh

echo "DONE."
exit 0
