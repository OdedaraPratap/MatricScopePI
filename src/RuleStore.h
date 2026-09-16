#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct MeasurementRule
{
    QString profile;
    int number;
    QString shape;
    double fromLength;
    double toLength;
    double fromWidth;
    double toWidth;
};

class RuleStore
{
public:
    explicit RuleStore(const QString &path = QString());

    bool load(QString *error = 0);
    bool save(QString *error = 0) const;
    const QVector<MeasurementRule> &rules() const;
    QStringList profiles() const;
    QVector<MeasurementRule> matching(const QString &profile,
                                      const QString &shape) const;
    bool upsert(const MeasurementRule &rule, QString *error = 0);
    bool remove(const QString &profile, const QString &shape, int number);
    QString path() const;

private:
    QString m_path;
    QVector<MeasurementRule> m_rules;
};
