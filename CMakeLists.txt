cmake_minimum_required(VERSION 3.10)

# プロジェクト名と使用する言語を設定
project(UdpForwardTcp LANGUAGES CXX)

# 実行ファイルを指定
add_executable(UdpForwardTcp main.cpp UdpToTcpForwarder.cpp)

# 必要なライブラリをリンク
target_link_libraries(UdpForwardTcp pthread)
