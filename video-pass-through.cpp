// video-pass-through
// this peace of code grabs a video stream, shows it in an preview window and
// also streams the unmodified stream to an v4l2-loopback device.
// heavily based on the following examples:
// http://stackoverflow.com/a/39267708/574981
// https://gist.github.com/thearchitect/96ab846a2dae98329d1617e538fbca3c



#include <stdio.h>
#include <string>
#include <cstring>

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

// using explicit cv::
// so we are not polutiong global namespace..
// using namespace cv;

bool saveFrameAsPNG(cv::Mat frame, std::string filename=NULL) {
    // filename
    // if no filename is given
    if (filename.length() <= 0) {
        // generate one with current timestamp.
        // TODO
        filename = "image.png";
    }
    // setup compression parameters
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);
    // save
    try {
        imwrite(filename, frame, compression_params);
    }
    catch (cv::Exception& ex) {
        fprintf(stderr, "Exception converting image to PNG format: %s \n", ex.what());
        return 0;
    }
    // fprintf(stdout, "Saved PNG file as %s.\n", filename);
    std::cout << "Saved PNG file as '" << filename << "'" << std::endl;
    return 1;
}


int getV4lDeviceID(std::string pathToDevice) {
    // http://stackoverflow.com/questions/20266743/using-device-name-instead-of-id-in-opencv-method-videocapture-open
    // http://stackoverflow.com/a/29081412/574981
    // #include <regex>
    // #include <boost/filesystem.hpp>
    // boost::filesystem::path path( "/dev/video19" );
    boost::filesystem::path path(pathToDevice);
    auto target = boost::filesystem::canonical(path).string();
    std::regex exp( ".*video(\\d+)" );
    std::smatch match;
    std::regex_search( target, match, exp );
    // std::cout << "match[1] = " << match[1] << std::endl;
    std::string id_string = match[1];
    // const char* id_char = id_string;
    // const char* id_char = (match[1]).c_str();
    // auto id = strtod( id_char, NULL );
    auto id = strtod( id_string.c_str(), NULL );
    // auto id = strtod("1", NULL);
    // std::cout << "id = " << id << std::endl;
    // auto cap = cv::VideoCapture( id );
    return id;
}

std::string decode_fourcc(int value) {
    // http://answers.opencv.org/question/81707/cv_cap_prop_fourcc-not-returning-video-codec-on-some-videos/
    char vcodec[] = {
        (char)(value & 0XFF),
        (char)((value & 0XFF00) >> 8),
        (char)((value & 0XFF0000) >> 16),
        (char)((value & 0XFF000000) >> 24),
        0
    };
    return vcodec;
}

int openV4l2Output(std::string device, int width, int height) {
    int v4l2out = open(device.c_str(), O_WRONLY);
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
        // V4L2_PIX_FMT_RGB24
        // V4L2_PIX_FMT_GREY
        // V4L2_PIX_FMT_YUYV
        // V4L2_PIX_FMT_YUV420
        v.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
        int vidsendsiz = width * height * 3;
        v.fmt.pix.sizeimage = vidsendsiz;
        t = ioctl(v4l2out, VIDIOC_S_FMT, &v);
        if( t < 0 ) {
            exit(t);
        }
    }
    return v4l2out;
}




const std::string cmd_keys =
"{help h usage ?     |              | print this message   }"
"{@input             | /dev/video20 | camera input         }"
"{@output            | /dev/video40 | v4l2-loopback output }"
"{uv updatepreview   | true         | update preview window }"
;


int main(int argc, char** argv ) {
    // switch to CommandlineParser
    // http://docs.opencv.org/3.2.0/d0/d2e/classcv_1_1CommandLineParser.html
    cv::CommandLineParser parser(argc, argv, cmd_keys);
    parser.about("video-pass-through example v0.1.0");
    parser.printMessage();
    // if (parser.has("help")) {
    //     parser.printMessage();
    //     return 0;
    // }
    if (!parser.check()) {
        parser.printErrors();
        return 0;
    }
    std::cout << std::endl << std::endl;

    auto updatepreview = parser.get<bool>("updatepreview");

    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }

    // input image
    cv::Mat src;
    // output image
    cv::Mat out;

    auto device_src = parser.get<std::string>("@input");
    std::cout << "device_src = " << device_src << std::endl;
    cv::VideoCapture cap(device_src, cv::CAP_V4L);

    // check if we succeeded
    if (!cap.isOpened()) {
        std::cerr << "ERROR! Unable to open input camera\n";
        return -1;
    }

    // set capture parameters
    // cap.set(cv::CAP_PROP_FOURCC, cv::CV_FOURCC('M', 'J', 'P', 'G'));
    // cap.set(cv::CAP_PROP_FOURCC, cv::CV_FOURCC('H', '2', '6', '4'));
    // cap.set(cv::CAP_PROP_FOURCC, cv::CV_FOURCC('Y', 'U', 'Y', 'V'));
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    std::cout << "cv::CAP_PROP_FPS = " << cap.get(cv::CAP_PROP_FPS) << std::endl;
    std::cout << "cv::CAP_PROP_POS_MSEC = " << cap.get(cv::CAP_PROP_POS_MSEC) << std::endl;
    std::cout << "cv::CAP_PROP_FOURCC = " << cap.get(cv::CAP_PROP_FOURCC) << std::endl;
    std::cout << "cv::CAP_PROP_FOURCC = " << decode_fourcc(cap.get(cv::CAP_PROP_FOURCC)) << std::endl;

    // get one frame from camera to know frame size and type
    cap >> src;
    // init out array
    // src >> out;

    // check if we succeeded
    if (src.empty()) {
        std::cerr << "ERROR! blank frame grabbed\n";
        return -1;
    }
    // bool isColor = (src.type() == CV_8UC3);
    // auto src_size = src.size();
    // std::cout << "src_size" << src_size << std::endl;

    // init output stream
    auto device_out = parser.get<std::string>("@output");
    int width = src.cols;
    int height = src.rows;
    // int vidsendsiz = width * height * 3;
    int v4l2out = openV4l2Output(
        device_out,
        width,
        height
    );


    // cv::namedWindow("src", cv::WINDOW_AUTOSIZE );
    cv::namedWindow("src", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO );
    // show initial frame
    cv::imshow("src", src);

    // print keyboard shortcuts
    // std::cout << "Writing videofile: " << filename << std::endl
    std::cout << "Press ESC to terminate." << std::endl;
    std::cout << "Press 'space' to show CAP_PROP_POS_MSEC" << std::endl;
    // std::cout << "Press 'o' to show camera driver option dialog" << std::endl;
    std::cout << "Press 's' to save current frame as png image." << std::endl;
    std::cout << "Press 'u' to toggle updatepreview." << std::endl;

    // loop
    while (1) {
        // prepared for multicam support:
        // http://docs.opencv.org/3.2.0/d8/dfe/classcv_1_1VideoCapture.html#ae38c2a053d39d6b20c9c649e08ff0146
        // first grab all frames
        bool grabSucceeded = true;
        grabSucceeded = cap.grab();
        // grabSucceeded = grabSucceeded && cap2.grab();
        // check if we succeeded
        if (!grabSucceeded) {
            std::cerr << "ERROR! blank frame grabbed\n";
            break;
        }
        // then use the slower retrieve method to get (and decode) the actual image.
        cap.retrieve(src);

        // correct color order for output.
        cv::cvtColor(src, out, cv::COLOR_BGR2YUV_I420);
        // http://docs.opencv.org/3.2.0/de/d25/imgproc_color_conversions.html
        // http://docs.opencv.org/3.2.0/d7/d1b/group__imgproc__misc.html#ga4e0972be5de079fed4e3a10e24ef5ef0
        // COLOR_BGR2RGB
        // COLOR_BGR2YUV
        // COLOR_BGR2GRAY
        // COLOR_BGR2YUV_I420

        // write frame to loopback
        int size = out.total() * out.elemSize();
        size_t written = write(v4l2out, out.data, size);
        if (written < 0) {
            std::cout << "Error writing v4l2l device";
            close(v4l2out);
            return 1;
        }

        // TODO: optional scaled down version
        // TODO: optional mouse colorpicker
        if (updatepreview) {
            // show input stream
            cv::imshow("src", src);
        }


        // give image-update time and handle keys
        switch (cv::waitKey(1)) {
            case 's': {
                // saveFrameAsPNG(src);
                saveFrameAsPNG(src, "my_image.png");
            }; break;
            case ' ': {

                // CAP_PROP_POS_MSEC seems to always return 0
                // http://www.answers.opencv.org/question/100052/opencv-videocapturegetcv_cap_prop_pos_msec-returns-0/
                // but should not?!
                // related lines in code:
                // cap_v4l.cpp
                // https://github.com/opencv/opencv/blob/master/modules/videoio/src/cap_v4l.cpp#L159
                // https://github.com/opencv/opencv/blob/master/modules/videoio/src/cap_v4l.cpp#L1651
                // cap_libv4l.cpp
                // https://github.com/opencv/opencv/blob/master/modules/videoio/src/cap_libv4l.cpp#L194
                // https://github.com/opencv/opencv/blob/master/modules/videoio/src/cap_libv4l.cpp#L1414
                std::cout << "cv::CAP_PROP_POS_MSEC = " << cap.get(cv::CAP_PROP_POS_MSEC) << std::endl;

                // not supported:
                // std::cout << "cv::CAP_PROP_POS_FRAMES = " << cap.get(cv::CAP_PROP_POS_FRAMES) << std::endl;

            }; break;
            case 'u': {
                // toggle updatepreview
                updatepreview = !updatepreview;
                std::cout << "updatepreview: " << updatepreview << std::endl;
            }; break;
            // not supported
            // case 'o': {
            //     //show driver settings window
            //     // cap.set(cv::CAP_PROP_SETTINGS, -1);
            // }; break;
            // escape key
            case 27: {
                return 0;
            }; break;
            // default:
            //     1;
        }
    }
    return 0;
}
