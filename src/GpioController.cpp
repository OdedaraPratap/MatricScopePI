#include "GpioController.h"

#include <QSettings>
#include <QThread>

#ifdef MATRIC_HAS_GPIOD
#include <gpiod.h>
#endif

GpioController::GpioController()
#ifdef MATRIC_HAS_GPIOD
    : m_chip(0), m_strobeLine(0), m_open(false)
#else
    : m_open(false)
#endif
{
}

GpioController::~GpioController() { close(); }

bool GpioController::open(QString *error)
{
    close();
#ifdef MATRIC_HAS_GPIOD
    QSettings settings;
    const QString chipName = settings.value("gpio/chip", QStringLiteral("gpiochip0")).toString();
    const int defaultPins[] = {17, 27, 22, 23, 25};
    m_chip = gpiod_chip_open_by_name(chipName.toLocal8Bit().constData());
    if (!m_chip) {
        if (error) *error = QStringLiteral("Cannot open %1. Check GPIO permissions.").arg(chipName);
        return false;
    }
    for (int bit = 0; bit < 5; ++bit) {
        const unsigned int pin = settings.value(QStringLiteral("gpio/data%1").arg(bit), defaultPins[bit]).toUInt();
        gpiod_line *line = gpiod_chip_get_line(m_chip, pin);
        if (!line || gpiod_line_request_output(line, "MatricScopePI", 0) < 0) {
            if (error) *error = QStringLiteral("Cannot reserve GPIO %1").arg(pin);
            close();
            return false;
        }
        m_dataLines.push_back(line);
    }
    const unsigned int strobePin = settings.value("gpio/strobe", 24).toUInt();
    m_strobeLine = gpiod_chip_get_line(m_chip, strobePin);
    if (!m_strobeLine || gpiod_line_request_output(m_strobeLine, "MatricScopePI", 0) < 0) {
        if (error) *error = QStringLiteral("Cannot reserve strobe GPIO %1").arg(strobePin);
        close();
        return false;
    }
    m_open = true;
    return true;
#else
    if (error) *error = QStringLiteral("This build does not contain libgpiod support");
    return false;
#endif
}

void GpioController::close()
{
#ifdef MATRIC_HAS_GPIOD
    if (m_strobeLine) { gpiod_line_set_value(m_strobeLine, 0); gpiod_line_release(m_strobeLine); }
    m_strobeLine = 0;
    for (std::size_t i = 0; i < m_dataLines.size(); ++i) {
        gpiod_line_set_value(m_dataLines[i], 0);
        gpiod_line_release(m_dataLines[i]);
    }
    m_dataLines.clear();
    if (m_chip) gpiod_chip_close(m_chip);
    m_chip = 0;
#endif
    m_open = false;
}

bool GpioController::isOpen() const { return m_open; }

bool GpioController::writeBin(int bin, QString *error)
{
    if (!m_open || bin < 0 || bin > 31) {
        if (error) *error = QStringLiteral("GPIO is unavailable or bin is outside 0–31");
        return false;
    }
#ifdef MATRIC_HAS_GPIOD
    for (int bit = 0; bit < 5; ++bit) {
        if (gpiod_line_set_value(m_dataLines[static_cast<std::size_t>(bit)], (bin >> bit) & 1) < 0) {
            if (error) *error = QStringLiteral("Failed to write GPIO data bit %1").arg(bit);
            return false;
        }
    }
    gpiod_line_set_value(m_strobeLine, 1);
    QThread::msleep(25);
    gpiod_line_set_value(m_strobeLine, 0);
    return true;
#else
    Q_UNUSED(error)
    return false;
#endif
}
