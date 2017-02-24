# video-pass-through
openCV example to pass a video stream through to a v4l2-loopback device

developed and tested under kubuntu 16.10 64bit.   
with
* [v4l2-loopback](https://github.com/umlaeute/v4l2loopback)
* [OpenCV 3.2 build from git](http://opencv.org/)


## build:

`/video-pass-through$ cmake .`  
then  
`make`

this builds both - the 'minimal' example and the 'video-pass-through' app.

usage:
`$ ./video-pass-through /dev/video20 /dev/video42`  
where video20 is your input device and video42 is a v4l2-loopback device.  
the app displays a live view of the captured stream.  
as output format currently it is fixed as `V4L2_PIX_FMT_YUV420`
(can be changed in the source..)

its also possible to save the current frame as 'my_image.png' by hitting 's'  

theoretically hitting ' ' (space) should print the ['Current position of the video file in milliseconds'](http://docs.opencv.org/3.2.0/d4/d15/group__videoio__flags__base.html#ggaeb8dd9c89c10a5c63c139bf7c4f5704da7c2fa550ba270713fca1405397b90ae0) - but for me it showed
`cv::CAP_PROP_POS_MSEC = 0`  
there are other people expiring the same behavior: [OpenCV: VideoCapture::get(CV_CAP_PROP_POS_MSEC ) returns 0](http://www.answers.opencv.org/question/100052/opencv-videocapturegetcv_cap_prop_pos_msec-returns-0/)
