import sqlite3
import argparse
from itertools import combinations

DB_PATH = 'pootjeover.db'
SUBS_ID = 3

VIRTUAL_PLAYERS = [
    (1, 'XX34'),
    (2, 'XX45'),
    (3, 'Subs')
]

def init_db(db_path=DB_PATH):
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()
    cur.execute(
        "CREATE TABLE IF NOT EXISTS Players("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "\
        "name TEXT UNIQUE NOT NULL, "\
        "rating INTEGER DEFAULT 0, "\
        "cur_rating INTEGER DEFAULT 0, "\
        "active BOOLEAN DEFAULT 1"
        ")")
    cur.execute(
        "CREATE TABLE IF NOT EXISTS Teams("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "\
        "name TEXT NOT NULL"
        ")")
    cur.execute(
        "CREATE TABLE IF NOT EXISTS TeamPlayers("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "\
        "team INTEGER NOT NULL, "\
        "player INTEGER NOT NULL, "\
        "FOREIGN KEY(team) REFERENCES Teams(id), "\
        "FOREIGN KEY(player) REFERENCES Players(id)"
        ")")
    cur.execute(
        "CREATE TABLE IF NOT EXISTS Games("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "\
        "team1 INTEGER NOT NULL, "\
        "team2 INTEGER NOT NULL, "\
        "score1 INTEGER DEFAULT 0, "\
        "score2 INTEGER DEFAULT 0, "\
        "FOREIGN KEY(team1) REFERENCES Teams(id), "\
        "FOREIGN KEY(team2) REFERENCES Teams(id)"
        ")")
    for pid, name in VIRTUAL_PLAYERS:
        cur.execute("INSERT OR IGNORE INTO Players(id, name) VALUES (?, ?)", (pid, name))
    conn.commit()
    return conn

def add_player(conn, name, rating):
    conn.execute("INSERT INTO Players(name, rating, cur_rating) VALUES (?, ?, ?)", (name, rating, rating))
    conn.commit()

def create_team(conn, players):
    cur = conn.cursor()
    cur.execute("INSERT INTO Teams(name) VALUES (?)", ("team",))
    team_id = cur.lastrowid
    for p in players:
        cur.execute("INSERT INTO TeamPlayers(team, player) VALUES (?, ?)", (team_id, p))
    conn.commit()
    return team_id

def record_game(conn, team1, team2, score1, score2):
    t1 = create_team(conn, team1)
    t2 = create_team(conn, team2)
    conn.execute(
        "INSERT INTO Games(team1, team2, score1, score2) VALUES (?, ?, ?, ?)",
        (t1, t2, score1, score2)
    )
    conn.commit()

def determine_ratings(conn):
    cur = conn.cursor()
    cur.execute("SELECT id, rating FROM Players ORDER BY id ASC")
    new_rating = {row[0]: float(row[1]) for row in cur.fetchall()}
    if not new_rating:
        return []
    ratings = [new_rating.copy()]
    cur.execute("SELECT team1, team2, score1, score2 FROM Games ORDER BY id")
    for g in cur.fetchall():
        team1_id, team2_id, score1, score2 = g
        team1 = [r[0] for r in conn.execute("SELECT player FROM TeamPlayers WHERE team=?", (team1_id,))]
        team2 = [r[0] for r in conn.execute("SELECT player FROM TeamPlayers WHERE team=?", (team2_id,))]
        subs_present = 0
        rank1 = 0
        for p in team1:
            if p == SUBS_ID:
                subs_present = 1
            else:
                rank1 += new_rating[p]
        rank2 = 0
        for p in team2:
            if p == SUBS_ID:
                subs_present = 2
            else:
                rank2 += new_rating[p]
        if subs_present == 1:
            rank2 -= rank2 / len(team2)
        elif subs_present == 2:
            rank1 -= rank1 / len(team1)
        rank_diff = (rank1 - rank2) / (len(team1) + len(team2))
        score_diff = score1 - score2
        for p in team1:
            new_rating[p] -= (rank_diff - score_diff)
        for p in team2:
            new_rating[p] += (rank_diff - score_diff)
        new_rating[SUBS_ID] = 0
        ratings.append(new_rating.copy())
    return ratings

def update_rating_in_db(conn):
    ratings = determine_ratings(conn)
    if not ratings:
        return
    last = ratings[-1]
    cur = conn.cursor()
    for pid, val in last.items():
        cur.execute("UPDATE Players SET cur_rating=? WHERE id=?", (int(val), pid))
    conn.commit()

def show_ratings(conn):
    update_rating_in_db(conn)
    cur = conn.cursor()
    cur.execute("SELECT id, name, cur_rating FROM Players ORDER BY id")
    for row in cur.fetchall():
        print(f"{row[0]:>2} {row[1]:<20} {row[2]}")

def suggest_teams(conn, players):
    if len(players) % 2:
        players.append(SUBS_ID)
    team_size = len(players) // 2
    cur_ratings = {pid: conn.execute("SELECT cur_rating FROM Players WHERE id=?", (pid,)).fetchone()[0] for pid in players}
    best = None
    best_diff = None
    for comb in combinations(players, team_size):
        team1 = set(comb)
        team2 = set(players) - team1
        rank1 = sum(cur_ratings[p] for p in team1)
        rank2 = sum(cur_ratings[p] for p in team2)
        diff = abs(rank1 - rank2)
        if best_diff is None or diff < best_diff:
            best_diff = diff
            best = (list(team1), list(team2))
    return best, best_diff

def main():
    parser = argparse.ArgumentParser(description="Football CLI")
    sub = parser.add_subparsers(dest="cmd")

    p = sub.add_parser("init-db")
    p.add_argument("--db", default=DB_PATH)

    p = sub.add_parser("add-player")
    p.add_argument("name")
    p.add_argument("rating", type=int)
    p.add_argument("--db", default=DB_PATH)

    p = sub.add_parser("record-game")
    p.add_argument("score1", type=int)
    p.add_argument("score2", type=int)
    p.add_argument("--team1")
    p.add_argument("--team2")
    p.add_argument("--db", default=DB_PATH)

    p = sub.add_parser("show-ratings")
    p.add_argument("--db", default=DB_PATH)

    p = sub.add_parser("suggest")
    p.add_argument("players")
    p.add_argument("--db", default=DB_PATH)

    args = parser.parse_args()

    conn = init_db(args.db) if args.cmd else init_db(DB_PATH)

    if args.cmd == "add-player":
        add_player(conn, args.name, args.rating)
    elif args.cmd == "record-game":
        team1 = [int(x) for x in args.team1.split(',')]
        team2 = [int(x) for x in args.team2.split(',')]
        record_game(conn, team1, team2, args.score1, args.score2)
    elif args.cmd == "show-ratings":
        show_ratings(conn)
    elif args.cmd == "suggest":
        players = [int(x) for x in args.players.split(',')]
        teams, diff = suggest_teams(conn, players)
        print("Team1:", teams[0])
        print("Team2:", teams[1])
        print("Rating diff:", diff)
    elif args.cmd == "init-db":
        pass
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
