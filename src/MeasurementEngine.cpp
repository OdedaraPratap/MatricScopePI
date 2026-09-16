#include "MeasurementEngine.h"

#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

namespace {
const int RequiredStableFrames = 8;
const double MovementThresholdPixels = 9.0;
const double MinimumContourArea = 350.0;
}

MeasurementEngine::MeasurementEngine()
    : m_lastCentre(0, 0), m_hadCentre(false), m_stableFrames(0),
      m_pixelsPerMillimetre(10.0)
{
}

void MeasurementEngine::setPixelsPerMillimetre(double value)
{
    if (value > 0.0) m_pixelsPerMillimetre = value;
}

double MeasurementEngine::pixelsPerMillimetre() const { return m_pixelsPerMillimetre; }

void MeasurementEngine::setBackground(const cv::Mat &frame)
{
    if (frame.empty()) return;
    cv::cvtColor(frame, m_backgroundGray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(m_backgroundGray, m_backgroundGray, cv::Size(5, 5), 0);
    resetTracking();
}

bool MeasurementEngine::hasBackground() const { return !m_backgroundGray.empty(); }

bool MeasurementEngine::objectPresent(const cv::Mat &frame) const
{
    std::vector<cv::Point> contour;
    return largestContour(frame, &contour);
}

void MeasurementEngine::resetTracking()
{
    m_hadCentre = false;
    m_stableFrames = 0;
}

bool MeasurementEngine::largestContour(const cv::Mat &frame,
                                       std::vector<cv::Point> *contour,
                                       cv::Mat *mask) const
{
    if (frame.empty()) return false;
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);
    cv::Mat binary;
    if (!m_backgroundGray.empty() && m_backgroundGray.size() == gray.size()) {
        cv::absdiff(m_backgroundGray, gray, binary);
        cv::threshold(binary, binary, 25, 255, cv::THRESH_BINARY);
    } else {
        cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    }
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN,
                     cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE,
                     cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9)));
    if (mask) binary.copyTo(*mask);

    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    double bestArea = MinimumContourArea;
    int bestIndex = -1;
    for (std::size_t i = 0; i < contours.size(); ++i) {
        const double area = cv::contourArea(contours[i]);
        if (area > bestArea) { bestArea = area; bestIndex = static_cast<int>(i); }
    }
    if (bestIndex < 0) return false;
    *contour = contours[static_cast<std::size_t>(bestIndex)];
    return true;
}

bool MeasurementEngine::objectIsStable(const cv::Mat &frame, cv::Mat *previewMask)
{
    std::vector<cv::Point> contour;
    if (!largestContour(frame, &contour, previewMask)) {
        resetTracking();
        return false;
    }
    const cv::Moments moments = cv::moments(contour);
    if (std::abs(moments.m00) < 0.001) return false;
    const cv::Point2f centre(static_cast<float>(moments.m10 / moments.m00),
                             static_cast<float>(moments.m01 / moments.m00));
    if (!m_hadCentre || cv::norm(centre - m_lastCentre) > MovementThresholdPixels)
        m_stableFrames = 0;
    else
        ++m_stableFrames;
    m_lastCentre = centre;
    m_hadCentre = true;
    return m_stableFrames >= RequiredStableFrames;
}

MeasurementResult MeasurementEngine::measure(const cv::Mat &frame, const QString &shape) const
{
    MeasurementResult result = {false, 0, 0, 0, cv::Point2f(), QStringLiteral("No object detected")};
    std::vector<cv::Point> contour;
    if (!largestContour(frame, &contour)) return result;

    const cv::RotatedRect box = cv::minAreaRect(contour);
    const double longPixels = std::max(box.size.width, box.size.height);
    const double shortPixels = std::min(box.size.width, box.size.height);
    result.length = longPixels / m_pixelsPerMillimetre;
    result.width = shortPixels / m_pixelsPerMillimetre;
    if (shape.compare(QStringLiteral("Round"), Qt::CaseInsensitive) == 0) {
        const double areaDiameter = 2.0 * std::sqrt(cv::contourArea(contour) / CV_PI);
        result.length = areaDiameter / m_pixelsPerMillimetre;
        result.width = result.length;
    }
    result.ratio = result.width > 0.0 ? result.length / result.width : 0.0;
    result.centre = box.center;
    result.valid = true;
    result.message = QStringLiteral("L %1 mm   W %2 mm   R %3")
                         .arg(result.length, 0, 'f', 2)
                         .arg(result.width, 0, 'f', 2)
                         .arg(result.ratio, 0, 'f', 2);
    return result;
}
