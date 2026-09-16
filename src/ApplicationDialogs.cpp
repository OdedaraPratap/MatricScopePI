#include "ApplicationDialogs.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QTableView>
#include <QVBoxLayout>

namespace {
void styleDialog(QDialog *dialog)
{
    dialog->setStyleSheet(QStringLiteral(
        "QDialog{background:#111;color:white} QLabel{color:white}"
        "QLineEdit,QComboBox,QDoubleSpinBox{background:#222;color:white;min-height:34px;padding:3px}"
        "QPushButton{background:#ff8000;color:white;font-weight:bold;min-height:42px;min-width:90px}"));
}
}

CameraSettingsDialog::CameraSettingsDialog(QWidget *parent)
    : QDialog(parent), m_exposure(new QDoubleSpinBox(this)),
      m_gain(new QDoubleSpinBox(this)), m_gamma(new QDoubleSpinBox(this))
{
    setWindowTitle(tr("Camera Settings")); styleDialog(this);
    QSettings settings;
    m_exposure->setRange(0.01, 1000); m_exposure->setValue(settings.value("camera/exposure", 20).toDouble());
    m_gain->setRange(0, 100); m_gain->setValue(settings.value("camera/gain", 0).toDouble());
    m_gamma->setRange(0.1, 5); m_gamma->setValue(settings.value("camera/gamma", 1).toDouble());
    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(tr("Exposure (ms)"), m_exposure);
    layout->addRow(tr("Gain"), m_gain);
    layout->addRow(tr("Gamma"), m_gamma);
    QPushButton *button = new QPushButton(tr("APPLY"), this);
    layout->addWidget(button); connect(button, SIGNAL(clicked()), this, SLOT(apply()));
}

void CameraSettingsDialog::apply()
{
    QSettings settings;
    settings.setValue("camera/exposure", m_exposure->value());
    settings.setValue("camera/gain", m_gain->value());
    settings.setValue("camera/gamma", m_gamma->value());
    emit settingsChanged(m_exposure->value(), m_gain->value(), m_gamma->value());
    accept();
}

VariationDialog::VariationDialog(QWidget *parent)
    : QDialog(parent), m_length(new QDoubleSpinBox(this)), m_width(new QDoubleSpinBox(this))
{
    setWindowTitle(tr("Measurement Variation")); styleDialog(this);
    QSettings settings;
    m_length->setRange(-100, 100); m_width->setRange(-100, 100);
    m_length->setSuffix(" %"); m_width->setSuffix(" %");
    m_length->setValue(settings.value("variation/length", 0).toDouble());
    m_width->setValue(settings.value("variation/width", 0).toDouble());
    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(tr("Length correction"), m_length);
    layout->addRow(tr("Width correction"), m_width);
    QHBoxLayout *buttons = new QHBoxLayout;
    QPushButton *saveButton = new QPushButton(tr("SAVE"), this);
    QPushButton *resetButton = new QPushButton(tr("RESET"), this);
    buttons->addWidget(saveButton); buttons->addWidget(resetButton); layout->addRow(buttons);
    connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
    connect(resetButton, SIGNAL(clicked()), this, SLOT(reset()));
}

void VariationDialog::save()
{
    QSettings settings; settings.setValue("variation/length", m_length->value());
    settings.setValue("variation/width", m_width->value()); accept();
}
void VariationDialog::reset() { m_length->setValue(0); m_width->setValue(0); }

HistoryDialog::HistoryDialog(HistoryStore *store, QWidget *parent)
    : QDialog(parent), m_store(store), m_search(new QLineEdit(this)),
      m_shape(new QComboBox(this)), m_table(new QTableView(this)),
      m_model(new QSqlQueryModel(this))
{
    setWindowTitle(tr("Measurement History")); resize(760, 440); styleDialog(this);
    m_search->setPlaceholderText(tr("Search profile…"));
    m_shape->addItems(QStringList() << tr("All shapes") << "Round" << "Pear" << "Marquise"
                                     << "Heart" << "Oval" << "Poly" << "Emerald");
    QHBoxLayout *filters = new QHBoxLayout; filters->addWidget(m_search); filters->addWidget(m_shape);
    QPushButton *remove = new QPushButton(tr("DELETE"), this);
    QVBoxLayout *layout = new QVBoxLayout(this); layout->addLayout(filters); layout->addWidget(m_table); layout->addWidget(remove);
    m_table->setModel(m_model); m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_search, SIGNAL(textChanged(QString)), this, SLOT(filter()));
    connect(m_shape, SIGNAL(currentTextChanged(QString)), this, SLOT(filter()));
    connect(remove, SIGNAL(clicked()), this, SLOT(removeSelected())); filter();
}

void HistoryDialog::filter()
{
    QString where = QStringLiteral(" WHERE Profile LIKE :profile");
    if (m_shape->currentIndex() > 0) where += QStringLiteral(" AND Shape=:shape");
    QSqlQuery query(m_store->database());
    query.prepare(QStringLiteral("SELECT ID,Date,Profile,Shape,Bin,Length,Width,Ratio FROM Records") + where +
                  QStringLiteral(" ORDER BY ID DESC"));
    query.bindValue(":profile", QStringLiteral("%") + m_search->text() + QStringLiteral("%"));
    if (m_shape->currentIndex() > 0) query.bindValue(":shape", m_shape->currentText());
    query.exec(); m_model->setQuery(query); m_table->resizeColumnsToContents();
}

void HistoryDialog::removeSelected()
{
    if (!m_table->currentIndex().isValid()) return;
    const int id = m_model->index(m_table->currentIndex().row(), 0).data().toInt();
    QSqlQuery query(m_store->database()); query.prepare("DELETE FROM Records WHERE ID=?");
    query.addBindValue(id); query.exec(); filter();
}

PasswordDialog::PasswordDialog(QWidget *parent)
    : QDialog(parent), m_password(new QLineEdit(this)), m_authenticated(false)
{
    setWindowTitle(tr("Administrator Login")); styleDialog(this); m_password->setEchoMode(QLineEdit::Password);
    QVBoxLayout *layout = new QVBoxLayout(this); layout->addWidget(new QLabel(tr("Password"), this)); layout->addWidget(m_password);
    QPushButton *login = new QPushButton(tr("LOGIN"), this); QPushButton *change = new QPushButton(tr("CHANGE PASSWORD"), this);
    layout->addWidget(login); layout->addWidget(change); connect(login, SIGNAL(clicked()), this, SLOT(submit()));
    connect(change, SIGNAL(clicked()), this, SLOT(changePassword()));
}

QByteArray PasswordDialog::passwordHash()
{
    QSettings settings; const QByteArray fallback = QCryptographicHash::hash("admin", QCryptographicHash::Sha256).toHex();
    return settings.value("security/passwordHash", fallback).toByteArray();
}
void PasswordDialog::submit()
{
    m_authenticated = QCryptographicHash::hash(m_password->text().toUtf8(), QCryptographicHash::Sha256).toHex() == passwordHash();
    if (m_authenticated) accept(); else QMessageBox::warning(this, tr("Login"), tr("Incorrect password."));
}
void PasswordDialog::changePassword()
{
    if (QCryptographicHash::hash(m_password->text().toUtf8(), QCryptographicHash::Sha256).toHex() != passwordHash()) {
        QMessageBox::warning(this, tr("Password"), tr("Enter the current password first.")); return;
    }
    bool ok = false; const QString password = QInputDialog::getText(this, tr("New Password"), tr("New password"), QLineEdit::Password, QString(), &ok);
    if (ok && password.size() >= 4) {
        QSettings().setValue("security/passwordHash", QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
        QMessageBox::information(this, tr("Password"), tr("Password changed."));
    }
}
bool PasswordDialog::authenticated() const { return m_authenticated; }
