#!/usr/bin/python3 -u
from sys import stdin
from random import choice


def index_who_wins(nums):
    for i in sorted(nums):
        if nums.count(i) == 1:
            return nums.index(i)
    return None


def nums_to_win(nums):
    arr = []
    for i in range(len(nums) + 1):
        if index_who_wins(nums + [i]) == len(nums):
            arr.append(i)
    return arr


moves_7 = []
best_choices_7 = []


def strategy(last_move):
    if last_move is not None:
        last_move_wo_7 = last_move[:K] + last_move[K + 1:]
        best_choice = nums_to_win(last_move_wo_7)
        if best_choice:
            best_choices_7.append(best_choice[0])
        if len(best_choices_7) >= 10:
            num = choice(best_choices_7)
            moves_7.append(num)
            return num
    moves_7.append(0)
    return 0


N_players, K, TURNS = map(int, input().split())

print(strategy(None))

for line in stdin:
    move = list(map(int, line.split()))
    print(strategy(move))
