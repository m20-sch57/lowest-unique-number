#!/usr/bin/python3

# import numpy as np
import random
import sys
import os
import subprocess


LOGPATH = os.environ.get('LOGPATH', 'logs')


def get_round_log_path(round_id, max_round_id):
    return LOGPATH + '/round' + str(round_id).zfill(len(str(max_round_id)))


def play(round_id, max_round_id, strats):
    log_path = get_round_log_path(round_id, max_round_id)
    os.makedirs(log_path)
    # status = os.system(f'./master {log_path}/config.txt {log_path}/log.txt {log_path}/user.txt {len(strats)} {" ".join(strats)} 2>/dev/null')
    proc = subprocess.Popen(["./master", f"{log_path}/config.txt", f"{log_path}/log.txt", f"{log_path}/user.txt", f"{log_path}/error.txt", str(len(strats)),] + strats)
    status = proc.wait()
    print(status)
    return status == 0


def get_winner(turns):
    cnt = dict()
    for turn in turns:
        if turn not in cnt:
            cnt[turn] = 1
        else:
            cnt[turn] += 1
    unique = [turn for turn in cnt.keys() if cnt[turn] == 1]
    if len(unique) == 0:
        return -1
    return turns.index(min(unique))


def tabify(s, l):
    return s + ' ' * max(0, l - len(s))


def print_results(players, scores, max_scores, fout=sys.stdout):
    results = [(scores[i] / max_scores[i] if max_scores[i] > 0 else 0, i) for i in range(N)]
    results.sort(reverse=True)
    width = max([len(player) for player in players])
    for result, i in results:
        print(f'{tabify(players[i], width)}\t{result:.3f}\t({scores[i]} / {max_scores[i]})', file=fout)


N_ROUNDS = 100

if len(sys.argv) < 3:
    print('usage: {} <M> <player1> ... <playerN>'.format(sys.argv[0]), file=sys.stderr)
    exit(1)

M = int(sys.argv[1])
players = sys.argv[2:]
N = len(players)
scores = [0] * N
max_scores = [0] * N

for round_id in range(1, N_ROUNDS + 1):
    print('Round {}...'.format(round_id))
    picked = random.sample(range(N), M)
    strats = [players[i] for i in picked]
    success = play(round_id, N_ROUNDS, strats)
    log_path = get_round_log_path(round_id, N_ROUNDS)
    if not success:
        print('Error, see ' + log_path + '/user.txt')
        exit(1)
    fin = open(log_path + '/log.txt')
    lines = fin.readlines()
    if not lines:
        print('Error, see ' + log_path + '/user.txt')
        exit(1)
    for line in lines:
        turns = list(map(int, line.split()))
        winner = get_winner(turns)
        if winner != -1:
            scores[picked[winner]] += 1
        for i in picked:
            max_scores[i] += 1
    fin.close()
    print(f'leaderboard after round {round_id}:')
    print_results(players, scores, max_scores)

results_path = LOGPATH + '/results.txt'
fout = open(results_path, 'w')
print_results(players, scores, max_scores, fout)
print('Saved results to ' + results_path)
