# Football Teams Formation

This project is a C++/Qt5 application that helps build balanced football teams and track player ratings over time. It uses an SQLite database (`pootjeover.db`) to store players, teams, games and ratings.

## Current features

- **Player management** – add new players with an initial rating, update their details and mark them active or inactive.
- **Team management** – create named teams and assign players to them.
- **Game tracking** – record games with two teams and final scores.
- **Rating calculation** – compute player ratings based on historical results. After each game, the algorithm adjusts each player’s score depending on the expected result versus the actual score difference.
- **Balanced team suggestion** – when starting a new game, the app suggests a split of available players into two sides with the smallest possible rating difference. Virtual players may be added if an odd number attend.
- **Rating history graph** – visualize rating progression per player across all games.
- **Persistent storage** – data is kept in `pootjeover.db` so ratings and results remain between sessions.
- **CLI player statistics** – compute per-player game results and goals from the database.

## Database schema

The application automatically creates these tables on first run:

- `Players(id, name, rating, cur_rating, active)`
- `Teams(id, name)`
- `TeamPlayers(id, team, player)`
- `Games(id, team1, team2, score1, score2)`

Three virtual players (`XX34`, `XX45`, `Subs`) are inserted during initialization.

## Building

The project contains a Qt Creator `.pro` file (`src/PootjeOver.pro`). Build the application with Qt 5 and a C++ compiler that supports the Qt Charts and Qt Sql modules. On Windows a pre-built executable (`PootjeOver.exe`) and Qt runtime libraries are included.

## Python CLI usage

Alongside the Qt application there is a lightweight CLI written in Python. Use
`python3 python_cli/main.py <command>` to operate on the same
`pootjeover.db` database. Available commands are:

- **`init-db`** – create the database if it doesn't exist.
  ```bash
  python3 python_cli/main.py init-db --db pootjeover.db
  ```

- **`add-player NAME RATING`** – insert a new player with an initial rating.
  ```bash
  python3 python_cli/main.py add-player "Alice" 1000 --db pootjeover.db
  ```

- **`record-game SCORE1 SCORE2 --team1 IDS --team2 IDS`** – store a match
  result. Player IDs for each team are given as comma-separated lists.
  ```bash
  python3 python_cli/main.py record-game 5 3 \
      --team1 1,2 --team2 3,4 --db pootjeover.db
  ```

- **`show-ratings`** – recompute and print the current rating for all players.
  ```bash
  python3 python_cli/main.py show-ratings --db pootjeover.db
  ```

- **`player-stats`** – display games played (GP), wins, draws, losses and total
  goals for/against for every player.
  ```bash
  python3 python_cli/main.py player-stats --db pootjeover.db
  ```

- **`suggest PLAYERS`** – suggest a balanced split for the comma-separated
  player ID list.
  ```bash
  python3 python_cli/main.py suggest 1,2,3,4 --db pootjeover.db
  ```
