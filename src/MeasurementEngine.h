#pragma once

#include <opencv2/core.hpp>
#include <QString>
#include <vector>

struct MeasurementResult
{
    bool valid;
    double length;
    double width;
    double ratio;
    cv::Point2f centre;
    QString message;
};

class MeasurementEngine
{
public:
    MeasurementEngine();

    void setPixelsPerMillimetre(double value);
    double pixelsPerMillimetre() const;
    void setBackground(const cv::Mat &frame);
    bool hasBackground() const;
    void resetTracking();

    bool objectPresent(const cv::Mat &frame) const;
    bool objectIsStable(const cv::Mat &frame, cv::Mat *previewMask = 0);
    MeasurementResult measure(const cv::Mat &frame, const QString &shape) const;

private:
    bool largestContour(const cv::Mat &frame,
                        std::vector<cv::Point> *contour,
                        cv::Mat *mask = 0) const;

    cv::Mat m_backgroundGray;
    cv::Point2f m_lastCentre;
    bool m_hadCentre;
    int m_stableFrames;
    double m_pixelsPerMillimetre;
};
