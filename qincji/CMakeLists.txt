cmake_minimum_required(VERSION 3.0)

set(SOURCE_FILES demuxing_decoding.cpp)

project(TEST2)                             # 项目名为SOLIDERFIRE

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")  # -Wall：输出警告的信息
# "${CMAKE_CXX_FLAGS} -Wall"表示在原有的CMAKE_CXX_FLAGS后添加-Wall

set(CMAKE_BUILD_TYPE Debug)    # 让输出的可执行文件是可debug的


# include_directories(${CMAKE_SOURCE_DIR}/ffmpeg-5.0.1/include)
# link_directories(
# 		${CMAKE_SOURCE_DIR}/ffmpeg-5.0.1/lib/    # CMakeLists.txt中似乎不需要-l用来指定编译时具体需要什么库
# )

# include_directories(${CMAKE_SOURCE_DIR}/ffmpeg/include)
# link_directories(
# 		${CMAKE_SOURCE_DIR}/ffmpeg/lib/    # CMakeLists.txt中似乎不需要-l用来指定编译时具体需要什么库
# )

include_directories(${CMAKE_SOURCE_DIR}/ffmpeg-4.2.7/include)
link_directories(
		${CMAKE_SOURCE_DIR}/ffmpeg-4.2.7/lib/    # CMakeLists.txt中似乎不需要-l用来指定编译时具体需要什么库
)

add_executable(my_cmake_exe  demuxing_decoding.cpp )  # 对avframe.c avpacket.c  main.c 三个cpp文件进行编译

target_link_libraries(my_cmake_exe avformat  m) # 指定运行时需要连接的库