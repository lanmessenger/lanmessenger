#! /usr/bin/python
"""
This script adds a license file to a DMG. Requires Xcode to be installed in the
default location (/Developer). Obviously only runs on a Mac.
"""
import os
import sys
import tempfile

class Path(str):
    def __enter__(self):
        return self
    def __exit__(self, type, value, traceback):
        os.unlink(self)

def mktemp(dir=None, suffix=''):
    (fd, filename) = tempfile.mkstemp(dir=dir, suffix=suffix)
    os.close(fd)
    return Path(filename)

def printUsage():
    print "This program adds a software license agreement to a DMG file."
    print "It requires Xcode to be installed in the default location (/Developer)\n"
    print "Usage: %s <dmgFile> <licenseFile>" % sys.argv[0]
    print "The <licenseFile> must be a plain ascii text file."
    sys.exit(1)

def main(dmgFile, license):
    with mktemp('.') as tmpFile:
        with open(tmpFile, 'w') as f:
            f.write("""data 'LPic' (5000) {
    $"0002 0011 0003 0001 0000 0000 0002 0000"
    $"0008 0003 0000 0001 0004 0000 0004 0005"
    $"0000 000E 0006 0001 0005 0007 0000 0007"
    $"0008 0000 0047 0009 0000 0034 000A 0001"
    $"0035 000B 0001 0020 000C 0000 0011 000D"
    $"0000 005B 0004 0000 0033 000F 0001 000C"
    $"0010 0000 000B 000E 0000"
};\n\n""")
            with open(license, 'r') as l:
                f.write('data \'TEXT\' (5002, "English") {\n')
                for line in l:
                    f.write('    "' + line.strip().replace('"', '\\"') + '\\n"\n')
                f.write('};\n\n')
            f.write("""resource 'STR#' (5002, "English") {
    {
        "English",
        "Agree",
        "Disagree",
        "Print",
        "Save...",
        "IMPORTANT - By clicking on the \\"Agree\\" button, you agree "
        "to be bound by the terms of the License Agreement.",
        "Software License Agreement",
        "This text cannot be saved. This disk may be full or locked, or the "
        "file may be locked.",
        "Unable to print. Make sure you have selected a printer."
    }
};""")
        os.system('/usr/bin/hdiutil unflatten -quiet "%s"' % dmgFile)
        os.system('/Developer/Tools/Rez /Developer/Headers/FlatCarbon/*.r %s -a -o "%s"' % (tmpFile, dmgFile))
        os.system('/usr/bin/hdiutil flatten -quiet "%s"' % dmgFile)
    print "Successfully added license to '%s'" % dmgFile

if __name__ == '__main__':
    if len(sys.argv) != 3:
        printUsage()
    if not os.path.exists('/Developer/Tools/Rez'):
        printUsage()
    main(*sys.argv[1:])

