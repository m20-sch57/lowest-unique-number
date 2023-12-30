#!/usr/bin/python3

import sys

import numpy as np
import matplotlib.pyplot as plt

def main() -> int:
    if (len(sys.argv) != 1):
        print("usage: {}".format(sys.argv[0]), file=sys.stderr)
        return 1
    N = -1
    scores = None
    hist = []
    succ = []
    for line in sys.stdin:
        a = list(map(int, line.split()))
        if (N == -1):
            N = len(a)
            scores = np.zeros(N)
        ans = -1
        for i in sorted(a):
            if (a.count(i) == 1):
                ans = a.index(i)
                break
        if (ans != -1):
            scores[ans] += 1
        succ.append(sum(scores))
        hist.append(scores.copy())
    succ = np.array(succ)
    hist = np.array(hist).T / succ
    plt.stackplot(np.arange(hist.shape[1])+1, *hist, labels=["strategy " + str(i) for i in range(N)])
    plt.xscale("log")
    plt.legend()
    plt.show()
    return 0

if (__name__ == "__main__"):
    exit(main())
