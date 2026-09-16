#include "HistoryStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>

HistoryStore::HistoryStore()
    : m_connectionName(QStringLiteral("history-%1").arg(QUuid::createUuid().toString())),
      m_database(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName))
{
    m_database.setDatabaseName(QDir(QCoreApplication::applicationDirPath()).filePath("History.db"));
}

HistoryStore::~HistoryStore()
{
    if (m_database.isOpen()) m_database.close();
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool HistoryStore::open(QString *error)
{
    if (!m_database.open()) {
        if (error) *error = m_database.lastError().text();
        return false;
    }
    QSqlQuery query(m_database);
    const bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS Records ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT, Date TEXT NOT NULL,"
        "Shape TEXT NOT NULL, Image TEXT, Length REAL NOT NULL, Width REAL NOT NULL,"
        "Profile TEXT DEFAULT '', Bin INTEGER DEFAULT 0, Ratio REAL DEFAULT 0)"));
    const QStringList migrations = QStringList()
        << "ALTER TABLE Records ADD COLUMN Profile TEXT DEFAULT ''"
        << "ALTER TABLE Records ADD COLUMN Bin INTEGER DEFAULT 0"
        << "ALTER TABLE Records ADD COLUMN Ratio REAL DEFAULT 0";
    for (int i = 0; i < migrations.size(); ++i) query.exec(migrations.at(i));
    if (!ok && error) *error = query.lastError().text();
    return ok;
}

bool HistoryStore::append(const HistoryRecord &record, QString *error)
{
    if (!m_database.isOpen() && !open(error)) return false;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO Records(Date,Profile,Shape,Bin,Length,Width,Ratio,Image) "
        "VALUES(?,?,?,?,?,?,?,?)"));
    query.addBindValue(record.measuredAt.toString(Qt::ISODate));
    query.addBindValue(record.profile);
    query.addBindValue(record.shape);
    query.addBindValue(record.bin);
    query.addBindValue(record.measurement.length);
    query.addBindValue(record.measurement.width);
    query.addBindValue(record.measurement.ratio);
    query.addBindValue(record.imagePath);
    const bool ok = query.exec();
    if (!ok && error) *error = query.lastError().text();
    return ok;
}

QSqlDatabase HistoryStore::database() const { return m_database; }
