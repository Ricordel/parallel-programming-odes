#!/usr/bin/python
#-*- coding: utf-8 -*-

import matplotlib.pyplot as plot
import numpy


def get_values(filename):
    # There should be only one line in each file
    line = open(filename).readlines()[0].strip()
    vals = [float(v) for v in line.split(" ")]
    return vals

def print_vals(ys):
    xs = numpy.linspace(0, 1.0, len(ys))
    plot.plot(xs, ys)
    plot.show()



if __name__ == "__main__":
    import sys
    import itertools
    # Get all the files IN ORDER, concatenate their content,
    # and draw the result.
    if len(sys.argv) != 3:
        print("Usage: %s <file prefix> <number of files>" % sys.argv[0])
        sys.exit(1)
    file_prefix = sys.argv[1]
    nb_files = int(sys.argv[2])

    files = ["%s%i.dat" % (file_prefix, i) for i in range(nb_files)]
    vals = [get_values(f) for f in files]
    vals = list(itertools.chain(*vals))
    vals = [0] + vals + [0]

    print_vals(vals)
