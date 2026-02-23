# Python Terminal Guide (No UI)

This project intentionally uses **only terminal commands** and reusable Python functions.

## 1) Requirements

- Python 3.10+
- No external dependencies (uses only Python standard library)

## 2) Main file

- `python_cli/team_manager.py`

## 3) Database

Default DB file:

- `pootjeover_py.db` (local file, ignored by git)

If needed, recreate it:

```bash
python3 python_cli/team_manager.py --db pootjeover_py.db init-db
```

This creates tables:

- `Players`
- `Teams`
- `TeamPlayers`
- `Games`

And seeds virtual players:

- `XX34`
- `XX45`
- `Subs`

## 4) Core commands

### Initialize DB

```bash
python3 python_cli/team_manager.py --db pootjeover_py.db init-db
```

### Add player

```bash
python3 python_cli/team_manager.py --db pootjeover_py.db add-player --name "Alice" --rating 62
```

### Update player

```bash
python3 python_cli/team_manager.py --db pootjeover_py.db update-player --id 4 --rating 64 --active
```

### List players

```bash
python3 python_cli/team_manager.py --db pootjeover_py.db list-players
```

### Create game with attendees

```bash
python3 python_cli/team_manager.py --db pootjeover_py.db create-game --team1 "Alice,XX34" --team2 "Bob,Subs" --score1 5 --score2 3
```

### List games

```bash
python3 python_cli/team_manager.py --db pootjeover_py.db list-games
```

### Recalculate ratings

```bash
python3 python_cli/team_manager.py --db pootjeover_py.db recalculate-ratings
```

### List ratings

```bash
python3 python_cli/team_manager.py --db pootjeover_py.db list-ratings
```

### Remove game

```bash
python3 python_cli/team_manager.py --db pootjeover_py.db remove-game --game-id 1
```

## 5) Function-level API (inside `team_manager.py`)

You can also import and call these directly from Python scripts:

- `initialize_database(conn)`
- `create_player(conn, name, rating, active=True)`
- `update_player(conn, player_id, name=None, rating=None, active=None)`
- `create_game_with_attendees(conn, team1_attendees, team2_attendees, score1=0, score2=0)`
- `list_players(conn)`
- `list_games(conn)`
- `list_ratings(conn)`
- `remove_game(conn, game_id)`
- `recalculate_ratings(conn, k_factor=2)`

## 6) Typical flow

1. Init DB.
2. Add players.
3. Create games from attendee names.
4. Recalculate ratings.
5. List players/games/ratings.
