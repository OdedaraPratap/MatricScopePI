#pragma once

#include "HistoryStore.h"
#include "RuleStore.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QSerialPort;
class QSqlQueryModel;
class QTableView;

class CameraSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CameraSettingsDialog(QWidget *parent = 0);
signals:
    void settingsChanged(double exposure, double gain, double gamma);
private slots:
    void apply();
private:
    QDoubleSpinBox *m_exposure;
    QDoubleSpinBox *m_gain;
    QDoubleSpinBox *m_gamma;
};

class VariationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VariationDialog(QWidget *parent = 0);
private slots:
    void save();
    void reset();
private:
    QDoubleSpinBox *m_length;
    QDoubleSpinBox *m_width;
};

class HistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HistoryDialog(HistoryStore *store, QWidget *parent = 0);
private slots:
    void filter();
    void removeSelected();
private:
    HistoryStore *m_store;
    QLineEdit *m_search;
    QComboBox *m_shape;
    QTableView *m_table;
    QSqlQueryModel *m_model;
};

class PrintDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PrintDialog(RuleStore *rules, QWidget *parent = 0);
private slots:
    void printRules();
private:
    RuleStore *m_rules;
    QComboBox *m_profile;
    QComboBox *m_printer;
};

class PasswordDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PasswordDialog(QWidget *parent = 0);
    bool authenticated() const;
private slots:
    void submit();
    void changePassword();
private:
    static QByteArray passwordHash();
    QLineEdit *m_password;
    bool m_authenticated;
};
