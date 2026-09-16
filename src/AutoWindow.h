#pragma once

#include "MeasurementEngine.h"
#include "RuleStore.h"

#include <QMainWindow>
#include <QTimer>
#include <opencv2/videoio.hpp>

class QLabel;
class QPushButton;

class AutoWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AutoWindow(QWidget *parent = 0);
    ~AutoWindow();

private slots:
    void readFrame();
    void selectMode();
    void captureBackground();
    void openSettings();
    void stopMeasurement();
    void chooseCalibration();

private:
    void buildUi();
    void openCamera();
    void showFrame(const cv::Mat &frame);
    void performMeasurement(const cv::Mat &frame);
    int matchingBin(const MeasurementResult &result) const;

    QLabel *m_preview;
    QLabel *m_status;
    QLabel *m_modeLabel;
    QLabel *m_profileLabel;
    QTimer m_timer;
    cv::VideoCapture m_camera;
    cv::Mat m_lastFrame;
    MeasurementEngine m_engine;
    RuleStore m_rules;
    QString m_mode;
    QString m_profile;
    bool m_measurementArmed;
};
