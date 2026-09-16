#pragma once

#include "MeasurementEngine.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>

struct HistoryRecord
{
    QDateTime measuredAt;
    QString profile;
    QString shape;
    int bin;
    MeasurementResult measurement;
    QString imagePath;
};

class HistoryStore
{
public:
    HistoryStore();
    ~HistoryStore();

    bool open(QString *error = 0);
    bool append(const HistoryRecord &record, QString *error = 0);
    QSqlDatabase database() const;

private:
    QString m_connectionName;
    QSqlDatabase m_database;
};
