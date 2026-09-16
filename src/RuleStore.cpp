#include "RuleStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QSet>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

RuleStore::RuleStore(const QString &path)
    : m_path(path.isEmpty()
                 ? QDir(QCoreApplication::applicationDirPath()).filePath("DiamondRules.xml")
                 : path)
{
}

bool RuleStore::load(QString *error)
{
    m_rules.clear();
    QFile file(m_path);
    if (!file.exists())
        return true;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = file.errorString();
        return false;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != QStringLiteral("Rule"))
            continue;

        MeasurementRule rule = {QString(), 0, QString(), 0, 0, 0, 0};
        while (!(xml.isEndElement() && xml.name() == QStringLiteral("Rule")) && !xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement()) continue;
            const QString name = xml.name().toString();
            const QString value = xml.readElementText();
            if (name == QStringLiteral("FileName")) rule.profile = value;
            else if (name == QStringLiteral("Number")) rule.number = value.toInt();
            else if (name == QStringLiteral("ShapeType")) rule.shape = value;
            else if (name == QStringLiteral("FromLength")) rule.fromLength = value.toDouble();
            else if (name == QStringLiteral("ToLength")) rule.toLength = value.toDouble();
            else if (name == QStringLiteral("FromWidth")) rule.fromWidth = value.toDouble();
            else if (name == QStringLiteral("ToWidth")) rule.toWidth = value.toDouble();
        }
        if (!rule.profile.isEmpty() && !rule.shape.isEmpty() && rule.number > 0)
            m_rules.append(rule);
    }

    if (xml.hasError()) {
        if (error) *error = xml.errorString();
        return false;
    }
    return true;
}

bool RuleStore::save(QString *error) const
{
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = file.errorString();
        return false;
    }
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("NewDataSet"));
    for (int i = 0; i < m_rules.size(); ++i) {
        const MeasurementRule &rule = m_rules.at(i);
        xml.writeStartElement(QStringLiteral("Rule"));
        xml.writeTextElement(QStringLiteral("FileName"), rule.profile);
        xml.writeTextElement(QStringLiteral("Number"), QString::number(rule.number));
        xml.writeTextElement(QStringLiteral("ShapeType"), rule.shape);
        xml.writeTextElement(QStringLiteral("FromLength"), QString::number(rule.fromLength, 'f', 3));
        xml.writeTextElement(QStringLiteral("ToLength"), QString::number(rule.toLength, 'f', 3));
        xml.writeTextElement(QStringLiteral("FromWidth"), QString::number(rule.fromWidth, 'f', 3));
        xml.writeTextElement(QStringLiteral("ToWidth"), QString::number(rule.toWidth, 'f', 3));
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndDocument();
    if (!file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

const QVector<MeasurementRule> &RuleStore::rules() const { return m_rules; }

QStringList RuleStore::profiles() const
{
    QSet<QString> names;
    names.insert(QStringLiteral("Default_Profile"));
    for (int i = 0; i < m_rules.size(); ++i) names.insert(m_rules.at(i).profile);
    QStringList result = names.values();
    result.sort(Qt::CaseInsensitive);
    return result;
}

QVector<MeasurementRule> RuleStore::matching(const QString &profile, const QString &shape) const
{
    QVector<MeasurementRule> result;
    for (int i = 0; i < m_rules.size(); ++i) {
        const MeasurementRule &rule = m_rules.at(i);
        if (rule.profile == profile && rule.shape.compare(shape, Qt::CaseInsensitive) == 0)
            result.append(rule);
    }
    return result;
}

bool RuleStore::upsert(const MeasurementRule &rule, QString *error)
{
    if (rule.profile.trimmed().isEmpty() || rule.shape.trimmed().isEmpty() || rule.number < 1) {
        if (error) *error = QStringLiteral("Profile, shape, and a positive square number are required.");
        return false;
    }
    for (int i = 0; i < m_rules.size(); ++i) {
        MeasurementRule &current = m_rules[i];
        if (current.profile == rule.profile && current.shape == rule.shape && current.number == rule.number) {
            current = rule;
            return save(error);
        }
    }
    m_rules.append(rule);
    return save(error);
}

bool RuleStore::remove(const QString &profile, const QString &shape, int number)
{
    for (int i = 0; i < m_rules.size(); ++i) {
        const MeasurementRule &rule = m_rules.at(i);
        if (rule.profile == profile && rule.shape == shape && rule.number == number) {
            m_rules.remove(i);
            return save();
        }
    }
    return false;
}

QString RuleStore::path() const { return m_path; }
