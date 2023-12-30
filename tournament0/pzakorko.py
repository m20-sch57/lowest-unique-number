#!/usr/bin/python3 -u

from random import choices
from statistics import median

N, k, TURNS = map(int, input().split())
people = [0 for _ in range(N - 1)]

print(0)

for number in range(TURNS - 1):
    others_ans = list(map(int, input().split()))
    if N == 3 or N == 4:
        weight = [2/5, 1/5, 2/5]
        ans = [0, 1, 2]
        print(*choices(ans, weight))
    elif N >= 5:
        weight = [1/4, 1/4, 5/16, 1/8, 1/16]
        ans = [0, 1, 2, 3, 4]
        print(*choices(ans, weight))
