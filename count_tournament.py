#!/usr/bin/python3

import os
import sys

import numpy as np
import matplotlib.pyplot as plt

def main() -> int:
    if (len(sys.argv) != 3):
        print("usage: {} <strats_dir> <results_dir>".format(sys.argv[0]), file=sys.stderr)
        return 1
    if (not os.path.isdir(sys.argv[1])):
        print("{}: `{}` not a directory".format(sys.argv[0], sys.argv[1]), file=sys.stderr)
        return 1
    if (not os.path.isdir(sys.argv[2])):
        print("{}: `{}` not a directory".format(sys.argv[0], sys.argv[2]), file=sys.stderr)
        return 1
    strats_dir = sys.argv[1]
    base_dir = sys.argv[2]
    st_names = {s: i for i, s in enumerate(sorted(list(map(lambda x: os.path.basename(x).replace(".py", ""), os.listdir(strats_dir)))))}
    N = len(st_names)
    scores = np.zeros(N)
    cnts = np.zeros(N)
    hist = []
    succ = []
    for name in sorted(os.listdir(base_dir)):
        if (not os.path.isdir(os.path.join(base_dir, name))):
            # it's results, skipping
            continue
        fd = open(os.path.join(base_dir, name, "config.txt"))
        for i in range(3):
            _ = fd.readline()
        names = list(map(lambda x: os.path.basename(x).replace(".py", ""), fd.readline().split()))
        fd.close()
        order = [st_names[s] for s in names]
        fd = open(os.path.join(base_dir, name, "log.txt"))
        cnt = 0
        _scores = np.zeros(len(order))
        for line in fd:
            a = list(np.array(list(map(int, line.split()))))
            ans = -1
            for i in sorted(a):
                if (a.count(i) == 1):
                    ans = a.index(i)
                    break
            if (ans != -1):
                _scores[ans] += 1
                cnt += 1
        fd.close()
        for i, pos in enumerate(order):
            scores[pos] += _scores[i]
            cnts[pos] += cnt
        hist.append(scores.copy())
        succ.append(cnts.copy())
    succ = np.array(succ).T
    hist = np.array(hist).T
    hist = hist / np.where(succ > 0, succ, 1)
    legend = sorted(st_names.keys())
    plt.stackplot(np.arange(hist.shape[1])+1, *hist, labels=legend)
    # cnt = 0
    # for i in cnts[:-1]:
    #     cnt += i
    #     plt.axvline(x=cnt, color="black", lw=0.5)
    plt.legend()
    plt.figure("normallized")
    hist = hist / hist.sum(axis=0) # normallization
    plt.stackplot(np.arange(hist.shape[1])+1, *hist, labels=legend)
    plt.ylim([0,1])
    plt.legend()
    plt.show()
    return 0

if (__name__ == "__main__"):
    exit(main())
