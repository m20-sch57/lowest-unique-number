'use strict'

const socket = io.connect(window.location.origin);
const tournamentId = location.href.substring(location.href.lastIndexOf('/') + 1);

function winRate(score, maxScore) {
    if (maxScore == 0) {
        return 0.0;
    }
    return Math.round(1000.0 * score / maxScore) / 1000;
}

function showLeaderboard(players, scores, maxScores) {
    let n = players.length;
    let leaderboard = document.getElementById("leaderboard");
    while (leaderboard.children.length < 4 * (n + 1)) {
        let child = document.createElement("div");
        child.classList.add("leaderboard-item");
        leaderboard.appendChild(child);
    }
    for (let i = 0; i < n; i++) {
        leaderboard.children[4 + 4 * i].innerHTML = players[i];
        leaderboard.children[5 + 4 * i].innerHTML = winRate(scores[i], maxScores[i]);
        leaderboard.children[6 + 4 * i].innerHTML = scores[i];
        leaderboard.children[7 + 4 * i].innerHTML = maxScores[i];
    }
}

function finalize(error) {
    let download = document.getElementById("download");
    download.setAttribute("href", `${tournamentId}/download`);
    download.style.display = "";
    if (error !== null) {
        document.getElementById("show-error").style.display = "";
        document.getElementById("leaderboard-title").style.display = "none";
        document.getElementById("error-title").style.display = "";
        document.getElementById("error-content").innerText = error;
    } else {
        document.getElementById("leaderboard-title").innerHTML = "Final leaderboard";
    }
}

function showError() {
    document.getElementById("popup").style.display = "";
}

function hideError() {
    document.getElementById("popup").style.display = "none";
}

socket.on("connect", () => socket.emit("join", tournamentId))
socket.on("results", (message) => {
    showLeaderboard(message.results.players, message.results.scores, message.results.max_scores);
    let numRounds = message.results.n_rounds;
    let roundId = message.round_id;
    let percentage = Math.round(100.0 * roundId / numRounds);
    document.getElementById("progress").innerHTML = `${percentage}%`;
    let error = message.results.error;
    if (error != null || roundId == numRounds) {
        finalize(error);
    }
});

window.onload = () => {
    document.getElementById("show-error").onclick = showError;
    document.getElementById("hide-error").onclick = hideError;
}
