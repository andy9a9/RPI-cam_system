#include "common.h"
#include "logger.h"
#include <sys/stat.h>
#include "base64.h"

#include <opencv2/imgproc.hpp>

#include "picture.h"

CCamera::CCamera() {
    m_connected = false;
    m_pRaspiCam = NULL;
    m_ID.clear();
}

CCamera::~CCamera() {
    Disconnect();
    if (m_pRaspiCam) delete m_pRaspiCam;
    m_pRaspiCam = NULL;
    m_ID.clear();
}

bool CCamera::Connect() {
    if (m_connected) return true;
    m_connected = true;

    // connect to camera
    if (!m_pRaspiCam->open()) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not open camera!");
        m_connected = false;
    }

    return m_connected;
}

void CCamera::Disconnect() {
    if (!m_connected) return;
    m_connected = false;

    // disconnect
    m_pRaspiCam->release();
}

bool CCamera::Init() {
    m_pRaspiCam = new raspicam::RaspiCam_Cv;

    // check initialization
    if (m_pRaspiCam == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "RaspiCam is not initialization failed!");
        return false;
    }

    //set camera parameters
    m_pRaspiCam->set(CV_CAP_PROP_FORMAT, CV_8UC3); // color
    m_pRaspiCam->set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH); // width
    m_pRaspiCam->set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT); // height

    // get camera ID
    m_ID = m_pRaspiCam->getId();
    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "RaspiCam ID: %s", m_ID.c_str());

    return true;
}

bool CCamera::Capture(cv::Mat &outImage, int count) {
    // check output image
    if (&outImage == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "output image is null!");
        return false;
    }

    if (!m_connected) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "camera is not open!");
        return false;
    }

    // take images
    for (int i = 1; i <= count; i++) {
        // grab the next frame from camera
        if (!m_pRaspiCam->grab()) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not grab frame from camera!");
            return false;
        }
        // decode grabbed frame
        m_pRaspiCam->retrieve(outImage);

        if ((i % 10) == 0)
            CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%i frames were taken", i);
    }

    cv::Mat channels[3];
    // split image into channels
    cv::split(outImage, channels);
    // reverse red and blue channels
    std::swap(channels[0], channels[2]);
    // merge swapped channels back into image
    cv::merge(channels, 3, outImage);

    return true;
}

CPicture::CPicture() {
    m_pCamera = NULL;
    m_pImage = NULL;
    m_picPath.clear();
}

CPicture::~CPicture() {
    m_pCamera->Disconnect();
    delete m_pCamera;
    m_pCamera = NULL;

    m_pImage->release();
    delete m_pImage;
    m_pImage = NULL;

    m_picPath.clear();
}

bool CPicture::Init(const std::string &outpuPath, bool useCamera) {
    // check if output path for pictures is not null
    if (!outpuPath.size()) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "output path for pictures is null!");
        return false;
    }

    // check if directory exist
    if (access(outpuPath.c_str(), F_OK) < 0) {
        // create new directory
        if (mkdir(outpuPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not create path %s!", outpuPath.c_str());
            return false;
        }
    }

    // check if directory is writable
    if (access(outpuPath.c_str(), W_OK) < 0) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "path %s is not writable!", outpuPath.c_str());
        return false;
    }

    // check if camera should be used
    if (useCamera) {
        // init camera
        m_pCamera = new CCamera();
        if (!m_pCamera->Init()) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "camera was not initialized!");
            return false;
        }

        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "camera was successfully initialized");
    } else CLogger::GetLogger()->LogPrintf(LL_DEBUG, "camera was not used");

    // set output path
    m_picPath = outpuPath;

    return true;
}

bool CPicture::TakePicture(const std::string &newFile) {
    // check initialization
    if (m_pCamera == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "camera is not initialized!");
        return false;
    }

    // connect to camera
    if (!m_pCamera->Connect()) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not connect to camera!");
        return false;
    }

    // clean old image
    if (m_pImage != NULL) {
        m_pImage->release();
        delete m_pImage;
        m_pImage = NULL;
    }

    // create new image
    m_pImage = new cv::Mat;

    // take picture from camera
    if (!m_pCamera->Capture(*m_pImage, IMAGE_FRAME_COUNT)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not get picture from camera!");
        return false;
    }

    // check if picture has to be saved
    if (&newFile != NULL) {
        // save image
        if (!cv::imwrite(m_picPath + newFile, *m_pImage)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not save picture \"%s\"!", newFile.c_str());
            return false;
        }
    }

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "picture has been taken from camera");

    return true;
}

void CPicture::PutText(const std::string &text, const std::string &existingImage,
    const cv::Point &invCoord, const cv::Scalar &color) {

    cv::Mat outImage;
    cv::Mat *pImage = m_pImage;

    if (existingImage.size()) {
        if (m_pImage == NULL) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "image to load is null!");
            return;
        }
    } else {
        // check if image exist with rw rights
        if (!access(existingImage.c_str(), R_OK | W_OK)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "image is not readable or writable!");
            return;
        }

        // load image
        outImage = cv::imread(m_picPath + existingImage, CV_LOAD_IMAGE_UNCHANGED);
        pImage = &outImage;
    }

    // put text on image
    cv::putText(*pImage, text, cv::Point(pImage->size().height - invCoord.x, pImage->size().height - invCoord.y),
        cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, color);

    // check if image needs to be saved
    if (existingImage.size()) {
        // write image file
        cv::imwrite(m_picPath + existingImage, *pImage);
    }

    // clean image
    outImage.release();
    pImage = NULL;
    delete(pImage);
}

void CPicture::Serialize(const cv::Mat image, std::stringstream &outStream) {
    // serialize the width, height, type and size of the matrix
    int width = image.cols;
    int height = image.rows;
    int type = image.type();
    size_t size = image.total() * image.elemSize();

    // write the data to stringstream
    outStream.write((char*)(&width), sizeof(int));
    outStream.write((char*)(&height), sizeof(int));
    outStream.write((char*)(&type), sizeof(int));
    outStream.write((char*)(&size), sizeof(size_t));

    // write the whole image data
    outStream.write((char*)image.data, size);
}

void CPicture::Deserialize(std::stringstream inStream, cv::Mat &outImage) {
    int width = 0;
    int height = 0;
    int type = 0;
    size_t size = 0;

    // read the width, height, type and size of the buffer
    inStream.read((char*)(&width), sizeof(int));
    inStream.read((char*)(&height), sizeof(int));
    inStream.read((char*)(&type), sizeof(int));
    inStream.read((char*)(&size), sizeof(size_t));

    // allocate a buffer for the pixels
    char* data = new char[size];

    // read the pixels from the stringstream
    inStream.read(data, size);

    // construct the image (clone it so that it won't need our buffer anymore)
    outImage = cv::Mat(height, width, type, data).clone();

    // clean
    delete[]data;
    inStream.clear();
}

std::string CPicture::EncodePicture(const std::string &existingImage) {
    cv::Mat outImage;
    cv::Mat *pImage = m_pImage;
    std::string outEncode = "";

    if (existingImage.size()) {
        if (m_pImage == NULL) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "image to load is null!");
            return outEncode;
        }
    } else {
        // check if image exist with rw rights
        if (!access(existingImage.c_str(), R_OK)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "image is not readable!");
            return outEncode;
        }

        // load image
        outImage = cv::imread(m_picPath + existingImage, CV_LOAD_IMAGE_UNCHANGED);
        pImage = &outImage;
    }

    // serialize image
    std::stringstream ss;
    Serialize(*pImage, ss);

    // Base64 encode the image
    outEncode = Base64Encode(reinterpret_cast<const unsigned char*>(ss.str().c_str()), ss.str().length());

    // clean
    ss.clear();
    outImage.release();
    pImage = NULL;
    delete(pImage);

    return outEncode;
}