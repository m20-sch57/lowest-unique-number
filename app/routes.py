import os
import shutil
import uuid
from flask import *
from app import app, socketio, tournaments, lock
from .game import run_tournament
from .results import Results


@app.route('/')
def new_tournament():
    return render_template('new_tournament.html')


@app.route('/tournament/<tournament_id>')
def view_tournament(tournament_id):
    if tournament_id not in tournaments.keys():
        abort(404)
    return render_template('tournament.html')


@app.route('/tournament/<tournament_id>/download')
def download_tournament(tournament_id):
    if tournament_id not in tournaments.keys():
        abort(404)
    tournament_dir = os.path.join('tournaments', tournament_id)
    output_file = os.path.join('tournaments', tournament_id)
    shutil.make_archive(output_file, 'zip', tournament_dir)
    return send_file(os.path.join(os.getcwd(), output_file + '.zip'), as_attachment=True)


@app.route('/submit', methods=['POST'])
def submit():
    n_players = int(request.form['n-players'])
    n_rounds = int(request.form['n-rounds'])
    sample_strats = request.form.getlist('sample-strats')
    strat_files = request.files.getlist('strat-files')
    if n_players > len(sample_strats) + len(strat_files) or n_players < 3:
        abort(400)
    tournament_id = str(uuid.uuid4())
    with lock:
        tournaments[tournament_id] = (0, Results([], [], [], n_rounds))
    user_strat_names = [strat_file.filename for strat_file in strat_files]
    user_strat_contents = [strat_file.read() for strat_file in strat_files]
    thread = socketio.start_background_task(run_tournament, tournament_id, n_players, n_rounds, sample_strats,
                                            user_strat_names, user_strat_contents)
    return tournament_id
