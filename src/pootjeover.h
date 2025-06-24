#ifndef POOTJEOVER_H
#define POOTJEOVER_H

#include <QMainWindow>
#include <QSql>
#include <QSqlError>
#include <QSqlTableModel>
#include <QSqlRelationalTableModel>
#include <QStandardItemModel>
#include <QList>
#include <QMap>
#include <QChart>
#include <QLineSeries>

namespace Ui {
class PootjeOver;
}

class PootjeOver : public QMainWindow
{
	Q_OBJECT

public:
	explicit PootjeOver(QWidget *parent = nullptr);
	~PootjeOver();

private slots:
	void on_addPlayer_clicked();
	void on_updatePlayer_clicked();

	void on_updateTeam_clicked();

	void on_addGame_clicked();
	void on_updateGame_clicked();

	void on_newGame_clicked();

	void on_acceptGame_clicked();

	void on_removeGame_clicked();

	void on_showRating_clicked();

	void on_actionInfo_triggered();

	void on_actionStart_new_triggered();

	void on_showLast_clicked();

private:
	Ui::PootjeOver *ui;

	QSqlDatabase m_db;
	QSqlError m_lasterror;

	QSqlTableModel *m_player_model = nullptr;
	QSqlRelationalTableModel *m_team_model = nullptr;
	QSqlRelationalTableModel *m_game_model = nullptr;

	QMap<int, QtCharts::QLineSeries *> m_rating_series;
	QtCharts::QChart m_rating_chart;

	bool openDatabase(void);
	void closeDatabase(void);
	bool createDatabaseTables(QSqlDatabase& db, bool init);
	QString lastError(void);

	void setupPlayersTab(void);
	void setupTeamsTab(void);
	void setupGamesTab(void);
	void setupGraphTab(void);

	QList<QMap<int,double>> determine_ratings(void);
	void update_rating_in_DB(void);

	QList<int> active_team;
	void addGame(const QList<int>& color, const QList<int>& white);

	QMap<int,QString> getPlayerNames(bool active_only, bool include_virtual);
	QList<int> getTeam(const QString tile, const QMap<int,QString>& players, QMap<int,QString>& left);
	int getAverageScore();
};

#endif // POOTJEOVER_H
