#!/usr/bin/python3 -u

import torch
from torch import nn
from torch.optim import Adam
from sys import stdin, stderr


def output(msg):
    print('fedro: ' + str(msg), file=stderr)


def encode_move(turn):
    v = torch.zeros(cnt_classes)
    v[min(turn, cnt_classes - 1)] = 1
    return v


def encode_moves(turns):
    return torch.concat([encode_move(turn) for turn in turns])


def winning_move(turns):
    cnt = [0] * cnt_classes
    for turn in turns:
        cnt[min(cnt_classes - 1, turn)] += 1
    if 0 not in cnt:
        return -1
    pos = cnt.index(0)
    if pos == 0 or min(cnt[:pos]) >= 2:
        return pos
    return -1


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


def make_turn():
    if turn < cnt_turns * sleep_ratio:
        return 0
    probs = [torch.softmax(model(x), dim=0).detach().numpy() for model in models]
    probs.pop(me)
    dp = dict()
    dp[(0,) * cnt_classes] = 1.0
    for i in range(cnt_players - 1):
        dp2 = dict()
        for mask in dp.keys():
            for v in range(cnt_classes):
                mask2 = mask[:v] + (min(2, mask[v] + 1),) + mask[v + 1:]
                if mask2 not in dp2:
                    dp2[mask2] = 0.0
                dp2[mask2] += dp[mask] * probs[i][v]
        dp = dp2
    dist = [0.0] * cnt_classes
    for j in range(cnt_classes):
        for mask in dp.keys():
            if (j == 0 or min(mask[:j]) >= 2) and mask[j] == 0:
                dist[j] += dp[mask]
    # output(max(dist))
    return dist.index(max(dist))


def results(turns):
    global x, scores
    w = get_winner(turns)
    if w != -1:
        scores[w] += 1
    for i in range(cnt_players):
        target = min(cnt_classes - 1, turns[i])
        loss = loss_fn(models[i](x).unsqueeze(0), torch.tensor([target]))
        optimizers[i].zero_grad()
        loss.backward()
        optimizers[i].step()
    extra = encode_moves(turns)
    x = torch.concat([x, extra])[len(extra):]


context_len = 3
cnt_classes = 4
sleep_ratio = 0.1
lr = 1e-2
cnt_players, me, cnt_turns = map(int, input().split())
loss_fn = nn.CrossEntropyLoss()
scores = [0] * cnt_players
models = []
optimizers = []
x = torch.zeros(context_len * cnt_players * cnt_classes)
for i in range(cnt_players):
    model = nn.Linear(len(x), cnt_classes)
    optimizer = Adam(model.parameters(), lr=lr)
    models.append(model)
    optimizers.append(optimizer)

turn = 0
print(make_turn())
for turn in range(1, cnt_turns):
    turns = list(map(int, input().split()))
    results(turns)
    print(make_turn())
