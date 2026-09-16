#pragma once

#include <QString>
#include <vector>

#ifdef MATRIC_HAS_GPIOD
struct gpiod_chip;
struct gpiod_line;
#endif

class GpioController
{
public:
    GpioController();
    ~GpioController();

    bool open(QString *error = 0);
    void close();
    bool isOpen() const;
    bool writeBin(int bin, QString *error = 0);

private:
#ifdef MATRIC_HAS_GPIOD
    gpiod_chip *m_chip;
    std::vector<gpiod_line *> m_dataLines;
    gpiod_line *m_strobeLine;
#endif
    bool m_open;
};
