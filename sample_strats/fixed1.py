#!/usr/bin/python3 -u

n, k, cnt_turns = map(int, input().split())
for turn in range(cnt_turns):
    print(1)
    if turn != cnt_turns - 1:
        turns = list(map(int, input().split()))
