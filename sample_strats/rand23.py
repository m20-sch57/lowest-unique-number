#!/usr/bin/python3 -u

import random


n, k, cnt_turns = map(int, input().split())
for turn in range(cnt_turns):
    print(random.randint(2, 3))
    if turn != cnt_turns - 1:
        turns = list(map(int, input().split()))
