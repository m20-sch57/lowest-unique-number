#!/usr/bin/python3 -u

import torch
from torch import nn
from torch.optim import Adam


def encode_move(turn):
    """
    Embedding of the turn number.
    """
    v = torch.zeros(cnt_classes)
    v[min(turn, cnt_classes - 1)] = 1
    return v


def encode_moves(turns):
    """
    Returns concatenated embeddings of the turn numbers.
    """
    return torch.concat([encode_move(turn) for turn in turns])


def make_turn():
    """
    Makes one turn of the strategy.
    """
    # bluff for the first 0.1 turns
    if turn < cnt_turns * 0.1:
        return 0
    # get estimated probabilities for each strategy
    probs = [torch.softmax(model(x), dim=0).detach().numpy() for model in models]
    probs.pop(me)
    # find the best number using dynamic programming
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
    return dist.index(max(dist))


def on_results(turns):
    """
    Updates the model parameters based on results.
    """
    global x
    for i in range(cnt_players):
        target = min(cnt_classes - 1, turns[i])
        loss = loss_fn(models[i](x).unsqueeze(0), torch.tensor([target]))
        optimizers[i].zero_grad()
        loss.backward()
        optimizers[i].step()
    extra = encode_moves(turns)
    x = torch.concat([x, extra])[len(extra):]


# some constants
context_len = 3
cnt_classes = 4
lr = 1e-2

cnt_players, me, cnt_turns = map(int, input().split())

# store linear models that simulate other strategies
loss_fn = nn.CrossEntropyLoss()
models = []
optimizers = []
x = torch.zeros(context_len * cnt_players * cnt_classes)
for i in range(cnt_players):
    model = nn.Linear(len(x), cnt_classes)
    optimizer = Adam(model.parameters(), lr=lr)
    models.append(model)
    optimizers.append(optimizer)

for turn in range(cnt_turns):
    print(make_turn())
    if turn != cnt_turns - 1:
        turns = list(map(int, input().split()))
        on_results(turns)
