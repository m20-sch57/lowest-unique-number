import os
import random
import shutil
import subprocess
from app import socketio, tournaments, lock
from .results import Results, socketio_obj


def get_strats_dir(tournament_id):
    return os.path.join('tournaments', tournament_id, 'strats')


def get_results_dir(tournament_id):
    return os.path.join('tournaments', tournament_id, 'results')


def get_round_dir(tournament_id, round_id, n_rounds):
    return os.path.join(get_results_dir(tournament_id), 'round' + str(round_id).zfill(len(str(n_rounds - 1))))


def prepare_strats(strats_dir, sample_strats, user_strat_names, user_strat_contents):
    for strat_name in sample_strats:
        shutil.copyfile(os.path.join('sample_strats', strat_name),
                        os.path.join(strats_dir, strat_name))
        os.chmod(os.path.join(strats_dir, strat_name), 0o775)
    for strat_name, strat_content in zip(user_strat_names, user_strat_contents):
        fout = open(os.path.join(strats_dir, strat_name), 'wb')
        fout.write(strat_content)
        fout.close()
        os.chmod(os.path.join(strats_dir, strat_name), 0o775)
    return sample_strats + user_strat_names


def play(round_dir, strats_dir, strats):
    os.makedirs(round_dir)
    strats_full = [os.path.join(os.getcwd(), strats_dir, strat) for strat in strats]
    master_path = os.path.join(os.getcwd(), 'master')
    config_path = os.path.join(os.getcwd(), round_dir, 'config.txt')
    log_path = os.path.join(os.getcwd(), round_dir, 'log.txt')
    user_path = os.path.join(os.getcwd(), round_dir, 'user.txt')
    error_path = os.path.join(os.getcwd(), round_dir, 'error.txt')
    proc = subprocess.Popen([master_path, config_path, log_path, user_path, error_path, str(len(strats))] +
                            strats_full, cwd=round_dir)
    status = proc.wait()
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


def get_error(round_dir):
    try:
        return open(os.path.join(round_dir, 'error.txt')).read()
    except FileNotFoundError:
        return ''


def run_tournament(tournament_id, n_players, n_rounds, sample_strats, user_strat_names, user_strat_contents):
    strats_dir = get_strats_dir(tournament_id)
    results_dir = get_results_dir(tournament_id)
    os.makedirs(strats_dir)
    os.makedirs(results_dir)
    players = prepare_strats(strats_dir, sample_strats, user_strat_names, user_strat_contents)
    scores = [0] * len(players)
    max_scores = [0] * len(players)
    for round_id in range(n_rounds):
        picked = random.sample(range(len(players)), n_players)
        strats = [players[i] for i in picked]
        round_dir = get_round_dir(tournament_id, round_id, n_rounds)
        success = play(round_dir, strats_dir, strats)
        if not success:
            with lock:
                tournaments[tournament_id][1].error = get_error(round_dir)
                socketio.emit('results', socketio_obj(*tournaments[tournament_id]), room=tournament_id)
            break
        fin = open(os.path.join(round_dir, 'log.txt'))
        lines = fin.readlines()
        if not lines:
            with lock:
                tournaments[tournament_id][1].error = get_error(round_dir)
                socketio.emit('results', socketio_obj(*tournaments[tournament_id]), room=tournament_id)
            f_err.close()
            break
        for line in lines:
            turns = list(map(int, line.split()))
            winner = get_winner(turns)
            if winner != -1:
                scores[picked[winner]] += 1
            for i in picked:
                max_scores[i] += 1
        fin.close()
        with lock:
            tournaments[tournament_id] = (round_id + 1, Results(players, scores, max_scores, n_rounds))
            socketio.emit('results', socketio_obj(*tournaments[tournament_id]), room=tournament_id)
    else:
        fout = open(os.path.join(results_dir, 'results.txt'), 'w')
        fout.write(str(tournaments[tournament_id][1]))
        fout.close()
