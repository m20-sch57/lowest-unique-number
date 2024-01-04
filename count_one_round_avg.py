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
    BUCK = 10
    _nums = [np.array(nums[_l]).reshape(BUCK, len(nums[_l]) // BUCK) for _l in legend]
    nums = [i.mean(axis=0) for i in _nums]
    vars = [np.sqrt(i.var(axis=0)) for i in _nums]
    x_ls = np.arange(len(nums[0]))+1
    plt.stackplot(x_ls, *nums, labels=legend)
    tmp = np.zeros_like(nums[0])
    for n, v in zip(nums, vars):
        tmp += n
        plt.fill_between(x_ls, tmp - v, tmp + v, alpha=0.25, color="black")
        # plt.plot(x_ls, tmp, color="black")
    plt.legend()
    plt.show()
    return 0

if (__name__ == "__main__"):
    exit(main())
