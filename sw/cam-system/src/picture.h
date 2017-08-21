#ifndef PICTURE_H_
#define PICTURE_H_

#include <string>
#include <opencv/cv.h>
#include <raspicam/raspicam_cv.h>

#define FRAME_WIDTH 1024
#define FRAME_HEIGHT 768

class CCamera {
public:
    CCamera();
    ~CCamera();

    bool Init();

    bool Connect();
    inline bool isConnected() { return m_connected; }
    void Disconnect();

    bool Capture(cv::Mat &outImage, int count = 1);

private:
    raspicam::RaspiCam_Cv *m_pRaspiCam;
    bool m_connected;
    std::string m_ID;
};

#define IMAGE_FRAME_COUNT 10

class CPicture {
public:
    CPicture();
    ~CPicture();

    bool InitCam(void);
    bool TakePicture(const std::string &newFile = "");
    void PutText(const std::string &text, const std::string &existingImage = "",
        const cv::Point &invCoord = cv::Point(120, 20),
        const cv::Scalar &color = cv::Scalar(255, 255, 255));

    std::string EncodePictureBase64(const std::string &existingImage = "");
    std::string DencodePictureBase64(const std::string &existingImage = "");

private:
    void Serialize(const cv::Mat image, std::stringstream &outStream);
    void Deserialize(std::stringstream inStream, cv::Mat &outImage);

private:
    CCamera *m_pCamera;
    cv::Mat *m_pImage;
};

#endif // PICTURE_H_
