rtspstreammuxtask:rtspstreammuxtask.cpp
	gcc rtspstreammuxtask.cpp -o rtspstreammuxtask  -g -I ./ffmpeg-4.2.7/include -L ./ffmpeg-4.2.7/lib/ -l avcodec -l avutil -l avformat -l swscale  -lm -Wl,-rpath=./ffmpeg-4.2.7/lib/

# g++ main.cpp -o main  -g  -L ../linsheng/ffmpeg-4.2.7/lib  -l avformat -l avcodec -l avutil -l swscale -lm -I ../linsheng/ffmpeg-4.2.7/include -Wl,-rpath=../linsheng/ffmpeg-4.2.7/lib # 编译