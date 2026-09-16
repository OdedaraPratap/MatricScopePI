#include "MvCameraBackend.h"

#include <opencv2/imgproc.hpp>

#ifdef MATRIC_HAS_MVCAMERA
#include <MvCameraControl.h>
#include <cstring>
#include <vector>
#endif

MvCameraBackend::MvCameraBackend()
#ifdef MATRIC_HAS_MVCAMERA
    : m_handle(0), m_grabbing(false)
#endif
{
}

MvCameraBackend::~MvCameraBackend() { close(); }

bool MvCameraBackend::open(QString *error)
{
    close();
#ifdef MATRIC_HAS_MVCAMERA
    MV_CC_DEVICE_INFO_LIST devices;
    std::memset(&devices, 0, sizeof(devices));
    int result = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &devices);
    if (result != MV_OK || devices.nDeviceNum == 0) {
        if (error) *error = QStringLiteral("No Hikrobot MVS camera found (0x%1)").arg(result, 0, 16);
        return false;
    }
    result = MV_CC_CreateHandle(&m_handle, devices.pDeviceInfo[0]);
    if (result != MV_OK) {
        if (error) *error = QStringLiteral("MV_CC_CreateHandle failed: 0x%1").arg(result, 0, 16);
        m_handle = 0;
        return false;
    }
    result = MV_CC_OpenDevice(m_handle, MV_ACCESS_Exclusive, 0);
    if (result != MV_OK) {
        if (error) *error = QStringLiteral("MV_CC_OpenDevice failed: 0x%1").arg(result, 0, 16);
        close();
        return false;
    }
    MV_CC_SetEnumValue(m_handle, "TriggerMode", MV_TRIGGER_MODE_OFF);
    result = MV_CC_StartGrabbing(m_handle);
    if (result != MV_OK) {
        if (error) *error = QStringLiteral("MV_CC_StartGrabbing failed: 0x%1").arg(result, 0, 16);
        close();
        return false;
    }
    m_grabbing = true;
    return true;
#else
    if (!m_capture.open(0, cv::CAP_V4L2) && !m_capture.open(0, cv::CAP_ANY)) {
        if (error) *error = QStringLiteral("Neither MvCameraControl nor V4L2 camera 0 is available");
        return false;
    }
    m_capture.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    m_capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
    return true;
#endif
}

void MvCameraBackend::close()
{
#ifdef MATRIC_HAS_MVCAMERA
    if (!m_handle) return;
    if (m_grabbing) MV_CC_StopGrabbing(m_handle);
    MV_CC_CloseDevice(m_handle);
    MV_CC_DestroyHandle(m_handle);
    m_handle = 0;
    m_grabbing = false;
#else
    m_capture.release();
#endif
}

bool MvCameraBackend::isOpen() const
{
#ifdef MATRIC_HAS_MVCAMERA
    return m_handle && m_grabbing;
#else
    return m_capture.isOpened();
#endif
}

bool MvCameraBackend::read(cv::Mat *frame, QString *error)
{
    if (!frame || !isOpen()) return false;
#ifdef MATRIC_HAS_MVCAMERA
    MV_FRAME_OUT output;
    std::memset(&output, 0, sizeof(output));
    const int result = MV_CC_GetImageBuffer(m_handle, &output, 100);
    if (result != MV_OK) {
        if (error && result != MV_E_NODATA)
            *error = QStringLiteral("MV_CC_GetImageBuffer failed: 0x%1").arg(result, 0, 16);
        return false;
    }
    const bool converted = convertFrame(output.pBufAddr, output.stFrameInfo.nFrameLen,
        output.stFrameInfo.nWidth,
        output.stFrameInfo.nHeight, output.stFrameInfo.enPixelType, frame, error);
    MV_CC_FreeImageBuffer(m_handle, &output);
    return converted;
#else
    return m_capture.read(*frame) && !frame->empty();
#endif
}

bool MvCameraBackend::setExposure(double milliseconds)
{
#ifdef MATRIC_HAS_MVCAMERA
    return m_handle && MV_CC_SetFloatValue(m_handle, "ExposureTime", static_cast<float>(milliseconds * 1000.0)) == MV_OK;
#else
    return m_capture.set(cv::CAP_PROP_EXPOSURE, milliseconds);
#endif
}
bool MvCameraBackend::setGain(double gain)
{
#ifdef MATRIC_HAS_MVCAMERA
    return m_handle && MV_CC_SetFloatValue(m_handle, "Gain", static_cast<float>(gain)) == MV_OK;
#else
    return m_capture.set(cv::CAP_PROP_GAIN, gain);
#endif
}
bool MvCameraBackend::setGamma(double gamma)
{
#ifdef MATRIC_HAS_MVCAMERA
    return m_handle && MV_CC_SetFloatValue(m_handle, "Gamma", static_cast<float>(gamma)) == MV_OK;
#else
    return m_capture.set(cv::CAP_PROP_GAMMA, gamma);
#endif
}

QString MvCameraBackend::backendName() const
{
#ifdef MATRIC_HAS_MVCAMERA
    return QStringLiteral("Hikrobot MvCameraControl");
#else
    return QStringLiteral("OpenCV V4L2 fallback");
#endif
}

#ifdef MATRIC_HAS_MVCAMERA
bool MvCameraBackend::convertFrame(const unsigned char *data, unsigned int dataLength,
                                   unsigned int width,
                                   unsigned int height, unsigned int pixelType,
                                   cv::Mat *frame, QString *error)
{
    if (pixelType == PixelType_Gvsp_Mono8) {
        cv::Mat mono(static_cast<int>(height), static_cast<int>(width), CV_8UC1,
                     const_cast<unsigned char *>(data));
        cv::cvtColor(mono, *frame, cv::COLOR_GRAY2BGR);
        return true;
    }
    if (pixelType == PixelType_Gvsp_BGR8_Packed) {
        cv::Mat bgr(static_cast<int>(height), static_cast<int>(width), CV_8UC3,
                    const_cast<unsigned char *>(data));
        bgr.copyTo(*frame);
        return true;
    }

    const unsigned int destinationSize = width * height * 3;
    std::vector<unsigned char> converted(destinationSize);
    MV_CC_PIXEL_CONVERT_PARAM conversion;
    std::memset(&conversion, 0, sizeof(conversion));
    conversion.nWidth = width;
    conversion.nHeight = height;
    conversion.pSrcData = const_cast<unsigned char *>(data);
    conversion.nSrcDataLen = dataLength;
    conversion.enSrcPixelType = static_cast<MvGvspPixelType>(pixelType);
    conversion.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
    conversion.pDstBuffer = converted.data();
    conversion.nDstBufferSize = destinationSize;
    const int result = MV_CC_ConvertPixelType(m_handle, &conversion);
    if (result != MV_OK) {
        if (error) *error = QStringLiteral("MV_CC_ConvertPixelType failed: 0x%1").arg(result, 0, 16);
        return false;
    }
    cv::Mat bgr(static_cast<int>(height), static_cast<int>(width), CV_8UC3, converted.data());
    bgr.copyTo(*frame);
    return true;
}
#endif
