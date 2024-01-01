#!/usr/bin/python3 -u

import numpy as np


n, k, cnt_turns = map(int, input().split())
mx = 4
prob = np.array([0.7 ** (i + 1) for i in range(mx)])
prob /= sum(prob)
for turn in range(cnt_turns):
    print(np.random.choice(mx, p=prob))
    if turn != cnt_turns - 1:
        turns = list(map(int, input().split()))
