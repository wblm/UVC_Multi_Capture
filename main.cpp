#include "usbCamera.h"

usbCamera camera;

// 处理POST请求的回调函数
void handle_post_get_image(const httplib::Request &req, httplib::Response &res) {
    std::string requestData = req.body;// 获取请求体中的数据
    size_t delimiterPos = requestData.find(',');
    if (delimiterPos == std::string::npos) {
        res.set_content("Invalid request format", "text/plain");
        return;
    }
    std::string index = requestData.substr(0, delimiterPos);
    std::string local_ip = requestData.substr(delimiterPos + 1);

    std::cout << "Received index: " << index << std::endl;
    camera.Server_flag = stoi(index) + 1;
    camera.get_frame_thread("Exp1");
    camera.set_device_num(local_ip);
    std::thread([index, local_ip]() {
        camera.upload_images(local_ip);
    }).detach();// 返回响应
std::cout << "Finish taking pictures. Index received: " << index << std::endl;
    // 返回响应
    res.set_content(" Finish taking pictures. Index received: " + index, "text/plain");
}

// 处理 POST 请求的函数
void handle_post_set_config(const httplib::Request &req, httplib::Response &res) {
    // 检查内容类型是否正确
    if (req.is_multipart_form_data()) {
        std::cerr << "Multipart form data not supported in this implementation." << std::endl;
        res.status = 400;  // Bad Request
        return;
    }

    std::string config_data = req.body;  // 获取请求的主体内容

    // 将配置数据保存到文件中
    std::string filename = "url.txt";
    std::ofstream config_file(filename);

    if (config_file.is_open()) {
        config_file << config_data;
        config_file.close();
        std::cout << "配置文件已保存到 " << filename << std::endl;
        camera.load_config();
        res.set_content("Configuration received and saved successfully.", "text/plain");
    } else {
        std::cerr << "无法打开文件保存配置数据。" << std::endl;
        res.status = 500;  // Internal Server Error
        res.set_content("Failed to save configuration.", "text/plain");
    }
}

void reboot_server(const httplib::Request &req, httplib::Response &res){
    system("init 6");
}

// 启动服务器的线程函数
void start_server() {
    // 创建HTTP服务器
    httplib::Server svr;

    // 注册处理"/setimage"路径的POST请求的回调函数
    svr.Post("/getimage", handle_post_get_image);
    svr.Post("/setconfig", handle_post_set_config);
    svr.Post("/reboot_server", reboot_server);
    // 启动服务器，监听端口8080
    std::cout << "Server is running on http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
}


int main(int argc, char *argv[]) {
    bool Local_flag;
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <Local_flag>" << std::endl;
        std::cout << "Local: 1, Server: 0" << std::endl;
        std::cin >> Local_flag;
//        return 1;
    } else {
        Local_flag = std::stoi(argv[1]);
    }
    if (Local_flag) {
        std::cout << "启动本地运行..." << std::endl;
        camera.Test_flag=true;
        char key;
        while (true) {
            std::cout << "s: start, q: quit" << std::endl;
            std::cin >> key;

            if (key == 's') {
                // 调用获取帧的线程函数
                camera.get_frame_thread("Exp1");
            } else if (key == 'q') {
                // 退出循环
                break;
            } else {
                std::cout << "无效输入，请输入 's' 或 'q'" << std::endl;
            }
        }
    } else {
        std::cout << "启动服务器运行..." << std::endl;
        start_server();
    }

    return 0;
}

