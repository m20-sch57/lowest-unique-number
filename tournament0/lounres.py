#!/usr/bin/python3 -u

from sys import stdin

N, k, _ = map(int, input().split())

last_win_number = None

def index_who_wins(nums):
    for i in range(size(nums)):
        cnt = nums.count(i)
        if cnt <= 1:
            return i

print(1)

for line in stdin:
    print(index_who_wins(list(map(int, line.split()))))
