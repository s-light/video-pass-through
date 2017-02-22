// video-pass-through minimal
// this peace of code grabs a video stream, shows it in an preview window and
// also streams the unmodified stream to an v4l2-loopback device.
// heavily based on the following examples:
// http://stackoverflow.com/a/39267708/574981
// https://gist.github.com/thearchitect/96ab846a2dae98329d1617e538fbca3c



#include <stdio.h>

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

// needed for getV4lDeviceID
#include <regex>
#include <boost/filesystem.hpp>


// needed for video output
// #include <iostream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

int main(int argc, char** argv ) {

    cv::Mat src;
    cv::Mat out;

    auto id = 0;
    std::cout << "id = " << id << std::endl;

    // init video capture with V4L backend.
    cv::VideoCapture cap(id + cv::CAP_V4L);

    // check if we succeeded
    if (!cap.isOpened()) {
        std::cerr << "ERROR! Unable to open camera" << std::endl;
        return -1;
    }

    // set capture parameters
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    std::cout << "cv::CAP_PROP_FRAME_WIDTH = " << cap.get(cv::CAP_PROP_FRAME_WIDTH) << std::endl;
    std::cout << "cv::CAP_PROP_FRAME_HEIGHT = " << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << std::endl;
    std::cout << "cv::CAP_PROP_FPS = " << cap.get(cv::CAP_PROP_FPS) << std::endl;
    std::cout << "cv::CAP_PROP_FOURCC = " << cap.get(cv::CAP_PROP_FOURCC) << std::endl;

    // get one frame from camera to know frame size and type
    cap >> src;

    // check if we succeeded
    if (src.empty()) {
        std::cerr << "ERROR! blank frame grabbed\n";
        return -1;
    }

    // init output stream
    int width = src.cols;
    int height = src.rows;
    int vidsendsiz = width * height * 3;
    int v4l2out = open("/dev/video40", O_WRONLY);
    if(v4l2out < 0) {
        std::cout << "Error opening v4l2lb device: " << strerror(errno);
        exit(-2);
    }
    // init
    {
        struct v4l2_format v;
        int t;
        v.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        t = ioctl(v4l2out, VIDIOC_G_FMT, &v);
        if( t < 0 ) {
            exit(t);
        }
        v.fmt.pix.width = width;
        v.fmt.pix.height = height;
        v.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
        vidsendsiz = width * height * 3;
        v.fmt.pix.sizeimage = vidsendsiz;
        t = ioctl(v4l2out, VIDIOC_S_FMT, &v);
        if( t < 0 ) {
            exit(t);
        }
    }

    // create preview window
    cv::namedWindow("src", cv::WINDOW_AUTOSIZE );

    // print help / info
    // std::cout << "Writing videofile: " << filename << std::endl
    std::cout << "Press ESC to terminate." << std::endl;
    // loop
    while (1) {
        // check if we succeeded
        if (!cap.read(src)) {
            std::cerr << "ERROR! blank frame grabbed\n";
            break;
        }

        // correct color order for output.
        cv::cvtColor(src, out, CV_BGR2RGB);

        // write frame to loopback
        int size = out.total() * out.elemSize();
        size_t written = write(v4l2out, out.data, size);
        if (written < 0) {
            std::cout << "Error writing v4l2l device" << std::endl;
            close(v4l2out);
            return 1;
        }

        // show live image
        cv::imshow("src", src);

        // handle keys
        switch (cv::waitKey(5)) {
            // escape key
            case 27: {
                return 0;
            }; break;
        }
    }
    return 0;
}
