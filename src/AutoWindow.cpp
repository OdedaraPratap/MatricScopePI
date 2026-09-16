#include "AutoWindow.h"
#include "ApplicationDialogs.h"
#include "SettingsDialog.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QPixmap>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSettings>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <opencv2/imgproc.hpp>

AutoWindow::AutoWindow(QWidget *parent)
    : QMainWindow(parent), m_preview(0), m_status(0), m_modeLabel(0),
      m_profileLabel(0), m_serial(new QSerialPort(this)), m_mode(QStringLiteral("Round")),
      m_profile(QStringLiteral("Default_Profile")), m_measurementArmed(true)
{
    buildUi();
    QString error;
    if (!m_rules.load(&error)) m_status->setText(tr("Rules warning: %1").arg(error));
    m_history.open();
    const QString serialName = QSettings().value("serial/port", QStringLiteral("/dev/ttyUSB0")).toString();
    m_serial->setPortName(serialName);
    m_serial->setBaudRate(QSerialPort::Baud9600);
    m_serial->open(QIODevice::WriteOnly);
    openCamera();
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(readFrame()));
    m_timer.start(50);
}

AutoWindow::~AutoWindow() { m_camera.release(); }

void AutoWindow::buildUi()
{
    setWindowTitle(tr("Matric Scope"));
    setStyleSheet(QStringLiteral(
        "QMainWindow,QWidget { background:#050505; color:white; }"
        "QPushButton { background:#ff8000; color:white; font-weight:bold;"
        "font-size:14px; min-height:48px; border-radius:7px; padding:4px; }"
        "QPushButton:checked { background:#b44b00; border:3px solid white; }"
        "QLabel#status { color:#ff8000; font-size:20px; font-weight:bold; }"));
    QWidget *central = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(central);
    root->setContentsMargins(8, 6, 8, 8);
    root->setSpacing(6);

    QHBoxLayout *header = new QHBoxLayout;
    QLabel *title = new QLabel(tr("MATRIC SCOPE"), central);
    title->setStyleSheet(QStringLiteral("font-size:20px;font-weight:bold;"));
    m_modeLabel = new QLabel(tr("Mode: Round"), central);
    m_profileLabel = new QLabel(tr("Profile: Default_Profile"), central);
    QPushButton *minimise = new QPushButton(QStringLiteral("—"), central);
    QPushButton *close = new QPushButton(QStringLiteral("×"), central);
    minimise->setFixedSize(52, 42); close->setFixedSize(52, 42);
    header->addWidget(title); header->addStretch(); header->addWidget(m_modeLabel);
    header->addWidget(m_profileLabel); header->addWidget(minimise); header->addWidget(close);
    connect(minimise, SIGNAL(clicked()), this, SLOT(showMinimized()));
    connect(close, SIGNAL(clicked()), qApp, SLOT(quit()));

    m_preview = new QLabel(central);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(400, 240);
    m_preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_preview->setStyleSheet(QStringLiteral("background:black;border:2px solid #ff8000;"));

    QWidget *buttonPanel = new QWidget(central);
    QGridLayout *buttons = new QGridLayout(buttonPanel);
    const QStringList modes = QStringList() << "Round" << "Pear" << "Marquise" << "Heart"
                                            << "Oval" << "Poly" << "Emerald";
    for (int i = 0; i < modes.size(); ++i) {
        QPushButton *button = new QPushButton(modes.at(i), buttonPanel);
        button->setCheckable(true);
        button->setProperty("mode", modes.at(i));
        buttons->addWidget(button, i / 2, i % 2);
        connect(button, SIGNAL(clicked()), this, SLOT(selectMode()));
        if (i == 0) button->setChecked(true);
    }
    QPushButton *background = new QPushButton(tr("BACKGROUND"), buttonPanel);
    QPushButton *calibrate = new QPushButton(tr("CALIBRATE"), buttonPanel);
    QPushButton *settings = new QPushButton(tr("SETTINGS"), buttonPanel);
    QPushButton *stop = new QPushButton(tr("STOP"), buttonPanel);
    QPushButton *camera = new QPushButton(tr("CAMERA"), buttonPanel);
    QPushButton *history = new QPushButton(tr("HISTORY"), buttonPanel);
    QPushButton *print = new QPushButton(tr("PRINT"), buttonPanel);
    QPushButton *variation = new QPushButton(tr("VARIATION"), buttonPanel);
    buttons->addWidget(background, 4, 0); buttons->addWidget(calibrate, 4, 1);
    buttons->addWidget(settings, 5, 0); buttons->addWidget(stop, 5, 1);
    buttons->addWidget(camera, 6, 0); buttons->addWidget(history, 6, 1);
    buttons->addWidget(print, 7, 0); buttons->addWidget(variation, 7, 1);
    connect(background, SIGNAL(clicked()), this, SLOT(captureBackground()));
    connect(calibrate, SIGNAL(clicked()), this, SLOT(chooseCalibration()));
    connect(settings, SIGNAL(clicked()), this, SLOT(openSettings()));
    connect(stop, SIGNAL(clicked()), this, SLOT(stopMeasurement()));
    connect(camera, SIGNAL(clicked()), this, SLOT(openCameraSettings()));
    connect(history, SIGNAL(clicked()), this, SLOT(openHistory()));
    connect(print, SIGNAL(clicked()), this, SLOT(openPrint()));
    connect(variation, SIGNAL(clicked()), this, SLOT(openVariation()));

    QHBoxLayout *body = new QHBoxLayout;
    body->addWidget(m_preview, 4);
    body->addWidget(buttonPanel, 2);
    m_status = new QLabel(tr("Starting camera…"), central);
    m_status->setObjectName(QStringLiteral("status"));
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setMinimumHeight(42);

    root->addLayout(header);
    root->addLayout(body, 1);
    root->addWidget(m_status);
    setCentralWidget(central);
}

void AutoWindow::openCamera()
{
    if (!m_camera.open(0, cv::CAP_ANY)) {
        m_status->setText(tr("Camera 0 is not available"));
        return;
    }
    m_camera.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    m_camera.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    m_camera.set(cv::CAP_PROP_BUFFERSIZE, 1);
    m_status->setText(tr("Camera ready — capture an empty background"));
}

void AutoWindow::readFrame()
{
    if (!m_camera.isOpened()) return;
    cv::Mat frame;
    if (!m_camera.read(frame) || frame.empty()) return;
    frame.copyTo(m_lastFrame);
    showFrame(frame);
    if (!m_measurementArmed) {
        if (!m_engine.objectPresent(frame)) {
            m_measurementArmed = true;
            m_engine.resetTracking();
            m_status->setText(tr("Ready for the next object"));
        }
        return;
    }
    if (m_engine.objectIsStable(frame)) performMeasurement(frame);
}

void AutoWindow::showFrame(const cv::Mat &frame)
{
    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    const QImage image(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
    m_preview->setPixmap(QPixmap::fromImage(image.copy()).scaled(
        m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void AutoWindow::selectMode()
{
    QPushButton *selected = qobject_cast<QPushButton *>(sender());
    if (!selected) return;
    const QList<QPushButton *> buttons = selected->parentWidget()->findChildren<QPushButton *>();
    for (int i = 0; i < buttons.size(); ++i)
        if (buttons.at(i)->property("mode").isValid()) buttons.at(i)->setChecked(buttons.at(i) == selected);
    m_mode = selected->property("mode").toString();
    m_modeLabel->setText(tr("Mode: %1").arg(m_mode));
    m_measurementArmed = true;
    m_engine.resetTracking();
}

void AutoWindow::captureBackground()
{
    if (m_lastFrame.empty()) return;
    m_engine.setBackground(m_lastFrame);
    m_measurementArmed = true;
    m_status->setText(tr("Background captured — place an object"));
}

void AutoWindow::performMeasurement(const cv::Mat &frame)
{
    MeasurementResult result = m_engine.measure(frame, m_mode);
    if (!result.valid) return;
    QSettings settings;
    result.length *= 1.0 + settings.value("variation/length", 0).toDouble() / 100.0;
    result.width *= 1.0 + settings.value("variation/width", 0).toDouble() / 100.0;
    result.ratio = result.width > 0 ? result.length / result.width : 0;
    result.message = tr("L %1 mm   W %2 mm   R %3").arg(result.length, 0, 'f', 2)
        .arg(result.width, 0, 'f', 2).arg(result.ratio, 0, 'f', 2);
    const int bin = matchingBin(result);
    m_status->setText(bin > 0 ? tr("%1   → BIN %2").arg(result.message).arg(bin) :
                                tr("%1   → NO MATCH").arg(result.message));
    m_measurementArmed = false;
    HistoryRecord record;
    record.measuredAt = QDateTime::currentDateTime(); record.profile = m_profile;
    record.shape = m_mode; record.bin = bin; record.measurement = result;
    m_history.append(record);
    if (bin > 0 && m_serial->isOpen()) m_serial->write(QString::number(bin).toLatin1() + '\n');
}

int AutoWindow::matchingBin(const MeasurementResult &result) const
{
    const QVector<MeasurementRule> rules = m_rules.matching(m_profile, m_mode);
    for (int i = 0; i < rules.size(); ++i) {
        const MeasurementRule &rule = rules.at(i);
        const bool lengthMatches = result.length >= rule.fromLength && result.length <= rule.toLength;
        const bool widthMatches = m_mode == QStringLiteral("Round") ||
            (result.width >= rule.fromWidth && result.width <= rule.toWidth);
        if (lengthMatches && widthMatches) return rule.number;
    }
    return 0;
}

void AutoWindow::openSettings()
{
    SettingsDialog dialog(&m_rules, this);
    connect(&dialog, &SettingsDialog::profileChanged, [this](const QString &profile) {
        m_profile = profile;
        m_profileLabel->setText(tr("Profile: %1").arg(profile));
    });
    dialog.exec();
    if (!dialog.selectedProfile().isEmpty()) {
        m_profile = dialog.selectedProfile();
        m_profileLabel->setText(tr("Profile: %1").arg(m_profile));
    }
    m_measurementArmed = true;
}

void AutoWindow::stopMeasurement()
{
    m_measurementArmed = !m_measurementArmed;
    m_engine.resetTracking();
    m_status->setText(m_measurementArmed ? tr("Measurement resumed") : tr("Measurement stopped"));
}

void AutoWindow::chooseCalibration()
{
    bool accepted = false;
    const double value = QInputDialog::getDouble(this, tr("Calibration"),
        tr("Pixels per millimetre"), m_engine.pixelsPerMillimetre(), 0.001, 10000, 3, &accepted);
    if (accepted) {
        m_engine.setPixelsPerMillimetre(value);
        m_status->setText(tr("Calibration set to %1 px/mm").arg(value, 0, 'f', 3));
    }
}

void AutoWindow::openCameraSettings()
{
    CameraSettingsDialog dialog(this);
    connect(&dialog, &CameraSettingsDialog::settingsChanged,
            [this](double exposure, double gain, double gamma) {
        m_camera.set(cv::CAP_PROP_EXPOSURE, exposure);
        m_camera.set(cv::CAP_PROP_GAIN, gain);
        m_camera.set(cv::CAP_PROP_GAMMA, gamma);
    });
    dialog.exec();
}

void AutoWindow::openHistory()
{
    HistoryDialog dialog(&m_history, this); dialog.exec();
}

void AutoWindow::openPrint()
{
    PrintDialog dialog(&m_rules, this); dialog.exec();
}

void AutoWindow::openVariation()
{
    PasswordDialog password(this);
    if (password.exec() != QDialog::Accepted || !password.authenticated()) return;
    VariationDialog dialog(this); dialog.exec();
}
