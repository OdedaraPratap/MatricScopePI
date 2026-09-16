#include "SettingsDialog.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

SettingsDialog::SettingsDialog(RuleStore *store, QWidget *parent)
    : QDialog(parent), m_store(store), m_profile(new QComboBox(this)),
      m_shape(new QComboBox(this)), m_number(new QSpinBox(this)),
      m_fromLength(new QDoubleSpinBox(this)), m_toLength(new QDoubleSpinBox(this)),
      m_fromWidth(new QDoubleSpinBox(this)), m_toWidth(new QDoubleSpinBox(this)),
      m_table(new QTableWidget(this))
{
    setWindowTitle(tr("Tray Settings"));
    setModal(true);
    resize(760, 440);
    setStyleSheet(QStringLiteral(
        "QDialog { background:#111; color:white; }"
        "QLabel { color:white; } QLineEdit,QComboBox,QSpinBox,QDoubleSpinBox {"
        "background:#222; color:white; min-height:34px; padding:2px 7px; }"
        "QPushButton { background:#ff8000; color:white; font-weight:bold;"
        "min-height:42px; min-width:90px; border-radius:5px; }"
        "QTableWidget { background:#090909; color:white; gridline-color:#555; }"));

    m_profile->setEditable(true);
    m_shape->addItems(QStringList() << "Round" << "Pear" << "Marquise" << "Heart"
                                     << "Oval" << "General" << "Poly" << "Emerald");
    m_number->setRange(1, 20);
    QDoubleSpinBox *dimensions[] = {m_fromLength, m_toLength, m_fromWidth, m_toWidth};
    for (int i = 0; i < 4; ++i) {
        dimensions[i]->setRange(0.0, 999.999);
        dimensions[i]->setDecimals(3);
        dimensions[i]->setSingleStep(0.1);
    }

    QFormLayout *form = new QFormLayout;
    form->addRow(tr("Profile"), m_profile);
    form->addRow(tr("Shape"), m_shape);
    form->addRow(tr("Square number"), m_number);
    form->addRow(tr("From length / diameter"), m_fromLength);
    form->addRow(tr("To length / diameter"), m_toLength);
    form->addRow(tr("From width"), m_fromWidth);
    form->addRow(tr("To width"), m_toWidth);

    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(QStringList() << tr("No.") << tr("From L")
                                                       << tr("To L") << tr("From W") << tr("To W"));
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    QPushButton *save = new QPushButton(tr("ADD / UPDATE"), this);
    QPushButton *remove = new QPushButton(tr("DELETE"), this);
    QPushButton *close = new QPushButton(tr("CLOSE"), this);
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(save);
    buttons->addWidget(remove);
    buttons->addStretch();
    buttons->addWidget(close);

    QGridLayout *layout = new QGridLayout(this);
    layout->addLayout(form, 0, 0);
    layout->addWidget(m_table, 0, 1);
    layout->addLayout(buttons, 1, 0, 1, 2);
    layout->setColumnStretch(1, 2);

    connect(m_profile, SIGNAL(currentTextChanged(QString)), this, SLOT(refreshTable()));
    connect(m_shape, SIGNAL(currentTextChanged(QString)), this, SLOT(refreshTable()));
    connect(m_shape, SIGNAL(currentTextChanged(QString)), this, SLOT(setRoundFields()));
    connect(m_table, SIGNAL(itemSelectionChanged()), this, SLOT(loadSelectedRow()));
    connect(save, SIGNAL(clicked()), this, SLOT(saveRule()));
    connect(remove, SIGNAL(clicked()), this, SLOT(deleteRule()));
    connect(close, SIGNAL(clicked()), this, SLOT(accept()));

    populateProfiles();
    setRoundFields();
    refreshTable();
}

QString SettingsDialog::selectedProfile() const { return m_profile->currentText().trimmed(); }

void SettingsDialog::populateProfiles(const QString &selected)
{
    const QString keep = selected.isEmpty() ? m_profile->currentText() : selected;
    m_profile->blockSignals(true);
    m_profile->clear();
    m_profile->addItems(m_store->profiles());
    const int index = m_profile->findText(keep);
    m_profile->setCurrentIndex(index >= 0 ? index : 0);
    if (index < 0 && !keep.isEmpty()) m_profile->setEditText(keep);
    m_profile->blockSignals(false);
}

void SettingsDialog::refreshTable()
{
    const QVector<MeasurementRule> rules =
        m_store->matching(m_profile->currentText().trimmed(), m_shape->currentText());
    m_table->setRowCount(rules.size());
    for (int row = 0; row < rules.size(); ++row) {
        const MeasurementRule &rule = rules.at(row);
        const double values[] = {static_cast<double>(rule.number), rule.fromLength,
                                 rule.toLength, rule.fromWidth, rule.toWidth};
        for (int column = 0; column < 5; ++column)
            m_table->setItem(row, column, new QTableWidgetItem(
                column == 0 ? QString::number(rule.number) : QString::number(values[column], 'f', 3)));
    }
}

void SettingsDialog::loadSelectedRow()
{
    const int row = m_table->currentRow();
    if (row < 0) return;
    m_number->setValue(m_table->item(row, 0)->text().toInt());
    m_fromLength->setValue(m_table->item(row, 1)->text().toDouble());
    m_toLength->setValue(m_table->item(row, 2)->text().toDouble());
    m_fromWidth->setValue(m_table->item(row, 3)->text().toDouble());
    m_toWidth->setValue(m_table->item(row, 4)->text().toDouble());
}

MeasurementRule SettingsDialog::formRule() const
{
    MeasurementRule rule;
    rule.profile = m_profile->currentText().trimmed();
    rule.number = m_number->value();
    rule.shape = m_shape->currentText();
    rule.fromLength = m_fromLength->value();
    rule.toLength = m_toLength->value();
    rule.fromWidth = m_fromWidth->value();
    rule.toWidth = m_toWidth->value();
    return rule;
}

void SettingsDialog::saveRule()
{
    const MeasurementRule rule = formRule();
    if (rule.fromLength > rule.toLength || rule.fromWidth > rule.toWidth) {
        QMessageBox::warning(this, tr("Invalid rule"), tr("A From value cannot exceed its To value."));
        return;
    }
    QString error;
    if (!m_store->upsert(rule, &error)) {
        QMessageBox::critical(this, tr("Save failed"), error);
        return;
    }
    populateProfiles(rule.profile);
    refreshTable();
    emit profileChanged(rule.profile);
}

void SettingsDialog::deleteRule()
{
    if (m_store->remove(m_profile->currentText().trimmed(), m_shape->currentText(), m_number->value()))
        refreshTable();
}

void SettingsDialog::setRoundFields()
{
    const bool round = m_shape->currentText() == QStringLiteral("Round");
    m_fromWidth->setEnabled(!round);
    m_toWidth->setEnabled(!round);
    if (round) { m_fromWidth->setValue(0); m_toWidth->setValue(0); }
}
