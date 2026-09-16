#pragma once

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <QString>

class MvCameraBackend
{
public:
    MvCameraBackend();
    ~MvCameraBackend();

    bool open(QString *error = 0);
    void close();
    bool isOpen() const;
    bool read(cv::Mat *frame, QString *error = 0);
    bool setExposure(double milliseconds);
    bool setGain(double gain);
    bool setGamma(double gamma);
    QString backendName() const;

private:
#ifdef MATRIC_HAS_MVCAMERA
    bool convertFrame(const unsigned char *data, unsigned int dataLength,
                      unsigned int width, unsigned int height,
                      unsigned int pixelType, cv::Mat *frame, QString *error);
    void *m_handle;
    bool m_grabbing;
#else
    cv::VideoCapture m_capture;
#endif
};
