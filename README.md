## 项目介绍
本项目使用与UCV 协议控制嵌入式板进行拍照，可以设置相机曝光、白平衡等参数。此代码用途为本地UVC 相机控制和嵌入式板拍照调试。可以搭配软件洁面实现拍照实时显示，进行3D 矩阵式采图。
## 开始
### 运行环境
Mac 、 Linux 
运行 start.sh  下载必要依赖 Opencv 和libusb  等库，Mac 环境通过 brew 自行下载opencv

## 注意事项
此代码仅为服务端代码，单独服务端运行，只能进行本地采图测试
```c++
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
```
服务端代码可根据实际使用自行编写，传输协议为和端口自行查看代码，也可私信我。