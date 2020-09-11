#!/usr/bin/env python

##########################################################################
# Usage : add_version_info.py <par1> [par2] ... <parN-1> <parN>
#
# par1 .. parN-2 : strings to be appended to the binary at the end
# parN-1         : Total length of the buffer to be appended to the binary
# parN           : The binary to append the data to
#
# The binary mentioned in parN will be checksummed, and thats what will be
# appended to the versioned binary, over and above the provided strings on
# the command line.
# NOTE: This will change the binary inplace
##########################################################################

import sys, struct, hashlib

def compute_md5sum(filename):
    m = hashlib.md5()
    with open(filename, 'rb') as f:
        for line in f:
            m.update(line)
    return m.hexdigest()[:8]

if __name__ == "__main__":
    # Collect cmdline parameters in variables
    if (len(sys.argv) < 3): # Make sure there is at least one string to append
        sys.stdout.write('Usage: %s <string1> [<string2> ...] <len> <file>\n' %
                                                                    sys.argv[0])

    text = sys.argv[1:-2]
    length = int(sys.argv[-2])
    binfile = sys.argv[-1]

    # Checksum the original binary
    text.append(compute_md5sum(binfile))

    # Now append stuff to the original binary
    with open(binfile, 'ab') as f:
        version_string = '-'.join([str(item) for item in text])
        sys.stdout.write('Appending version string: %s to %s\n' %
                                        (version_string, binfile))
        f.write(struct.pack('%ds' % (length), version_string))
