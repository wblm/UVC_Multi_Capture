//
// Created by Pc H on 2024/7/31.
//

#include "usbCamera.h"

std::unordered_map<std::string, std::string> usbCamera::readConfigFile(const std::string &filename) {
    std::unordered_map<std::string, std::string> config;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "无法打开配置文件: " << filename << std::endl;
        return config;
    }

    for (std::string line; std::getline(file, line);) {
        if (line.empty() || line[0] == '#')
            continue;

        auto delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos)
            continue;

        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        config[key] = value;
    }

    return config;
}

void usbCamera::load_config() {
    auto config = readConfigFile("url.txt");
    if (config.empty()) {
        config["hostname"] = '0';
        config["port"] = '0';
        config["expTime"] = '0';
        config["AWB"] = '0';
    }
    for (const auto &[key, value]: config) {
        std::cout << key << " = " << value << std::endl;
    }
    hostname = config["hostname"];
    std::string port = config["port"];
    expTime = stoi(config["expTime"]);
    AWB = stoi(config["AWB"]);
    for (int i = 0; i < num_device; i++) {
        cameraDevices[i].res = set_camera_config(cameraDevices[i]);
        if (cameraDevices[i].res == 0) {
            std::cout << "设备 " << i + 1 << "配置加载成功" << std::endl;
        } else {
            std::cout << "设备 " << i + 1 << "配置加载失败" << std::endl;
        }
    }
}

/********初始化白平衡校准***********/
void usbCamera::inti_cameras_config() {
    for (int index = 0; index < num_device; index++) {
        if (cameraDevices[index].res == 0) {
            cameraDevices[index].res = uvc_set_white_balance_temperature_auto(cameraDevices[index].devh, 1);
            if (cameraDevices[index].res == 0) {
                uvc_perror(cameraDevices[index].res, "初始化WAB");
                uvc_frame_t *frame;
                start_stream(&cameraDevices[index]);
                for (int num = 0; num < 30; num++)
                    uvc_stream_get_frame(cameraDevices[index].strmh, &frame, 800000); // 0.5
                stop_cam_retry(&cameraDevices[index]);
            }
            std::cout << "设备 " << index + 1 << "配置加载成功" << std::endl;
        } else {
            std::cout << "设备 " << index + 1 << "配置加载失败" << std::endl;
        }
    }
}

int usbCamera::init() {
    uvc_error res = uvc_init(&ctx, NULL);
    if (res < 0) {
        uvc_perror(res, "uvc_init");
        return res;
    }
    res = uvc_find_devices(ctx, &devices, 0, 0, NULL);
    if (res < 0) {
        uvc_perror(res, "uvc_find_devices");
        uvc_exit(ctx);
        return res;
    }
    for (num_device = 0; devices[num_device] != NULL; num_device++);
    // set_device_num(num_device);
    std::cout << "一共找到" << num_device << "个设备" << std::endl;
    //    std::string configFile = "config.ini";
    //    auto config = loadConfig(configFile);
    cameraDevices.clear();
    cameraDevices.resize(num_device);
    for (int i = 0; i < num_device; i++) {
        cameraDevices[i].dev = devices[i];
        cameraDevices[i].num = i + 1;
        // cameraDevices[i].cb = frame_callback;
        //        cameraDevices[i].expTime = config[i];
        cameraDevices[i].res = uvc_open(cameraDevices[i].dev, &cameraDevices[i].devh);
        get_camera_info(&cameraDevices[i]);
        cameraDevices[i].res = uvc_get_stream_ctrl_format_size(
                cameraDevices[i].devh, &cameraDevices[i].ctrl,  /* 结果存储在 ctrl 中 */
                (enum uvc_frame_format) cameraDevices[i].format, /* 颜色格式 */
                cameraDevices[i].frameWidth,
                cameraDevices[i].frameHeight, cameraDevices[i].targetFps /* 宽度，高度，帧率 */
        );
        if (cameraDevices[i].res < 0) {
            printf("设备 %d 创建失败\n", i + 1);
            uvc_perror(cameraDevices[i].res, "get_mode"); /* 设备不提供匹配的流 */
        } else {
            printf("设备 %d 创建成功\n", i + 1);
        }
    }
    uvc_free_device_list(devices, 1);
    return res;
}

int usbCamera::set_device_num(std::string ip){
    try {
        httplib::Client cli(ip, 8088);
        auto num = cameraDevices.size();
        std::string data = std::to_string(Server_flag) + " " + std::to_string(num);

        // 发送 POST 请求
        auto response = cli.Post("/setdevice", data, "text/plain");

        // 检查响应
        if (response && response->status == 200) {
            return 1;
        } else {
            return -1;
        }
    }
    catch (...) {
        return -1;
    }
}
int usbCamera::set_upload_image(const cv::Mat &img, int pictureId, std::string ip){
//int usbCamera::set_upload_image(cv::Mat img, int pictureId,std::string ip ) {
    std::string filename = "Exp1/" + std::to_string(pictureId) + "_frame.jpg";
    if (!cv::imwrite(filename, img)) {
        std::cerr << "无法保存图像到 " << filename << std::endl;
        return -1;
    }
    std::cout << "Device" << this->Server_flag << "相机 " << pictureId << " 保存成功" << std::endl;

    if (!Test_flag) {
        // 编码的前必须的data格式，用一个uchar类型的vector
        std::vector<uchar> data_encode;
        cv::imencode(".png", img, data_encode);

        data_encode.push_back(static_cast<uchar>(this->Server_flag));
        data_encode.push_back(static_cast<uchar>(pictureId));
        std::string str_encode(reinterpret_cast<const char*>(data_encode.data()), data_encode.size());

//        std::string str_encode(data_encode.begin(), data_encode.end());
        try {
            httplib::Client cli(ip, 8088);
            auto response = cli.Post("/setimage", str_encode, "text/plain");
            if (response && response->status == 200) {
                return 1;
            } else {
                return -1;
            }
        }
        catch (...) {
            return -1;
        }
    }
    return 1;
}

int usbCamera::set_time(std::string time) {
    try {
        httplib::Client cli(hostname, 8088);
        auto response = cli.Post("/settime", time, "text/plain");
        if (response && response->status == 200) {
            return 1;
        } else {
            return -1;
        }
    }
    catch (...) {
        return -1;
    }
}

int usbCamera::get_camera_info(UVC_UTILS_DEVICE *cam) {
    const uvc_format_desc_t *format_desc = uvc_get_format_descs(cam->devh);
    while (format_desc != NULL) {
        if (format_desc->bDescriptorSubtype != UVC_VS_FORMAT_MJPEG) {
            format_desc = format_desc->next;
            continue;
        }
        const uvc_frame_desc_t *frame_desc = format_desc->frame_descs;
        int max_pix = 0;
        while (frame_desc != NULL) {
            int width = frame_desc->wWidth;
            int height = frame_desc->wHeight;
            int fps = 10000000 / frame_desc->dwDefaultFrameInterval;

            if (max_pix < width * height && fps > 0) {
                max_pix = width * height;
                cam->frameWidth = width;
                cam->frameHeight = height;
                cam->targetFps = fps;
            }
            frame_desc = frame_desc->next;
        }
        format_desc = format_desc->next;
    }
    printf("设备 %d 支持的最大分辨率：%dx%d\n帧率：%d fps\n", cam->num, cam->frameWidth, cam->frameHeight,
           cam->targetFps);
    return 0;
}

void usbCamera::start_stream(UVC_UTILS_DEVICE *cam) {

    uvc_error_t res = uvc_stream_open_ctrl(cam->devh, &cam->strmh, &cam->ctrl);

    if (res < 0) {
        uvc_perror(res, "uvc_stream_open_ctrl");
        uvc_close(cam->devh);
        uvc_unref_device(cam->dev);
        exit(EXIT_FAILURE);
    }
    res = uvc_stream_start(cam->strmh, NULL, NULL, 0);
    if (res < 0) {
        std::cout << "Device NUM :" << cam->num << std::endl;
        uvc_perror(res, "uvc_stream_start erro");
        uvc_stream_close(cam->strmh);
        uvc_close(cam->devh);
        uvc_unref_device(cam->dev);
        //        uvc_exit(cam->ctx);
        exit(EXIT_FAILURE);
    }
}

void usbCamera::capture_image_retry(UVC_UTILS_DEVICE &device) {
    stop_cam_retry(&device);
    start_stream(&device);
}

void usbCamera::stop_cam_retry(UVC_UTILS_DEVICE *cam) {
    uvc_error_t res = uvc_stream_stop(cam->strmh);
    uvc_stream_close(cam->strmh);
}

void usbCamera::close_uvc(UVC_UTILS_DEVICE *cam) {
    uvc_stream_stop(cam->strmh);
    uvc_stream_close(cam->strmh);
    uvc_close(cam->devh);
    uvc_unref_device(cam->dev);
}

void usbCamera::capture_image_thread1(int start, int run_num, const std::string &save_path) {
    std::vector<std::thread> threads;

    for (int i = start - 1; i < run_num; ++i) {
        threads.emplace_back(&usbCamera::capture_image, this, std::ref(cameraDevices[i]), std::ref(save_path));
    }
    for (auto &t: threads) {
        t.join();
    }
}

uvc_error_t usbCamera::set_camera_config(UVC_UTILS_DEVICE &device) {
    set_exposure_time(device.devh, expTime);
    set_WB_auto(device.devh, AWB);

    // 获取自动白平衡温度状态
    uint8_t temperature_auto;
    device.res = uvc_get_white_balance_temperature_auto(device.devh, &temperature_auto, UVC_GET_CUR);
    if (device.res < 0) {
        uvc_perror(device.res, "uvc_get_white_balance_temperature_auto");
    } else {
        if (temperature_auto == 1) {
            std::cout << "Auto White Balance is currently enabled." << std::endl;
        } else {
            std::cout << "Auto White Balance is currently disabled." << std::endl;
        }
    }
    uvc_frame_t *frame;
    start_stream(&device);
    // 预处理30帧，进行配置加载
    for (int i = 0; i < 30; i++)
        uvc_stream_get_frame(device.strmh, &frame, 800000); // 0.5
    stop_cam_retry(&device);
    return UVC_SUCCESS;
}

int usbCamera::capture_frame(uvc_stream_handle_t *strmh, int num, std::string save_path) {
    uvc_error_t res;
    uvc_frame_t *frame;
    res = uvc_stream_get_frame(strmh, &frame, 800000); // 0.5
    res = uvc_stream_get_frame(strmh, &frame, 800000); // 0.5
    res = uvc_stream_get_frame(strmh, &frame, 800000); // 0.5
    if (frame == NULL) {
        std::cout << "frame NULL " << num << std::endl;
        return -101;
    }
    if (res < 0) {
        uvc_perror(res, "uvc_stream_get_frame");
        return res;
    }
    cv::Mat jpegImage = cv::Mat(1, frame->data_bytes, CV_8UC1, frame->data);
    cv::Mat img = cv::imdecode(jpegImage, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "设备" << num << " Failed to decode MJPEG." << std::endl;
        return -1;
    }
    {
        std::lock_guard<std::mutex> lock(save_images_mutex); // 锁住操作
        save_images.emplace_back(std::move(img), num);
    }
    return res;
}

void usbCamera::capture_image(UVC_UTILS_DEVICE &device, const std::string &save_path) {
    uint16_t temperature;
    // 获取当前色温
    device.res = uvc_get_white_balance_temperature(device.devh, &temperature, UVC_GET_CUR);
    if (device.res == UVC_SUCCESS) {
        printf("当前白平衡的色温为: %d K\n", temperature);
    }
    device.res = set_exposure_time(device.devh, expTime);
    if (device.res == -4) {
        int device_index = device.num - 1;
        if (device_index >= 0 && device_index < cameraDevices.size()) {
            cameraDevices.erase(cameraDevices.begin() + device_index); // 删除指定索引的设备
            std::cout << "移除相机设备，索引为 " << device.num << std::endl;
        }
        return;
    }
    start_stream(&device);
    int res = capture_frame(device.strmh, device.num, save_path);
    int test_num = 1;
    while (res < 0 && test_num <= 3) {
        if (test_num >= 1) {
            capture_image_retry(device);
            std::string str =  "ERRO:"+std::to_string(res)+" 相机" + std::to_string(device.num)+ "超时重试" +std::to_string(test_num) +"次";
            set_time(str);
            std::cout << "相机" << device.num << "超时重试" << test_num << "次" << std::endl;
        }
        res = capture_frame(device.strmh, device.num, save_path);
        test_num++;
    }
    stop_cam_retry(&device);
}

uvc_error_t usbCamera::set_exposure_time(uvc_device_handle_t *devh, uint32_t exposure_time) {
    uvc_error_t res;
    if (exposure_time > 0) {
        // 首先将自动曝光模式设置为手动
        res = uvc_set_ae_mode(devh, 1); // 1 代表手动模式
        if (res != UVC_SUCCESS) {
            uvc_perror(res, "uvc_set_ae_mode");
        }
        // 然后设置绝对曝光时间
        res = uvc_set_exposure_abs(devh, exposure_time);
        if (res == UVC_SUCCESS) {
            std::cout << "Exposure time set to: " << exposure_time << " (units of 0.0001 seconds)" << std::endl;
        } else {
            uvc_perror(res, "uvc_set_exposure_abs");
        }
    } else {
        res = uvc_set_ae_mode(devh, 8); // 1 代表手动模式
        if (res != UVC_SUCCESS) {
            uvc_perror(res, "uvc_set_ae_mode");
        }
    }
    return res;
}

uvc_error_t usbCamera::set_WB_auto(uvc_device_handle_t *devh, int AWB) {
    uvc_error_t res;
    uint16_t temperature = AWB; // 设置白平衡色温为 5000K
    if (AWB != 1) {
        std::cout << "设置手动白平衡" << AWB << " K" << std::endl;
        res = uvc_set_white_balance_temperature_auto(devh, 0);
        if (res < 0) {
            std::cout << "Auto White Balance is currently disabled." << std::endl;
        }
        // 设置手动白平衡色温
        res = uvc_set_white_balance_temperature(devh, temperature);
        if (res < 0) {
            uvc_perror(res, "uvc_set_white_balance_temperature");
            return res;
        } else {
            std::cout << "White Balance Temperature set to: " << temperature << "K" << std::endl;
        }
    } else {
        // 打开自动白平衡
        res = uvc_set_white_balance_temperature_auto(devh, 1);
        if (res < 0) {
            uvc_perror(res, "uvc_set_white_balance_temperature_auto");
        }
        std::cout << "自动白平衡启动成功 !" << std::endl;
    }
    return res;
}

int usbCamera::get_frame_thread(const std::string &save_path) {
    save_images.clear();
    auto start_time = std::chrono::high_resolution_clock::now();
    capture_image_thread1(1, cameraDevices.size(), save_path);
    // 结束计时
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "操作耗时: " << elapsed.count() << " 秒" << std::endl;
    set_time("操作耗时: " + std::to_string(elapsed.count()) + " 秒");
    return 0;
}

int usbCamera::upload_images(std::string ip) {
    std::vector<std::thread> threads;
    std::cout << "图像传输开始,目标Ip :" <<ip << std::endl;
    for (const auto &pair: save_images) {
        const cv::Mat &img = pair.first; // 使用引用，避免拷贝
        int num = pair.second;
        if (!img.empty()) {
            threads.emplace_back(&usbCamera::set_upload_image, this, img, num,ip);
        } else {
            std::cout << "图像" << num << "为空" << std::endl;
            return -1;
        }
    }
    for (auto &t: threads) {
        t.join();
    }
    save_images.clear();
    return 0;
}
