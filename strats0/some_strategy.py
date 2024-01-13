#!/usr/bin/python3 -u

from random import choices
from statistics import median

list_of_winning_numbers = []

def winning_number(lst):
    s = sorted(lst)
    i = 0
    if len(s) == 1:
        return s[0]
    while i < len(s) - 1:
        if s[i] < s[i + 1]:
            return s[i]
        else:
            while i < len(s) - 1 and s[i] == s[i + 1]:
                i += 1
            i += 1
    if i == len(s) - 1:
        return s[i]
    return -1

N, k, TURNS = map(int, input().split())
people = [0 for _ in range(N - 1)]

print(1)

for number in range(TURNS - 1):
    others_ans = list(map(int, input().split()))
    w = winning_number(others_ans)
    if w != -1:
        list_of_winning_numbers.append(w)
    ans = round(median(list_of_winning_numbers))
    if ans == 0:
        print(*choices([0, 1], [3/4, 1/4]))
    else:
        print(*choices([ans - 1, ans, ans + 1], [1/4, 2/3, 1/12]))
    
print(list_of_winning_numbers)
