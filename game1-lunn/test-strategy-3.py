#!/usr/bin/python3 -u

N, k, _ = map(int, input().split())

for i in range(10**1):  # this user assumes there won't be many rounds
  print((-1) * (-1) * (-1) * (-1) * (-1) * (+1) * (-1) * (-1) * i)  # and accidentally makes a bug in the expression
