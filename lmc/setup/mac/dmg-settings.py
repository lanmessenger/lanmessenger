#!/usr/bin/env python3
"""dmgbuild settings for the LAN-Messenger macOS disk image.

Builds the distributable dmg in one step: app bundle, Applications
symlink, Finder window layout (icon positions, background picture)
and the software license agreement.

dmgbuild synthesizes the .DS_Store itself and never scripts the
Finder, so the detach/convert flake class of the old
createdisk+addlicense scripts cannot occur here.

Usage (from lmc/setup/mac, after stage-mac-app prepared the bundle):
    dmgbuild -s dmg-settings.py "LAN-Messenger" "../lmc_<ver>_<arch>.dmg"
"""
import os

# dmgbuild exec()s this file without providing __file__
# (core.py load_settings), so resolve everything from the cwd.
# Must be run from lmc/setup/mac, as documented above and in CI.
MACDIR = os.getcwd()

volume_name = 'LAN-Messenger'
format = 'UDBZ'
filesystem = 'HFS+'

files = [os.path.join(MACDIR, 'LAN-Messenger.app')]
symlinks = {'Applications': '/Applications'}

# Former createdisk osascript layout, now declarative.
icon_locations = {
    'LAN-Messenger.app': (160, 205),
    'Applications': (360, 205),
}
icon_size = 72
default_view = 'icon-view'
# Old Finder bounds were {400, 100, 950, 470} in top-left origin;
# dmgbuild takes ((x, y), (w, h)) with y running bottom-up, and the
# Finder clamps the window onto the display anyway.
window_rect = ((400, 300), (550, 370))
show_status_bar = False
show_tab_view = False
show_toolbar = False
show_pathbar = False
show_sidebar = False
background = os.path.join(MACDIR, 'package', 'images', 'background.jpg')

license = {
    'default-language': 'en_US',
    'licenses': {
        'en_US': os.path.join(MACDIR, 'package', 'eula', 'license.txt'),
    },
}
