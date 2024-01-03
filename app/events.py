from flask import *
from flask_socketio import *
from app import socketio, tournaments, lock
from .results import socketio_obj


@socketio.on('join')
def join(tournament_id):
    if tournament_id not in tournaments.keys():
        return
    with lock:
        join_room(tournament_id)
        emit('results', socketio_obj(*tournaments[tournament_id]), room=request.sid)
