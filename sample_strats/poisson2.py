#!/usr/bin/python3 -u

import numpy as np
from scipy.stats import poisson


n, k, cnt_turns = map(int, input().split())
mx = 4
prob = poisson.pmf(np.arange(mx), 0.9)
prob /= sum(prob)
for turn in range(cnt_turns):
    print(np.random.choice(mx, p=prob))
    if turn != cnt_turns - 1:
        turns = list(map(int, input().split()))
