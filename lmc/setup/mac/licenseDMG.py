#!/usr/bin/env python3
"""
Adds a license file to a DMG (embeds 'TEXT' and 'STR#' resources).
Requires the Rez tool from the Xcode command line tools. Only runs on Mac.
License embedding is best-effort: on failure the DMG is left as-is.
"""
import os
import sys
import shutil
import subprocess
import tempfile

STR_ITEMS = [
    "English",
    "Agree",
    "Disagree",
    "Print",
    "Save...",
    "IMPORTANT - By clicking on the \"Agree\" button, you agree "
    "to be bound by the terms of the License Agreement.",
    "Software License Agreement",
    "This text cannot be saved. This disk may be full or locked, or the "
    "file may be locked.",
    "Unable to print. Make sure you have selected a printer.",
]

LPIC_HEX = """    $"0002 0011 0003 0001 0000 0000 0002 0000"
    $"0008 0003 0000 0001 0004 0000 0004 0005"
    $"0000 000E 0006 0001 0005 0007 0000 0007"
    $"0008 0000 0047 0009 0000 0034 000A 0001"
    $"0035 000B 0001 0020 000C 0000 0011 000D"
    $"0000 005B 0004 0000 0033 000F 0001 000C"
    $"0010 0000 000B 000E 0000"
"""


def find_rez():
    xcrun = shutil.which('xcrun')
    if xcrun:
        try:
            out = subprocess.check_output([xcrun, '--find', 'Rez'],
                                          stderr=subprocess.DEVNULL)
            path = out.decode().strip()
            if path and os.path.exists(path):
                return path
        except (subprocess.CalledProcessError, OSError):
            pass
    for candidate in ('/usr/bin/Rez', '/Developer/Tools/Rez'):
        if os.path.exists(candidate):
            return candidate
    return None


def printUsage():
    print("This program adds a software license agreement to a DMG file.")
    print("It requires the Rez tool (Xcode command line tools).\n")
    print("Usage: %s <dmgFile> <licenseFile>" % sys.argv[0])
    print("The <licenseFile> must be a plain ascii text file.")
    sys.exit(1)


def str_resource_hex():
    body = '%04x' % len(STR_ITEMS)
    for item in STR_ITEMS:
        raw = item.encode('mac_roman', 'replace')[:255]
        body += '%02x' % len(raw) + raw.hex()
    chunks = [body[i:i + 32] for i in range(0, len(body), 32)]
    return '\n'.join('    $"%s"' % c for c in chunks)


def write_resource_file(path, license):
    with open(path, 'w') as f:
        f.write("data 'LPic' (5000) {\n%s};\n\n" % LPIC_HEX)
        with open(license, 'r') as lic:
            f.write('data \'TEXT\' (5002, "English") {\n')
            for line in lic:
                esc = line.strip().replace('\\', '\\\\').replace('"', '\\"')
                f.write('    "' + esc + '\\n"\n')
            f.write('};\n\n')
        f.write('data \'STR#\' (5002, "English") {\n%s};\n' % str_resource_hex())


def main(dmgFile, license):
    rez = find_rez()
    if not rez:
        print("WARNING: Rez not found, skipping license embedding",
              file=sys.stderr)
        return 0

    fd, tmpFile = tempfile.mkstemp(dir='.', suffix='.r')
    os.close(fd)
    try:
        write_resource_file(tmpFile, license)
        try:
            subprocess.check_call(['/usr/bin/hdiutil', 'unflatten', '-quiet',
                                   dmgFile])
            try:
                subprocess.check_call([rez, tmpFile, '-a', '-o', dmgFile])
            finally:
                subprocess.check_call(['/usr/bin/hdiutil', 'flatten', '-quiet',
                                       dmgFile])
        except (subprocess.CalledProcessError, OSError) as exc:
            print("WARNING: license embedding failed: %s" % exc,
                  file=sys.stderr)
            return 0
        print("Successfully added license to '%s'" % dmgFile)
        return 0
    finally:
        if os.path.exists(tmpFile):
            os.unlink(tmpFile)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        printUsage()
    sys.exit(main(*sys.argv[1:]))
