#!/bin/bash

# 定义远程主机的用户名、IP 和密码
USER="orangepi"
HOST="192.168.0.159"
PASSWORD="orangepi"
MAX_RETRIES=3  # 最大重试次数
RETRY_DELAY=5  # 每次重试间隔的秒数

# 启用 extglob 模式
shopt -s extglob

# 判断是否安装软件依赖
read -p "是否安装软件依赖? (y/n): " install_dependencies
if [ "$install_dependencies" = "y" ]; then
   # 更新系统包列表
   echo "更新系统包列表..."
   echo "$PASSWORD" | sudo -S apt-get update

    echo "安装 CMake..."
    echo "$PASSWORD" | sudo -S apt-get install -y cmake
    echo "CMake 安装完成。"

    echo "安装 OpenCV..."
    echo "$PASSWORD" | sudo -S apt install -y libopencv-dev
    echo "OpenCV 安装完成。"

    echo "安装 libusb..."
    echo "$PASSWORD" | sudo -S apt-get install -y libusb-1.0-0-dev
    echo "libusb 安装完成。"
else
    echo "跳过安装软件依赖"
fi

echo "所有安装操作已完成。"

# 编译
mkdir build
cd build/
echo "开始编译..."
cmake ..
make

echo "编译完成，请开始运行"
