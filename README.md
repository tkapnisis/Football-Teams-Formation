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

## Database schema

The application automatically creates these tables on first run:

- `Players(id, name, rating, cur_rating, active)`
- `Teams(id, name)`
- `TeamPlayers(id, team, player)`
- `Games(id, team1, team2, score1, score2)`

Three virtual players (`XX34`, `XX45`, `Subs`) are inserted during initialization.

## Building

The project contains a Qt Creator `.pro` file (`src/PootjeOver.pro`). Build the application with Qt 5 and a C++ compiler that supports the Qt Charts and Qt Sql modules. On Windows a pre-built executable (`PootjeOver.exe`) and Qt runtime libraries are included.

