#!/usr/bin/python3 -u

from sys import stdin, stderr
from random import choice

win_nums = []


def index_who_wins(nums):
    #for i in range(0, max(nums) + 1):
    for i in nums:
        cnt = nums.count(i)
        if cnt == 1:
            return nums.index(i)
    return None


def strategy():
    if win_nums:
        return choice(win_nums)
    return choice([0, 1])


N = int(input())

print(strategy())

for line in stdin:
    move = list(map(int, line.split()))
    ind = index_who_wins(move)
    if ind is not None:
        win_nums.append(move[ind])
    print(strategy())
    print("\t", win_nums, file=stderr)
