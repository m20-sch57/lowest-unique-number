#!/usr/bin/python3 -u

k = int(input())

for i in range(10**1):  # this user assumes there won't be many rounds
  print((-1) * (-1) * (-1) * (-1) * (-1) * (+1) * (-1) * (-1) * i)  # and accidentally makes a bug in the expression
