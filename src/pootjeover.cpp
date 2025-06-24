#include "pootjeover.h"
#include "ui_pootjeover.h"
#include "CheckBoxDelegate.h"

#include <QSql>
#include <QSqlQuery>
#include <QSqlRelation>
#include <QSqlRelationalDelegate>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

PootjeOver::PootjeOver(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::PootjeOver)
{
	ui->setupUi(this);

	if (!openDatabase()) {
		QMessageBox::critical(this, tr("Open database failed"), lastError());
		exit(1);
	}

	setupPlayersTab();
	setupTeamsTab();
	setupGamesTab();
	setupGraphTab();
}

PootjeOver::~PootjeOver()
{
	delete ui;
}

void PootjeOver::setupPlayersTab(void)
{
	m_player_model = new QSqlTableModel(this, m_db);
	m_player_model->setTable("Players");

	m_player_model->setHeaderData(0, Qt::Horizontal, tr("id"));
	m_player_model->setHeaderData(1, Qt::Horizontal, tr("name"));
	m_player_model->setHeaderData(2, Qt::Horizontal, tr("rating"));
	m_player_model->setHeaderData(3, Qt::Horizontal, tr("current rating"));
	m_player_model->setHeaderData(4, Qt::Horizontal, tr("Active  "));

	m_player_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
	m_player_model->select();

	ui->playerTable->setModel(m_player_model);
	ui->playerTable->setItemDelegateForColumn(4, new CheckBoxDelegate(ui->playerTable));
	ui->playerTable->resizeColumnsToContents();
}

void PootjeOver::setupTeamsTab(void)
{
	m_team_model = new QSqlRelationalTableModel(this, m_db);
	m_team_model->setTable("TeamPlayers");

	m_team_model->setHeaderData(0, Qt::Horizontal, tr("id"));
	m_team_model->setHeaderData(1, Qt::Horizontal, tr("team"));
	m_team_model->setRelation(1, QSqlRelation("Teams", "id", "name"));
	m_team_model->setHeaderData(2, Qt::Horizontal, tr("player"));
	m_team_model->setRelation(2, QSqlRelation("Players", "id", "name"));

	m_team_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
	m_team_model->select();

	ui->teamTable->setModel(m_team_model);
	ui->teamTable->setItemDelegate(new QSqlRelationalDelegate(ui->teamTable));
	ui->teamTable->resizeColumnsToContents();
}


void PootjeOver::setupGamesTab(void)
{
	m_game_model = new QSqlRelationalTableModel(this, m_db);
	m_game_model->setTable("Games");

	m_game_model->setHeaderData(0, Qt::Horizontal, tr("id"));
	m_game_model->setHeaderData(1, Qt::Horizontal, tr("team1"));
	m_game_model->setRelation(1, QSqlRelation("Teams", "id", "name"));
	m_game_model->setHeaderData(2, Qt::Horizontal, tr("team2"));
	m_game_model->setRelation(2, QSqlRelation("Teams", "id", "name"));
	m_game_model->setHeaderData(3, Qt::Horizontal, tr("score1"));
	m_game_model->setHeaderData(4, Qt::Horizontal, tr("score2"));

	m_game_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
	m_game_model->select();

	ui->gameTable->setModel(m_game_model);
	ui->gameTable->setItemDelegate(new QSqlRelationalDelegate(ui->gameTable));
	ui->gameTable->resizeColumnsToContents();
}

void PootjeOver::setupGraphTab(void)
{
	m_rating_chart.setTitle("Rating conversion");
	ui->ratingGraph->setChart(&m_rating_chart);
	ui->ratingGraph->setRenderHint(QPainter::Antialiasing);
}

/* Keep in SYNC with below createtable !! */
#define XX34_ID	1
#define XX45_ID 2
#define SUBS_ID 3
#define MAX_VIRTUAL_ID 3

bool PootjeOver::createDatabaseTables(QSqlDatabase& db, bool init)
{
	QString createtable;

	createtable = "CREATE TABLE ";
	createtable += "IF NOT EXISTS ";
	createtable += "`Players` (";
	createtable += "	`id`		INTEGER,";
	createtable += "	`name`		TEXT NOT NULL UNIQUE COLLATE NOCASE,";
	createtable += "	`rating`	INTEGER DEFAULT 0,";
	createtable += "	`cur_rating`	INTEGER DEFAULT 0,";
	createtable += "	`active`	BOOLEAN DEFAULT 1,";
	createtable += "	PRIMARY KEY(`id`)";
	createtable += ")";
	db.exec(createtable);
	if (db.lastError().type() != QSqlError::NoError)
		return false;

	if (init) {
		/* Initial setup, set the virtual players */
		createtable = "INSERT OR IGNORE INTO ";
		createtable += "Players(id, name, rating, cur_rating) VALUES ";
		createtable += " (1, 'XX34', 50, 50), ";
		createtable += " (2, 'XX45', 50, 50), ";
		createtable += " (3, 'Subs', 50, 50);";
		db.exec(createtable);
		if (db.lastError().type() != QSqlError::NoError)
			return false;
	}

	createtable = "CREATE TABLE ";
	createtable +="IF NOT EXISTS ";
	createtable += "`Teams` (";
	createtable += "	`id`	INTEGER,";
	createtable += "	`name`	TEXT NOT NULL,";
	createtable += "	PRIMARY KEY(`id`)";
	createtable += ");";
	db.exec(createtable);
	if (db.lastError().type() != QSqlError::NoError)
		return false;

	createtable = "CREATE TABLE ";
	createtable += "IF NOT EXISTS ";
	createtable += "`TeamPlayers` (";
	createtable += "	`id`		INTEGER,";
	createtable += "	`team`		INTEGER NOT NULL,";
	createtable += "	`player`	INTEGER NOT NULL,";
	createtable += "	PRIMARY KEY(`id`),";
	createtable += "	FOREIGN KEY(`team`) REFERENCES `Teams`(`id`),";
	createtable += "	FOREIGN KEY(`player`) REFERENCES `Players`(`id`)";
	createtable += ");";
	db.exec(createtable);
	if (db.lastError().type() != QSqlError::NoError)
		return false;

	createtable = "CREATE TABLE ";
	createtable += "IF NOT EXISTS ";
	createtable += "`Games` (";
	createtable += "	`id`		INTEGER,";
	createtable += "	`team1`		INTEGER NOT NULL,";
	createtable += "	`team2`		INTEGER NOT NULL,";
	createtable += "	`score1`	INTEGER DEFAULT 0,";
	createtable += "	`score2`	INTEGER DEFAULT 0,";
	createtable += "	PRIMARY KEY(`id`),";
	createtable += "	FOREIGN KEY(`team1`) REFERENCES `Teams`(`id`),";
	createtable += "	FOREIGN KEY(`team2`) REFERENCES `Teams`(`id`)";
	createtable += ");";
	db.exec(createtable);
	if (db.lastError().type() != QSqlError::NoError)
		return false;

	return true;
}

bool PootjeOver::openDatabase(void)
{
	bool result;

	m_lasterror = QSqlError();

	m_db = QSqlDatabase::addDatabase("QSQLITE", "pootjeover.db");
	m_db.setDatabaseName("pootjeover.db");
	result = m_db.open();
	if (result) {
		result = createDatabaseTables(m_db, true);
		if (!result)
			m_lasterror = m_db.lastError();
	} else {
		m_lasterror = m_db.lastError();
	}
	return result;
}

void PootjeOver::closeDatabase(void)
{
	m_db.close();
	m_lasterror = m_db.lastError();
}

QString PootjeOver::lastError()
{
	switch (m_lasterror.type()) {
	case QSqlError::NoError:
		return "No error";
	case QSqlError::ConnectionError:
		return "ConnectionError: "+m_lasterror.text();
	case QSqlError::StatementError:
		return "StatementError: "+m_lasterror.text();
	case QSqlError::TransactionError:
		return "TransactionError: "+m_lasterror.text();
	case QSqlError::UnknownError:
		return "Unknown error occured: "+m_lasterror.text();
	}

	return "Unhandled error occured: "+m_lasterror.text();
}

void PootjeOver::on_addPlayer_clicked()
{
	QString name = QInputDialog::getText(this, tr("Player"), tr("Name: "), QLineEdit::Normal);
	int rating = QInputDialog::getInt(this, tr("Player"), tr("Rating: "), QLineEdit::Normal);

	if (!name.isEmpty()) {
		QSqlQuery rs(m_db);
		rs.setForwardOnly(true);

		rs.prepare(QString("INSERT INTO [Players] (name, rating, cur_rating) VALUES (:name, :rating, :cur_rating);"));
		rs.bindValue(":name", name);
		rs.bindValue(":rating", rating);
		/* New player current rating is equal to initial rating */
		rs.bindValue(":cur_rating", rating);

		if (!rs.exec()) {
			m_lasterror = rs.lastError();
			QMessageBox::warning(this, tr("Add failed"), lastError());
		} else {
			m_player_model->select();
			ui->playerTable->resizeColumnsToContents();
		}
	}
}

void PootjeOver::on_updatePlayer_clicked()
{
	m_player_model->database().transaction();
	if (m_player_model->submitAll()) {
		m_player_model->database().commit();

		update_rating_in_DB();
	} else {
		m_lasterror = m_player_model->database().lastError();
		m_player_model->database().rollback();
		QMessageBox::warning(this, (tr("Update failed")), lastError());
	}
}

void PootjeOver::on_updateTeam_clicked()
{
	m_team_model->database().transaction();
	if (m_team_model->submitAll()) {
		m_team_model->database().commit();

		update_rating_in_DB();
	} else {
		m_lasterror = m_team_model->database().lastError();
		m_team_model->database().rollback();
		QMessageBox::warning(this, (tr("Update failed")), lastError());
	}
}

void PootjeOver::on_addGame_clicked()
{
	/* Manual selection show everybody */
	QMap<int,QString> players = getPlayerNames(false, true);
	QMap<int,QString> left = players;

	QList<int> color = getTeam(tr("Color Player"), players, left);
	QList<int> white = getTeam(tr("White Player"), players, left);

	if (!color.isEmpty() && !white.isEmpty()) {
		addGame(color, white);
	}
}

void PootjeOver::on_updateGame_clicked()
{
	m_game_model->database().transaction();
	if (m_game_model->submitAll()) {
		m_game_model->database().commit();

		update_rating_in_DB();
	} else {
		m_lasterror = m_game_model->database().lastError();
		m_game_model->database().rollback();
		QMessageBox::warning(this, (tr("Update failed")), lastError());
	}
}

QList<QMap<int, double>> PootjeOver::determine_ratings()
{
	int subs_present = 0;
	QSqlQuery rs(m_db);
	rs.setForwardOnly(true);

	QMap<int, double> new_rating;
	QList<QMap<int, double>> ratings;

	/* Fill array with the initial rating of all players, will be adjusted during the loop */
	rs.exec("SELECT id,rating FROM Players ORDER BY id ASC;");
	while (rs.next())
		new_rating.insert(rs.value(0).toInt(), static_cast<double>(rs.value(1).toInt()));

	if (new_rating.size() == 0) {
		m_lasterror = rs.lastError();
		QMessageBox::warning(this, (tr("Failed to initial ratings")), lastError());
		return ratings;
	}
	ratings << new_rating;

	/* Now walk through all the games and ajust rating each time */
	rs.exec("SELECT team1,team2,score1,score2 FROM Games;");
	while (rs.next()) {
		QSqlQuery rs2(m_db);

		int score_diff;
		int score1;
		int score2;

		double rank_diff;
		double rank1;
		double rank2;

		QList<int> team1;
		QList<int> team2;

		/* Get list of players playing the team */
		rs2.prepare("SELECT player FROM TeamPlayers WHERE team = :team_id");
		/* Get first team */
		rs2.bindValue(":team_id", rs.value(0));
		rs2.exec();
		while (rs2.next())
			team1 << rs2.value(0).toInt();

		if (team1.size() == 0) {
			m_lasterror = rs2.lastError();
			QMessageBox::warning(this, (tr("Failed to determine team1")), lastError());
			ratings.clear();
			return ratings;
		}

		/* Get second team */
		rs2.bindValue(":team_id", rs.value(1));
		rs2.exec();
		while (rs2.next())
			team2 << rs2.value(0).toInt();

		if (team2.size() == 0) {
			m_lasterror = rs2.lastError();
			QMessageBox::warning(this, (tr("Failed to determine team2")), lastError());
			ratings.clear();
			return ratings;
		}

		/* Get scores from the query */
		score1 = rs.value(2).toInt();
		score2 = rs.value(3).toInt();

		/* Determine goal difference (possitive if team1 stronger, negative if team2 stronger */
		score_diff = score1 - score2;

		/* Determine actual team1 ranking */
		rank1 = 0;
		for (int i = 0; i < team1.size(); i++) {
			if (team1.at(i) == SUBS_ID)
				subs_present = 1;
			else 
				rank1 += new_rating[team1.at(i)];
		}
		
		/* Determine actual team2 ranking */
		rank2 = 0;
		for (int i = 0; i < team2.size(); i++) {
			if (team1.at(i) == SUBS_ID)
				subs_present = 2;
			else 
				rank2 += new_rating[team2.at(i)];
		}
		
		/* Spare players substract average */
		if (subs_present == 1) {
			rank2 -= (rank2 / team2.size());
		} else if (subs_present == 2) {
			rank1 -= (rank1 / team1.size());
		}
		/* Determine rating difference (possitive if team1 stronger, negative if team2 stronger */
		rank_diff = (rank1 - rank2) / (team1.size() + team2.size());

		/*
		 * Determine the new rating
		 * We expect that the team with the higher rating to win, where the difference in goals translates to
		 * 1 goal foreach rating point (averaged over the players) difference.
		 * rank_diff contains the rating difference averaged over the players
		 * score_diff contains the difference in goals.
		 * So rating needs to be adjusted with rank_diff - score_diff
		 * Both diffs are determined team1-team2, so subtract from team1 players and add to team2 players
		 *
		 * Examples:
		 * 4 against 4, with rating 408 against 400
		 *  => expectation == 1 goal difference for team1
		 *  => actual score 11-10
		 *  => score_diff = 1 == (11 - 10)
		 *  => rank_diff = 1  == ((408 - 400) / 8)
		 *  => rating adjusted with 0, which is correct because prediction matches actual result
		 *
		 * 5 against 5, with rating 550 against 500
		 *  => expectation == 5 goals difference for team1
		 *  => actual score 10-8
		 *  => score_diff = 2 == (10 - 8)
		 *  => rank_diff = 5  == ((550 - 500) / 10)
		 *  => rating adjustment == 3, remove from team1 players (ie they underperformed, should have won with +5)
		 *				add to team2 players (ie they overperformed, should have lost with -5)
		 *
		 * Interchanging the scores from above example
		 * 5 against 5, with rating 500 against 550
		 *  => expectation == 5 goals difference for team2
		 *  => actual score 8-10
		 *  => score_diff = -2 == (8 - 10)
		 *  => rank_diff = -5  == ((500 - 550) / 10)
		 *  => rating adjustment == -3, add to team1 players (ie they overperformed, should have lost with -5)
		 *				remove from team2 (ie they underperformed, should have won with +5)
		 */
		/* Adjust the rating based on predicted score and actual achieved difference */
		for (int i = 0; i < team1.size(); i++) {
			double new_rank = new_rating[team1.at(i)] - (rank_diff - score_diff);
			new_rating[team1.at(i)] = new_rank;
		}
		for (int i = 0; i < team2.size(); i++) {
			double new_rank = new_rating[team2.at(i)] + (rank_diff - score_diff);
			new_rating[team2.at(i)] = new_rank;
		}
		new_rating[SUBS_ID] = 0;

#if 0
		Old, set 'bank' as virtual player and thread equal to the xx34 / xxx45

		/* Rating of the 'bank' is the average rating of all normal players */
		double val;
		double group_rating = 0;
		foreach(val, new_rating.values())
			group_rating += val;
		group_rating -= new_rating[XX34_ID];
		group_rating -= new_rating[XX45_ID];
		group_rating -= new_rating[SUBS_ID];
		group_rating /= (new_rating.size() - MAX_VIRTUAL_ID);
		new_rating[SUBS_ID] = group_rating;
#endif
		ratings << new_rating;
	}

	return ratings;
}

void PootjeOver::update_rating_in_DB(void)
{
	QList<QMap<int, double>> ratings = determine_ratings();
	if (ratings.isEmpty())
		return;

	QMap<int, double> last_rating = ratings.last();
	QSqlQuery rs(m_db);

	rs.prepare("UPDATE Players SET cur_rating = :rating WHERE id = :player");
	m_db.transaction();
	int player_id;
	foreach(player_id, last_rating.keys()) {
		rs.bindValue(":rating", static_cast<int>(last_rating[player_id]));
		rs.bindValue(":player", player_id);
		if (!rs.exec()) {
			m_lasterror = rs.lastError();
			m_db.rollback();
			QMessageBox::warning(this, tr("Rating update failed"), lastError());
			return;
		}
	}
	m_db.commit();

	/* Force refresh of the GUI */
	m_player_model->select();
	ui->playerTable->resizeColumnsToContents();
}

#include <algorithm>
template <typename Iterator>
static bool next_combination(const Iterator first, Iterator k, const Iterator last)
{
   /* Credits: Mark Nelson http://marknelson.us */
   if ((first == last) || (first == k) || (last == k))
      return false;
   Iterator i1 = first;
   Iterator i2 = last;
   ++i1;
   if (last == i1)
      return false;
   i1 = last;
   --i1;
   i1 = k;
   --i2;
   while (first != i1)
   {
      if (*--i1 < *i2)
      {
	 Iterator j = k;
	 while (!(*i1 < *j)) ++j;
	 std::iter_swap(i1,j);
	 ++i1;
	 ++j;
	 i2 = k;
	 std::rotate(i1,j,last);
	 while (last != j)
	 {
	    ++j;
	    ++i2;
	 }
	 std::rotate(k,i2,last);
	 return true;
      }
   }
   std::rotate(first,k,last);
   return false;
}

void PootjeOver::on_newGame_clicked()
{
	QSqlQuery rs(m_db);
	rs.setForwardOnly(true);

	QString name;
	/* Automatic calculation only active players, virtual not needed */
	QMap<int,QString> players = getPlayerNames(true, false);
	QMap<int,QString> input = players;

	QList<int> attendance = getTeam(tr("Select player"), players, input);
	QList<QMap<int, double>> ratings = determine_ratings();
	QMap<int, double> rating = ratings.last();
	double rank1 = 0;
	double rank2 = 0;

	if (attendance.size() == 0) {
		QMessageBox::information(this, "Info", "No attendances selected");
		return;
	}

	/* Check if virtual player is needed */
	if ((attendance.size() > 10 && attendance.size()%2))
		attendance << SUBS_ID;
	else if (attendance.size() == 9)
		attendance << XX45_ID;
	else if (attendance.size() == 7)
		attendance << XX34_ID;
	else if (attendance.size() < 6) {
		QMessageBox::information(this, "Info", "Attendance too low for model");
		return;
	}

	int team_size = attendance.size()/2;
	/* next_combination expects a sorted list */
	std::sort(attendance.begin(), attendance.end());

	int first_player = attendance[0];
	QList<QList<int>> teams;
	do
	{
		/* If the first element is changed next_combination is at the half of
		 * all possible combinations. This is enough for use (ie the other half of
		 * the combinations is the same only change of team color
		 */
		if (attendance[0] != first_player)
		    break;
		teams << attendance;
	} while (next_combination(attendance.begin(), attendance.begin() + team_size, attendance.end()));

	double team_diff = std::numeric_limits<double>::max();
	for (int i = 0; i < teams.size(); i++) {
		rank1 = 0;
		rank2 = 0;
		for (int j = 0; j < team_size; j++)
			rank1 += rating[teams[i][j]];
		for (int j = team_size; j < attendance.size(); j++)
			rank2 += rating[teams[i][j]];
		if (abs(rank1 - rank2) < team_diff) {
			team_diff = abs(rank1 - rank2);
			active_team = teams[i];
		}
	}

	QString team1 = "";
	QString team2 = "";

	/* (re)get all active players (including virtual now) for displaying */
	players = getPlayerNames(true, true);
	rank1 = 0;
	rank2 = 0;

	for (int j = 0; j < team_size; j++) {
		team1 += players[active_team[j]];
		team1 += " ";
		rank1 += rating[active_team[j]];
	}
	team1 += QString("(%1)").arg(rank1);

	for (int j = team_size; j < attendance.size(); j++) {
		team2 += players[active_team[j]];
		team2 += " ";
		rank2 += rating[active_team[j]];
	}
	team2 += QString("(%1)").arg(rank2);

	int goals = getAverageScore();
	int team1_score = (goals + (rank1-rank2)/attendance.size() + 1)/2;
	int team2_score = (goals - (rank1-rank2)/attendance.size() + 1)/2;

	QString predict_score = QString("%1 - %2").arg(team1_score).arg(team2_score);
	QString outcome = QString("Team KLEUR      :  %1\n"
				  "Team WIT        :  %2\n"
				  "Predicted Score :  %3\n").arg(team1).arg(team2).arg(predict_score);
	ui->outcome->setPlainText(outcome);
	ui->acceptGame->setEnabled(true);
}

void PootjeOver::addGame(const QList<int>& color, const QList<int>& white)
{
	QSqlQuery rs(m_db);
	rs.setForwardOnly(true);

	int color_id;
	int white_id;

	m_db.transaction();

	rs.prepare(QString("INSERT INTO Teams (name) VALUES (:name);"));

	rs.bindValue(":name", "color");
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Create color team failed"), lastError());
		return;
	}
	color_id = rs.lastInsertId().toInt();

	rs.bindValue(":name", "white");
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Create white team failed"), lastError());
		return;
	}
	white_id = rs.lastInsertId().toInt();

	rs.prepare(QString("INSERT INTO [TeamPlayers] (team, player) VALUES (:team_id, :player);"));
	for (const auto& player : color) {
		rs.bindValue(":team_id", color_id);
		rs.bindValue(":player", player);
		if (!rs.exec()) {
			m_lasterror = rs.lastError();
			m_db.rollback();
			QMessageBox::warning(this, tr("Add player to color team failed"), lastError());
			return;
		}
	}

	for (const auto& player : white) {
		rs.bindValue(":team_id", white_id);
		rs.bindValue(":player", player);
		if (!rs.exec()) {
			m_lasterror = rs.lastError();
			m_db.rollback();
			QMessageBox::warning(this, tr("Add player to white team failed"), lastError());
			return;
		}
	}

	rs.prepare(QString("INSERT INTO [Games] (team1, team2) VALUES (:color_id, :white_id);"));
	rs.bindValue(":color_id", color_id);
	rs.bindValue(":white_id", white_id);

	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Add failed"), lastError());
		return;
	}
	m_db.commit();

	m_team_model->select();
	ui->teamTable->resizeColumnsToContents();

	m_game_model->select();
	ui->gameTable->resizeColumnsToContents();
}

void PootjeOver::on_acceptGame_clicked()
{
	QList<int> color;
	QList<int> white;

	ui->acceptGame->setEnabled(false);

	if (active_team.size() == 0)
		return;

	color = active_team.mid(0, active_team.size() / 2);
	white = active_team.mid(active_team.size() / 2, active_team.size());

	addGame(color, white);
	active_team.clear();
	ui->outcome->clear();
}

QMap<int,QString> PootjeOver::getPlayerNames(bool active_only, bool include_virtual)
{
	QSqlQuery rs(m_db);
	QString query;
	rs.setForwardOnly(true);

	QMap<int,QString> players;

	query = "SELECT id,name FROM Players";
	if (active_only || include_virtual)
		query += QString(" WHERE active='1'");
	else if (active_only && !include_virtual)
		query += QString(" WHERE active='1' and id > '%1'").arg(MAX_VIRTUAL_ID);
	else if (!include_virtual)
		query += QString(" WHERE id > '%1'").arg(MAX_VIRTUAL_ID);
	query += " ORDER by id ASC";

	rs.exec(query);
	players.insert(0, ""); /* id 0 == empty string */
	while (rs.next()) {
	    if (include_virtual || (!include_virtual && rs.value(0).toInt() > MAX_VIRTUAL_ID))
		players.insert(rs.value(0).toInt(), rs.value(1).toString());
	}

	return players;
}

QList<int> PootjeOver::getTeam(const QString title, const QMap<int,QString>& players, QMap<int,QString>& left)
{
	QString name;
	QList<int> team;
	bool ok;

	do {
		name = QInputDialog::getItem(this, title, tr("player: "), left.values(), 0, false, &ok);
		if (ok && !name.isEmpty()) {
			team << players.key(name);
			left.remove(players.key(name));
		}
	} while (!name.isEmpty() && ok);

	return team;
}

int PootjeOver::getAverageScore(void)
{
	QSqlQuery rs(m_db);
	rs.setForwardOnly(true);
	int score = 0;

	rs.exec("SELECT avg(score1),avg(score2) FROM Games;");
	while (rs.next())
		score = (rs.value(0).toInt() + rs.value(1).toInt());
	return score;
}

void PootjeOver::on_removeGame_clicked()
{
	QSqlQuery rs(m_db);

	int gameId;
	int team1;
	int team2;

	m_db.transaction();

	/* Select the last game */
	rs.prepare(QString("SELECT id,team1,team2 FROM [Games] ORDER BY ID DESC LIMIT 1"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to select last game"), lastError());
		return;
	}
	if (!rs.next()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to select last game"), lastError());
		return;
	}

	gameId = rs.value(0).toInt();
	team1 = rs.value(1).toInt();
	team2 = rs.value(2).toInt();

	rs.prepare(QString("DELETE FROM [TeamPlayers] WHERE team = :team_id"));
	rs.bindValue(":team_id", team1);
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to remove players from team1"), lastError());
		return;
	}
	rs.bindValue(":team_id", team2);
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to remove players from team2"), lastError());
		return;
	}

	rs.prepare(QString("DELETE FROM [Games] WHERE id = :game_id"));
	rs.bindValue(":game_id", gameId);
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to remove game"), lastError());
		return;
	}

	rs.prepare(QString("DELETE FROM [Teams] WHERE id = :team_id"));
	rs.bindValue(":team_id", team1);
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to remove team1"), lastError());
		return;
	}

	rs.bindValue(":team_id", team2);
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to remove team2"), lastError());
		return;
	}

	m_db.commit();

	m_team_model->select();
	ui->teamTable->resizeColumnsToContents();

	m_game_model->select();
	ui->gameTable->resizeColumnsToContents();
}

void PootjeOver::on_showRating_clicked()
{
	QtCharts::QLineSeries *serie;
	/* get all (active) real players */
	QMap<int,QString> players = getPlayerNames(true, false);
	QString player;

	QPen pen[] = {
		QPen(QBrush(Qt::darkBlue), 2, Qt::SolidLine),
		QPen(QBrush(Qt::darkGreen), 2, Qt::SolidLine),
		QPen(QBrush(Qt::darkMagenta), 2, Qt::SolidLine),
		QPen(QBrush(Qt::darkCyan), 2, Qt::SolidLine),
		QPen(QBrush(Qt::darkRed), 2, Qt::SolidLine),
		QPen(QBrush(Qt::blue), 2, Qt::SolidLine),
		QPen(QBrush(Qt::green), 2, Qt::SolidLine),
		QPen(QBrush(Qt::magenta), 2, Qt::SolidLine),
		QPen(QBrush(Qt::cyan), 2, Qt::SolidLine),
		QPen(QBrush(Qt::red), 2, Qt::SolidLine),
		/* Repeat with dashes */
		QPen(QBrush(Qt::darkBlue), 2, Qt::DashLine),
		QPen(QBrush(Qt::darkGreen), 2, Qt::DashLine),
		QPen(QBrush(Qt::darkMagenta), 2, Qt::DashLine),
		QPen(QBrush(Qt::darkCyan), 2, Qt::DashLine),
		QPen(QBrush(Qt::darkRed), 2, Qt::DashLine),
		QPen(QBrush(Qt::blue), 2, Qt::DashLine),
		QPen(QBrush(Qt::green), 2, Qt::DashLine),
		QPen(QBrush(Qt::magenta), 2, Qt::DashLine),
		QPen(QBrush(Qt::cyan), 2, Qt::DashLine),
		QPen(QBrush(Qt::red), 2, Qt::DashLine),
		/* Repeat with dots */
		QPen(QBrush(Qt::darkBlue), 2, Qt::DotLine),
		QPen(QBrush(Qt::darkGreen), 2, Qt::DotLine),
		QPen(QBrush(Qt::darkMagenta), 2, Qt::DotLine),
		QPen(QBrush(Qt::darkCyan), 2, Qt::DotLine),
		QPen(QBrush(Qt::darkRed), 2, Qt::DotLine),
		QPen(QBrush(Qt::blue), 2, Qt::DotLine),
		QPen(QBrush(Qt::green), 2, Qt::DotLine),
		QPen(QBrush(Qt::magenta), 2, Qt::DotLine),
		QPen(QBrush(Qt::cyan), 2, Qt::DotLine),
		QPen(QBrush(Qt::red), 2, Qt::DotLine),
	};
	int pen_idx = 0;
	const int pens = sizeof(pen)/sizeof(pen[0]);

	/* cleanup possible old graph */
	if (m_rating_series.size()) {
		m_rating_chart.removeAllSeries();
		m_rating_series.clear();
	}

	QList<QMap<int, double>> ratings = determine_ratings();
	QMap<int, double> rating;
	QMap<int, double> start_rating;

	foreach(player, players) {
		if (player.isEmpty())
			continue;
		serie = new QtCharts::QLineSeries();
		serie->setName(player);
		m_rating_series.insert(players.key(player), serie);
	}

	int idx = 0;
	start_rating = ratings.first();
	foreach(rating, ratings) {
		auto it = rating.begin();
		while (it != rating.end()) {
			if (players.find(it.key()) != players.end()) {
				double val = it.value();
				val -= start_rating[it.key()];
				m_rating_series[it.key()]->append(idx, val);
			}
			++it;
		}
		++idx;
	}

	foreach(serie, m_rating_series) {
		serie->setPointsVisible(true);
		serie->setPen(pen[pen_idx]);
		pen_idx = ((pen_idx + 1) % pens);
		m_rating_chart.addSeries(serie);
	}
	m_rating_chart.createDefaultAxes();
}


void PootjeOver::on_actionInfo_triggered()
{
    QMessageBox::information(this, "PootjeOver About", "PootjeOver v1.0\n\n © 2020 Bart vdr. Meulen\n\n Original MathLab code © Tom Clabbers");
}

void PootjeOver::on_actionStart_new_triggered()
{
	QSqlDatabase backup_db;
	QSqlQuery rs(m_db);


	bool ok;
	QString season = QInputDialog::getText(this, tr("Old season name"), tr("name:"), QLineEdit::Normal, "", &ok);
	if (!ok || season.isEmpty())
		return;

	backup_db = QSqlDatabase::addDatabase("QSQLITE", "pootjeover_" + season + ".db");
	backup_db.setDatabaseName("pootjeover_" + season + ".db");
	if (backup_db.open()) {
		if (!createDatabaseTables(backup_db, false)) {
			m_lasterror = backup_db.lastError();
			QMessageBox::warning(this, tr("Failed to setup backup database"), lastError());
			return;
		}
	} else {
		m_lasterror = backup_db.lastError();
		QMessageBox::warning(this, tr("Failed to open backup database"), lastError());
		return;
	}
	backup_db.close();


	m_db.transaction();
	/* Create connection to the newly created database */
	rs.prepare(QString("ATTACH DATABASE '%1' AS backup_db").arg("pootjeover_" + season + ".db"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to create database connection"), lastError());
		return;
	}

	/* Backup the Players */
	rs.prepare(QString("INSERT INTO backup_db.Players SELECT * FROM [Players]"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to copy players"), lastError());
		return;
	}

	/* Backup the Teams step1 */
	rs.prepare(QString("INSERT INTO backup_db.Teams SELECT * FROM [Teams]"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to copy teams (1)"), lastError());
		return;
	}

	/* Backup the Teams step2 */
	rs.prepare(QString("INSERT INTO backup_db.TeamPlayers SELECT * FROM [TeamPlayers]"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to copy teams (2)"), lastError());
		return;
	}

	/* Backup the Games */
	rs.prepare(QString("INSERT INTO backup_db.Games SELECT * FROM [Games]"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to copy Games"), lastError());
		return;
	}
	m_db.commit();

	/* Close the backup */
	rs.prepare(QString("DETACH DATABASE backup_db"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to close backup"), lastError());
		return;
	}

	/* Cleanup / reset the current database */
	m_db.transaction();
	rs.prepare(QString("DELETE FROM [Games]"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to cleanup games"), lastError());
		return;
	}

	rs.prepare(QString("DELETE FROM [TeamPlayers]"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to cleanup teams (1)"), lastError());
		return;
	}
	rs.prepare(QString("DELETE FROM [Teams]"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to cleanup teams (2)"), lastError());
		return;
	}

	rs.prepare(QString("DELETE FROM [Players] WHERE active = '0'"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to cleanup players (1)"), lastError());
		return;
	}

	/* Last, set the 'base' rating to the current (season final) one */
	rs.prepare(QString("UPDATE [Players] SET rating = cur_rating WHERE rating != cur_rating"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to cleanup players (2)"), lastError());
		return;
	}
	/* Done */
	m_db.commit();

	/* Refresh GUI */
	m_team_model->select();
	ui->teamTable->resizeColumnsToContents();
	m_game_model->select();
	ui->gameTable->resizeColumnsToContents();
	m_player_model->select();
	ui->playerTable->resizeColumnsToContents();
}

void PootjeOver::on_showLast_clicked()
{
	QSqlQuery rs(m_db);

	int team1Id;
	int team2Id;

	double rank1;
	double rank2;

	QList<int> team1;
	QList<int> team2;

	QString Team1players = "";
	QString Team2players = "";

	QList<QMap<int, double>> ratings = determine_ratings();
	QMap<int,QString> players = getPlayerNames(false, true);
	QMap<int, double> rating = ratings[ratings.size() - 2];

	/* Select the last game */
	rs.prepare(QString("SELECT id,team1,team2 FROM [Games] ORDER BY ID DESC LIMIT 1"));
	if (!rs.exec()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to select last game"), lastError());
		return;
	}
	if (!rs.next()) {
		m_lasterror = rs.lastError();
		m_db.rollback();
		QMessageBox::warning(this, tr("Failed to select last game"), lastError());
		return;
	}

	team1Id = rs.value(1).toInt();
	team2Id = rs.value(2).toInt();

	rs.prepare(QString("SELECT player FROM [TeamPlayers] WHERE team = :team_id"));
	rs.bindValue(":team_id", team1Id);
	rs.exec();
	while (rs.next())
		team1 << rs.value(0).toInt();
	if (team1.size() == 0) {
		m_lasterror = rs.lastError();
		QMessageBox::warning(this, (tr("Failed to determine team1")), lastError());
		return;
	}

	/* Get second team */
	rs.bindValue(":team_id", team2Id);
	rs.exec();
	while (rs.next())
		team2 << rs.value(0).toInt();
	if (team2.size() == 0) {
		m_lasterror = rs.lastError();
		QMessageBox::warning(this, (tr("Failed to determine team1")), lastError());
		return;
	}

	rank1 = 0;
	foreach (team1Id, team1) {
		Team1players += players[team1Id];
		Team1players += " ";
		rank1 += rating[team1Id];
	}
	Team1players += QString("(%1)").arg(rank1);

	rank2 = 0;
	foreach (team2Id, team2) {
		Team2players += players[team2Id];
		Team2players += " ";
		rank2 += rating[team2Id];
	}
	Team2players += QString("(%1)").arg(rank2);

	QString outcome = QString("Team KLEUR     :  %1\n"
				  "Team WIT       :  %2\n").arg(Team1players).arg(Team2players);
	ui->outcome->setPlainText(outcome);
}
