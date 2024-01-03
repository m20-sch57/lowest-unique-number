class Results:
    def __init__(self, players, scores, max_scores, n_rounds):
        res = [(self.win_rate(scores[i], max_scores[i]), i) for i in range(len(players))]
        res.sort(reverse=True)
        self.players = [None] * len(players)
        self.scores = [None] * len(scores)
        self.max_scores = [None] * len(max_scores)
        for j, (_, i) in enumerate(res):
            self.players[j] = players[i]
            self.scores[j] = scores[i]
            self.max_scores[j] = max_scores[i]

        self.n_rounds = n_rounds
        self.error = None
    
    @staticmethod
    def win_rate(score, max_score):
        return score / max_score if max_score > 0 else 0
    
    @staticmethod
    def tabify(s, l):
        return s + ' ' * max(0, l - len(s))

    def __str__(self):
        res = [(self.scores[i] / self.max_scores[i] if self.max_scores[i] > 0 else 0, i) 
               for i in range(len(self.players))]
        res.sort(reverse=True)
        width = max([len(player) for player in self.players])
        s = ''
        for frac, i in res:
            name = self.tabify(self.players[i], width)
            s += f'{name}\t{frac:.3f}\t({self.scores[i]} / {self.max_scores[i]})\n'
        return s
    

def socketio_obj(round_id, results):
    return {
        'round_id': round_id,
        'results': results.__dict__
    }
