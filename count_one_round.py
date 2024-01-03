#!/usr/bin/python3

import os
import sys

import numpy as np
import matplotlib.pyplot as plt

def main() -> int:
    if (len(sys.argv) != 2):
        print("usage: {} <round_log>".format(sys.argv[0]), file=sys.stderr)
        return 1
    fd = open(sys.argv[1])
    nums = dict()
    for i, line in enumerate(fd):
        a = list(map(int, line.split()))
        for j in set(a):
            if (j in nums):
                nums[j].append(a.count(j))
            else:
                nums[j] = [0 for _ in range(i)]
                nums[j].append(a.count(j))
        for j in nums.keys():
            if (len(nums[j]) != i+1):
                nums[j].append(0)
    legend = sorted(nums.keys())
    nums = [nums[_l] for _l in legend]
    plt.stackplot(np.arange(len(nums[0]))+1, *nums, labels=legend)
    plt.legend()
    plt.show()
    return 0

if (__name__ == "__main__"):
    exit(main())
