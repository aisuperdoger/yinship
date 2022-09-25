#include <iostream>
#include <stdio.h>
// #include "pch.h"
extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/mathematics.h"
}

void rtsp()
{

	AVFormatContext *pFormatCtx;
	int i, videoindex;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int ret, got_picture;

	// struct SwsContext *img_convert_ctx;
	//下面是RTSP地址,按照使用的网络摄像机的URL格式即可
	char filepath[] = "rtsp://admin:a1234567@ten01.adapdo.com:6002/Streaming/Channels/102";

	// av_register_all();    //初始化所有组件，只有调用了该函数，才能使用复用器和编解码器,在所有FFmpeg程序中第一个被调用
	avformat_network_init();			   //加载socket库以及网络加密协议相关的库，为后续使用网络相关提供支持
	pFormatCtx = avformat_alloc_context(); //用来申请AVFormatContext类变量并初始化默认参数。申请的空间通过void avformat_free_context(AVFormatContext *s)函数释放。

	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) // 打开网络流或文件流
	{
		printf("Couldn't open input stream.\n");
		return;
	}
	// if (avformat_find_stream_info(pFormatCtx, NULL) < 0) // 读取一部分视音频数据并且获得一些相关的信息
	// {
	// 	printf("Couldn't find stream information.\n");
	// 	return;
	// }
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) //在多个数据流中找到视频流 video stream(类型为AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	if (videoindex == -1)
	{
		printf("Didn't find a video stream.\n");
		return;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id); //查找video stream 相对应的解码器
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) //打开解码器
	{
		printf("Could not open codec.\n");
		return;
	}
	pFrame = av_frame_alloc(); //为解码帧分配内存
	pFrameYUV = av_frame_alloc();
	out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

	// Output Info---输出一些文件（RTSP）信息
	printf("---------------- File Information ---------------\n");
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");

	// img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
									//  pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, 4, NULL, NULL, NULL);

	packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	FILE *fpSave;
	if ((fpSave = fopen("video.h264", "ab")) == NULL) // h264保存的文件名
		return;
	// for (;;)
	//{
	//------------------------------
	if (av_read_frame(pFormatCtx, packet) >= 0) //从流中读取读取数据到Packet中
	{
		if (packet->stream_index == videoindex)
		{
			fwrite(packet->data, 1, packet->size, fpSave); //写数据到文件中
		}
		av_free_packet(packet);
	}
	//}

	//--------------
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx); //需要关闭avformat_open_input打开的输入流，avcodec_open2打开的CODEC
	avformat_close_input(&pFormatCtx);

	return;
}

int main()
{

	rtsp();
	return 0;
}
