//
// Created by Pc H on 2024/7/31.
//
#ifndef UVC_FRAME_USBCAMERA_VIDEO_H
#include "libuvc/libuvc.h"
#include <unistd.h>
#include "opencv2/opencv.hpp"
#include <iostream>
#include <string>
#include <condition_variable>
#include <thread>
#include <fstream>
#include <sstream>
#include <map>
#include "httplib.h"
#include <unordered_map>
#include <thread>
#include <future>

#define UVC_FRAME_USBCAMERA_H

typedef void (*uvc_utils_device_cb)(uvc_frame_t *frame, void *ptr);

struct UVC_UTILS_DEVICE {
    int num;         // 相机编号
    int pid;         // 产品ID（Product ID），用于唯一标识USB设备的型号。
    int vid;         // 厂商ID（Vendor ID），用于识别制造USB设备的厂商。
    int format;      // 视频格式代码，定义了视频数据的编码格式，如YUV、MJPEG等。
    int targetFps;   // 目标帧率（Frames Per Second），指定设备应尝试捕捉视频的帧率。
    int frameWidth;  // 视频帧的宽度，以像素为单位。
    int frameHeight; // 视频帧的高度，以像素为单位。
    int expTime;     // 曝光时间
    uvc_stream_handle_t *strmh;
    uvc_utils_device_cb cb;    // 回调函数指针，用于接收视频流事件或数据的通知。
    uvc_error_t res;           // 操作结果或错误状态，用枚举类型表示操作是否成功或具体的错误类型。
    uvc_device_t *dev;         // 指向uvc_device_t结构的指针，代表一个UVC设备，用于访问设备信息。
    uvc_device_handle_t *devh; // 指向uvc_device_handle_t的指针，代表一个打开的设备句柄，用于设备控制和操作。
    uvc_stream_ctrl_t ctrl;    // uvc_stream_ctrl_t结构，用于管理视频流的控制参数，如帧率和分辨率设置。
};

class usbCamera {
public:
    usbCamera() {
        init();
        inti_cameras_config();
        load_config();
        get_frame_thread("Exp1");
    }

    ~usbCamera() {
        for (auto &cam: cameraDevices) {
            close_uvc(&cam);
        }
        uvc_exit(ctx);
        puts("free_uvc_camera_device ok!");
    }

    void load_config();

    int set_device_num(std::string ip);

private:
    std::vector<UVC_UTILS_DEVICE> cameraDevices;
    uvc_context_t *ctx;
    uvc_device_t **devices;
    std::vector<std::thread> save_image_threads;
    std::vector<std::pair<cv::Mat, int>> save_images;
    std::condition_variable queueCondVar;
    int num_device;
    std::string hostname;
    std::string Port;
    int expTime;
    int AWB;
    std::mutex save_images_mutex;  // 互斥锁定义

public:
    int Server_flag;
    bool Test_flag = false;

private:
    std::unordered_map<std::string, std::string> readConfigFile(const std::string &filename);

    int set_upload_image(const cv::Mat &img, int pictureId, std::string ip);

    int set_time(std::string time);

    void inti_cameras_config();

    int get_camera_info(UVC_UTILS_DEVICE *cam);

    int init();

    void start_stream(UVC_UTILS_DEVICE *cam);

    void capture_image_retry(UVC_UTILS_DEVICE &device);

    void stop_cam_retry(UVC_UTILS_DEVICE *cam);

    // 停止视频流并关闭设备
    void close_uvc(UVC_UTILS_DEVICE *cam);

    void capture_image(UVC_UTILS_DEVICE &device, const std::string &save_path);

    void capture_image_thread1(int start, int run_num, const std::string &save_path);

    uvc_error_t set_camera_config(UVC_UTILS_DEVICE &device);

    int capture_frame(uvc_stream_handle_t *strmh, int num, std::string save_path);

    uvc_error_t set_exposure_time(uvc_device_handle_t *devh, uint32_t exposure_time);

    uvc_error_t set_WB_auto(uvc_device_handle_t *devh, int AWB);

public:
    int get_frame_thread(const std::string &save_path);

    int upload_images(std::string ip);
};

#endif // UVC_FRAME_USBCAMERA_VIDEO_H
