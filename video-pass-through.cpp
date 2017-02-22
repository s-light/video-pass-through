// video-pass-through
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
        fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
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
        v.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
        int vidsendsiz = width * height * 3;
        v.fmt.pix.sizeimage = vidsendsiz;
        t = ioctl(v4l2out, VIDIOC_S_FMT, &v);
        if( t < 0 ) {
            exit(t);
        }
    }
    return v4l2out;
}

int main(int argc, char** argv ) {
    if ( argc < 2 )
    {
        printf("usage: /dev/video20 /dev/video40\n");
        // return -1;
    }

    cv::Mat src;
    cv::Mat out;

    // auto device_src = "/dev/video19";
    auto device_src = "/dev/video20";
    if (argc >= 2) {
        device_src = argv[1];
    }
    std::cout << "device_src = " << device_src << std::endl;
    auto id = getV4lDeviceID(device_src);
    std::cout << "id = " << id << std::endl;

    // init video capture with V4L backend.
    cv::VideoCapture cap(id + cv::CAP_V4L);

    // check if we succeeded
    if (!cap.isOpened()) {
        std::cerr << "ERROR! Unable to open camera\n";
        return -1;
    }

    // set capture parameters
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    std::cout << "cv::CAP_PROP_FPS = " << cap.get(cv::CAP_PROP_FPS) << std::endl;
    std::cout << "cv::CAP_PROP_FOURCC = " << cap.get(cv::CAP_PROP_FOURCC) << std::endl;
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
    auto device_out = "/dev/video40";
    if (argc >= 3) {
        device_out = argv[2];
    }
    int width = src.cols;
    int height = src.rows;
    // int vidsendsiz = width * height * 3;
    int v4l2out = openV4l2Output(
        device_out,
        width,
        height
    );


    cv::namedWindow("src", cv::WINDOW_AUTOSIZE );

    // print help / info
    // std::cout << "Writing videofile: " << filename << std::endl
    std::cout << "Press ESC to terminate." << std::endl;
    std::cout << "Press 's' to save current frame as png image." << std::endl;
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
            std::cout << "Error writing v4l2l device";
            close(v4l2out);
            return 1;
        }

        // show live and wait for a key with timeout long enough to show images
        cv::imshow("src", src);

        // handle keys
        switch (cv::waitKey(5)) {
            case 's': {
                saveFrameAsPNG(src, "my_image.png");
            }; break;
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
