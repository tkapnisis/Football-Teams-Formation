#!/usr/bin/env python3
"""Terminal-first football team manager.

This module is intentionally function-oriented so each action can be reused
from scripts/tests and mapped directly to CLI commands.
"""

from __future__ import annotations

import argparse
import sqlite3
from pathlib import Path
from typing import Iterable


DB_DEFAULT = "data.db"
VIRTUAL_PLAYERS = [
    (1, "XX34", 50, 50, 1),
    (2, "XX45", 50, 50, 1),
    (3, "Subs", 50, 50, 1),
]

def get_connection(db_path: str) -> sqlite3.Connection:
    conn = sqlite3.connect(db_path)
    conn.execute("PRAGMA foreign_keys = ON;")
    return conn


def initialize_database(conn: sqlite3.Connection) -> None:
    conn.executescript(
        """
        CREATE TABLE IF NOT EXISTS Players (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE COLLATE NOCASE,
            rating INTEGER DEFAULT 0,
            cur_rating INTEGER DEFAULT 0,
            active BOOLEAN DEFAULT 1
        );

        CREATE TABLE IF NOT EXISTS Teams (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS TeamPlayers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            team INTEGER NOT NULL,
            player INTEGER NOT NULL,
            FOREIGN KEY(team) REFERENCES Teams(id),
            FOREIGN KEY(player) REFERENCES Players(id)
        );

        CREATE TABLE IF NOT EXISTS Games (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            team1 INTEGER NOT NULL,
            team2 INTEGER NOT NULL,
            score1 INTEGER DEFAULT 0,
            score2 INTEGER DEFAULT 0,
            FOREIGN KEY(team1) REFERENCES Teams(id),
            FOREIGN KEY(team2) REFERENCES Teams(id)
        );
        """
    )

    conn.executemany(
        """
        INSERT OR IGNORE INTO Players(id, name, rating, cur_rating, active)
        VALUES (?, ?, ?, ?, ?)
        """,
        VIRTUAL_PLAYERS,
    )
    conn.commit()


def create_player(conn: sqlite3.Connection, name: str, rating: int, active: bool = True) -> int:
    cursor = conn.execute(
        """
        INSERT INTO Players(name, rating, cur_rating, active)
        VALUES (?, ?, ?, ?)
        """,
        (name.strip(), rating, rating, int(active)),
    )
    conn.commit()
    return int(cursor.lastrowid)


def update_player(
    conn: sqlite3.Connection,
    player_id: int,
    name: str | None = None,
    rating: int | None = None,
    active: bool | None = None,
) -> None:
    row = conn.execute(
        "SELECT id, name, rating, cur_rating, active FROM Players WHERE id = ?",
        (player_id,),
    ).fetchone()
    if row is None:
        raise ValueError(f"Unknown player id: {player_id}")

    new_name = name.strip() if name is not None else row[1]
    new_rating = int(rating) if rating is not None else int(row[2])
    # Keep current rating in sync when base rating is explicitly changed.
    new_cur_rating = int(rating) if rating is not None else int(row[3])
    new_active = int(active) if active is not None else int(row[4])

    conn.execute(
        """
        UPDATE Players
        SET name = ?, rating = ?, cur_rating = ?, active = ?
        WHERE id = ?
        """,
        (new_name, new_rating, new_cur_rating, new_active, player_id),
    )
    conn.commit()


def list_players(conn: sqlite3.Connection) -> list[sqlite3.Row]:
    conn.row_factory = sqlite3.Row
    return conn.execute(
        """
        SELECT id, name, rating, cur_rating, active
        FROM Players
        ORDER BY id
        """
    ).fetchall()


def list_ratings(conn: sqlite3.Connection) -> list[sqlite3.Row]:
    conn.row_factory = sqlite3.Row
    return conn.execute(
        """
        SELECT id, name, rating AS initial_rating, cur_rating AS current_rating, active
        FROM Players
        ORDER BY cur_rating DESC, id ASC
        """
    ).fetchall()


def _create_team(conn: sqlite3.Connection, name: str, player_ids: Iterable[int]) -> int:
    team_cursor = conn.execute("INSERT INTO Teams(name) VALUES (?)", (name,))
    team_id = int(team_cursor.lastrowid)
    conn.executemany(
        "INSERT INTO TeamPlayers(team, player) VALUES (?, ?)",
        [(team_id, pid) for pid in player_ids],
    )
    return team_id


def _find_player_ids(conn: sqlite3.Connection, names: list[str]) -> list[int]:
    player_ids: list[int] = []
    for raw_name in names:
        name = raw_name.strip()
        row = conn.execute(
            "SELECT id FROM Players WHERE name = ? COLLATE NOCASE",
            (name,),
        ).fetchone()
        if row is None:
            raise ValueError(f"Unknown player: {name}")
        player_ids.append(int(row[0]))
    return player_ids


def create_game_with_attendees(
    conn: sqlite3.Connection,
    team1_attendees: list[str],
    team2_attendees: list[str],
    score1: int = 0,
    score2: int = 0,
) -> int:
    if not team1_attendees or not team2_attendees:
        raise ValueError("Both team attendee lists must be non-empty.")

    team1_player_ids = _find_player_ids(conn, team1_attendees)
    team2_player_ids = _find_player_ids(conn, team2_attendees)

    next_game_number = conn.execute("SELECT COALESCE(MAX(id), 0) + 1 FROM Games").fetchone()[0]
    team1_name = f"Game{next_game_number}-Team1"
    team2_name = f"Game{next_game_number}-Team2"

    team1_id = _create_team(conn, team1_name, team1_player_ids)
    team2_id = _create_team(conn, team2_name, team2_player_ids)

    game_cursor = conn.execute(
        """
        INSERT INTO Games(team1, team2, score1, score2)
        VALUES (?, ?, ?, ?)
        """,
        (team1_id, team2_id, score1, score2),
    )
    conn.commit()
    return int(game_cursor.lastrowid)


def list_games(conn: sqlite3.Connection) -> list[sqlite3.Row]:
    conn.row_factory = sqlite3.Row
    return conn.execute(
        """
        SELECT
            g.id,
            t1.name AS team1_name,
            t2.name AS team2_name,
            g.score1,
            g.score2
        FROM Games g
        JOIN Teams t1 ON g.team1 = t1.id
        JOIN Teams t2 ON g.team2 = t2.id
        ORDER BY g.id
        """
    ).fetchall()


def _team_player_ids(conn: sqlite3.Connection, team_id: int) -> list[int]:
    rows = conn.execute(
        "SELECT player FROM TeamPlayers WHERE team = ?",
        (team_id,),
    ).fetchall()
    return [int(row[0]) for row in rows]


def recalculate_ratings(conn: sqlite3.Connection, k_factor: int = 2) -> None:
    """Recalculate current ratings from game history.

    Baseline: `cur_rating = rating` for each player.
    For every game:
    - winner side players gain `k_factor`
    - loser side players lose `k_factor`
    - draw: no change
    """
    conn.execute("UPDATE Players SET cur_rating = rating")

    games = conn.execute(
        "SELECT id, team1, team2, score1, score2 FROM Games ORDER BY id"
    ).fetchall()

    for _gid, team1_id, team2_id, score1, score2 in games:
        team1_players = _team_player_ids(conn, int(team1_id))
        team2_players = _team_player_ids(conn, int(team2_id))

        if score1 == score2:
            continue
        if score1 > score2:
            winners, losers = team1_players, team2_players
        else:
            winners, losers = team2_players, team1_players

        conn.executemany(
            "UPDATE Players SET cur_rating = cur_rating + ? WHERE id = ?",
            [(k_factor, pid) for pid in winners],
        )
        conn.executemany(
            "UPDATE Players SET cur_rating = cur_rating - ? WHERE id = ?",
            [(k_factor, pid) for pid in losers],
        )

    conn.commit()


def remove_game(conn: sqlite3.Connection, game_id: int) -> None:
    row = conn.execute("SELECT team1, team2 FROM Games WHERE id = ?", (game_id,)).fetchone()
    if row is None:
        raise ValueError(f"Unknown game id: {game_id}")

    team1_id, team2_id = int(row[0]), int(row[1])

    conn.execute("DELETE FROM Games WHERE id = ?", (game_id,))
    conn.execute("DELETE FROM TeamPlayers WHERE team IN (?, ?)", (team1_id, team2_id))
    conn.execute("DELETE FROM Teams WHERE id IN (?, ?)", (team1_id, team2_id))
    conn.commit()


def format_rows(rows: list[sqlite3.Row]) -> str:
    if not rows:
        return "(no records)"

    headers = rows[0].keys()
    widths = {h: len(h) for h in headers}
    for row in rows:
        for h in headers:
            widths[h] = max(widths[h], len(str(row[h])))

    sep = " | "
    header_line = sep.join(f"{h:{widths[h]}}" for h in headers)
    divider = "-+-".join("-" * widths[h] for h in headers)
    body = [sep.join(f"{str(row[h]):{widths[h]}}" for h in headers) for row in rows]
    return "\n".join([header_line, divider, *body])


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Football teams terminal manager")
    parser.add_argument("--db", default=DB_DEFAULT, help="Path to sqlite database file")

    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("init-db", help="Create DB tables and default virtual players")

    add_player = sub.add_parser("add-player", help="Create a new player")
    add_player.add_argument("--name", required=True)
    add_player.add_argument("--rating", type=int, required=True)
    add_player.add_argument("--inactive", action="store_true", help="Create as inactive player")

    sub.add_parser("list-players", help="List all players")
    sub.add_parser("list-ratings", help="List player ratings")
    sub.add_parser("list-games", help="List all games")
    sub.add_parser("recalculate-ratings", help="Rebuild current ratings from all games")

    remove_game_parser = sub.add_parser("remove-game", help="Remove a game by id")
    remove_game_parser.add_argument("--game-id", type=int, required=True)

    update_player_parser = sub.add_parser("update-player", help="Update player fields by id")
    update_player_parser.add_argument("--id", type=int, required=True)
    update_player_parser.add_argument("--name")
    update_player_parser.add_argument("--rating", type=int)
    update_active = update_player_parser.add_mutually_exclusive_group()
    update_active.add_argument("--active", action="store_true")
    update_active.add_argument("--inactive", action="store_true")

    add_game = sub.add_parser("create-game", help="Create a game from attendee names")
    add_game.add_argument("--team1", required=True, help="Comma-separated attendees for team 1")
    add_game.add_argument("--team2", required=True, help="Comma-separated attendees for team 2")
    add_game.add_argument("--score1", type=int, default=0)
    add_game.add_argument("--score2", type=int, default=0)

    return parser.parse_args()


def main() -> None:
    args = parse_args()
    db_path = Path(args.db)
    conn = get_connection(str(db_path))

    try:
        if args.command == "init-db":
            initialize_database(conn)
            print(f"Database initialized at: {db_path}")

        elif args.command == "add-player":
            player_id = create_player(conn, args.name, args.rating, active=not args.inactive)
            print(f"Created player #{player_id}: {args.name}")

        elif args.command == "list-players":
            print(format_rows(list_players(conn)))

        elif args.command == "list-ratings":
            print(format_rows(list_ratings(conn)))

        elif args.command == "list-games":
            print(format_rows(list_games(conn)))

        elif args.command == "recalculate-ratings":
            recalculate_ratings(conn)
            print("Ratings recalculated from game history.")

        elif args.command == "remove-game":
            remove_game(conn, args.game_id)
            print(f"Removed game #{args.game_id}")

        elif args.command == "update-player":
            active = None
            if args.active:
                active = True
            if args.inactive:
                active = False
            update_player(conn, args.id, name=args.name, rating=args.rating, active=active)
            print(f"Updated player #{args.id}")

        elif args.command == "create-game":
            team1 = [x.strip() for x in args.team1.split(",") if x.strip()]
            team2 = [x.strip() for x in args.team2.split(",") if x.strip()]
            game_id = create_game_with_attendees(conn, team1, team2, args.score1, args.score2)
            print(f"Created game #{game_id}")

    except ValueError as exc:
        print(f"Error: {exc}")
    finally:
        conn.close()


if __name__ == "__main__":
    main()
