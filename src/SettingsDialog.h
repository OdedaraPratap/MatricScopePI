#pragma once

#include "RuleStore.h"

#include <QDialog>

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QTableWidget;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(RuleStore *store, QWidget *parent = 0);
    QString selectedProfile() const;

signals:
    void profileChanged(const QString &profile);

private slots:
    void refreshTable();
    void loadSelectedRow();
    void saveRule();
    void deleteRule();
    void setRoundFields();

private:
    MeasurementRule formRule() const;
    void populateProfiles(const QString &selected = QString());

    RuleStore *m_store;
    QComboBox *m_profile;
    QComboBox *m_shape;
    QSpinBox *m_number;
    QDoubleSpinBox *m_fromLength;
    QDoubleSpinBox *m_toLength;
    QDoubleSpinBox *m_fromWidth;
    QDoubleSpinBox *m_toWidth;
    QTableWidget *m_table;
};
