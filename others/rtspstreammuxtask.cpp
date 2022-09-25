// .cpp
#include "rtspstreammuxtask.h"
#include <stdio.h>
#include <iostream>
#include <QDebug>
#include <QDateTime>
#include <stdlib.h>

RtspStreamMuxTask::RtspStreamMuxTask(QWidget *parent) :
    QMainWindow(parent)
{
    rtsp();
}

RtspStreamMuxTask::~RtspStreamMuxTask()
{
}

void RtspStreamMuxTask::rtsp()
{
        AVFormatContext *inVFmtCtx=NULL,*outFmtCtx=NULL;
        int frame_index=0;//统计帧数
        int inVStreamIndex=-1,outVStreamIndex=-1;//输入输出视频流在文件中的索引位置
        const char *inVFileName = "rtsp://admin:123456@192.168.31.168:554/type=0&id=1";
        const char *outFileName = "video.mp4";

        //======================输入部分============================//

        inVFmtCtx = avformat_alloc_context();//初始化内存

        //打开输入文件
        //打开一个文件并解析。可解析的内容包括：视频流、音频流、视频流参数、音频流参数、视频帧索引。
        //参数一：AVFormatContext **ps, 格式化的上下文（由avformat_alloc_context分配）的指针。
        //参数二：要打开的流的url,地址最终会存入到AVFormatContext结构体当中。
        //参数三：指定输入的封装格式。一般传NULL，由FFmpeg自行探测。
        //参数四：包含AVFormatContext和demuxer私有选项的字典。返回时，此参数将被销毁并替换为包含找不到的选项
        if(avformat_open_input(&inVFmtCtx,inVFileName,NULL,NULL)<0){
            printf("Cannot open input file.\n");
            return ;
        }

        //查找输入文件中的流
        /*avformat_find_stream_info函数*/
        //参数一：媒体文件上下文。
        //参数二：字典，一些配置选项。      /*媒体句柄*/
        if(avformat_find_stream_info(inVFmtCtx,NULL)<0){
            printf("Cannot find stream info in input file.\n");
            return ;
        }

        //查找视频流在文件中的位置
        for(size_t i=0;i<inVFmtCtx->nb_streams;i++){//nb_streams 视音频流的个数
              //streams ：输入视频的AVStream []数组  codec：该流对应的AVCodecContext
            if(inVFmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){//streams 视频流
                inVStreamIndex=(int)i;
                break;
            }
        }     

        AVCodecParameters *codecPara = inVFmtCtx->streams[inVStreamIndex]->codecpar;//输入视频流的编码参数


        printf("===============Input information========>\n");
        av_dump_format(inVFmtCtx, 0, inVFileName, 0); //输出视频信息
        printf("===============Input information========<\n");


        //=====================输出部分=========================//
        //打开输出文件并填充格式数据
        //参数一：函数调用成功之后创建的AVFormatContext结构体。
        //参数二：指定AVFormatContext中的AVOutputFormat，确定输出格式。指定为NULL，设定后两个参数（format_name或者filename）由FFmpeg猜测输出格式。。
        //参数三：使用该参数需要自己手动获取AVOutputFormat，相对于使用后两个参数来说要麻烦一些。
        //参数四：指定输出格式的名称。根据格式名称，FFmpeg会推测输出格式。输出格式可以是“flv”，“mkv”等等。
        if(avformat_alloc_output_context2(&outFmtCtx,NULL,NULL,outFileName)<0){
            printf("Cannot alloc output file context.\n");
            return;
        }
   

        //打开输出文件并填充数据
        if(avio_open(&outFmtCtx->pb,outFileName,AVIO_FLAG_READ_WRITE)<0){
            printf("output file open failed.\n");
            return;
        }
     

        //在输出的mp4文件中创建一条视频流
        AVStream *outVStream = avformat_new_stream(outFmtCtx,NULL);//记录视频流通道数目。存储视频流通道。
        if(!outVStream){
            printf("Failed allocating output stream.\n");
            return ;
        }

        outVStream->time_base.den=25;//AVRational这个结构标识一个分数，num为分数，den为分母(时间的刻度)
        outVStream->time_base.num=1;
        outVStreamIndex=outVStream->index;
      


        //查找编码器
        //参数一：id请求的编码器的AVCodecID
        //参数二：如果找到一个编码器，则为NULL。
        //H264/H265码流后，再调用avcodec_find_decoder解码后，再写入到/MP4文件中去
        AVCodec *outCodec = avcodec_find_decoder(codecPara->codec_id);
        if(outCodec==NULL){
            printf("Cannot find any encoder.\n");
            return;
        }


        //从输入的h264编码器数据复制一份到输出文件的编码器中
        AVCodecContext *outCodecCtx=avcodec_alloc_context3(outCodec); //申请AVCodecContext空间。需要传递一个编码器，也可以不传，但不会包含编码器。
        //AVCodecParameters与AVCodecContext里的参数有很多相同的
        AVCodecParameters *outCodecPara = outFmtCtx->streams[outVStream->index]->codecpar;

        //avcodec_parameters_copy()来copyAVCodec的上下文。
        if(avcodec_parameters_copy(outCodecPara,codecPara)<0){
            printf("Cannot copy codec para.\n");
            return;
        }
     
        //基于编解码器提供的编解码参数设置编解码器上下文参数
        //参数一：要设置参数的编解码器上下文
        //参数二：媒体流的参数信息 , 包含 码率 , 宽度 , 高度 , 采样率 等参数信息
        if(avcodec_parameters_to_context(outCodecCtx,outCodecPara)<0){
            printf("Cannot alloc codec ctx from para.\n");
            return ;
        }

        //设置编码器参数(不同参数对视频编质量或大小的影响)
        /*outCodecCtx->time_base.den=25;
        outCodecCtx->time_base.num=1;*/
        outCodecCtx->bit_rate =0;//目标的码率，即采样的码率；显然，采样码率越大，视频大小越大  比特率
        outCodecCtx->time_base.num=1;//下面两行：一秒钟25帧
        outCodecCtx->time_base.den=15;
        outCodecCtx->frame_number=1;//每包一个视频帧

     
        //打开输出文件需要的编码器
        if(avcodec_open2(outCodecCtx,outCodec,NULL)<0){
            printf("Cannot open output codec.\n");
            return ;
        }

     

        printf("============Output Information=============>\n");
        av_dump_format(outFmtCtx,0,outFileName,1);//输出视频信息
        printf("============Output Information=============<\n");


        //写入文件头
        if(avformat_write_header(outFmtCtx,NULL)<0){
            printf("Cannot write header to file.\n");
            return ;
        }
      
        //===============编码部分===============//
        //AVPacket 数据结构 显示时间戳（pts）、解码时间戳（dts）、数据时长，所在媒体流的索引等
        AVPacket *pkt = av_packet_alloc();
        //存储每一个视频/音频流信息的结构体
        AVStream *inVStream = inVFmtCtx->streams[inVStreamIndex];

        //循环读取每一帧直到读完 从媒体流中读取帧填充到填充到Packet的数据缓存空间
        while(av_read_frame(inVFmtCtx,pkt)>=0){//循环读取每一帧直到读完
            pkt->dts = 0;//不加这个时间戳会出问题，时间戳比之前小的话 FFmpeg会选择丢弃视频包，现在给视频包打时间戳可以重0开始依次递增。
            if(pkt->stream_index==inVStreamIndex){//确保处理的是视频流 stream_index标识该AVPacket所属的视频/音频流。
                //FIXME：No PTS (Example: Raw H.264)
                //Simple Write PTS
                //如果当前处理帧的显示时间戳为0或者没有等等不是正常值
                if(pkt->pts==AV_NOPTS_VALUE){
                    printf("frame_index:%d\n", frame_index);

                    //Write PTS时间 刻度
                    AVRational time_base1 = inVStream->time_base;

                    //Duration between 2 frames (us) 时长
                    //AV_TIME_BASE 时间基
                    //av_q2d(AVRational);该函数负责把AVRational结构转换成double，通过这个函数可以计算出某一帧在视频中的时间位置
                    //r_frame_rate
                    int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(inVStream->r_frame_rate);
                    //Parameters参数
                    pkt->pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
                    pkt->dts = pkt->pts;
                    pkt->duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
                    frame_index++;
                }
                //Convert PTS/DTS
                //AVPacket
                // pts 显示时间戳
                // dts 解码时间戳
                // duration 数据的时长，以所属媒体流的时间基准为单位
                // pos 数据在媒体流中的位置，未知则值为-1
                // 标识该AVPacket所属的视频/音频流。
                pkt->pts = av_rescale_q_rnd(pkt->pts, inVStream->time_base, outVStream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt->dts = av_rescale_q_rnd(pkt->dts, inVStream->time_base, outVStream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt->duration = av_rescale_q(pkt->duration, inVStream->time_base, outVStream->time_base);
                pkt->pos = -1;
                pkt->stream_index = outVStreamIndex;
                printf("Write 1 Packet. size:%5d\tpts:%ld\n", pkt->size, pkt->pts);

                //Write
                if (av_interleaved_write_frame(outFmtCtx, pkt) < 0) {
                    printf("Error muxing packet\n");
                    break;
                }
                //处理完压缩数据之后，并且在进入下一次循环之前，
                //记得使用 av_packet_unref 来释放已经分配的AVPacket->data缓冲区。
                av_packet_unref(pkt);
            }
        }
      
        av_write_trailer(outFmtCtx);

        //=================释放所有指针=======================
        av_packet_free(&pkt);//堆栈上数据缓存空间
        av_free(inVStream);//存储每一个视频/音频流信息的结构体
        av_free(outVStream);//在输出的mp4文件中创建一条视频流
        avformat_close_input(&outFmtCtx);//关闭一个AVFormatContext
        avcodec_close(outCodecCtx);
        avcodec_free_context(&outCodecCtx);
        av_free(outCodec);
        avcodec_parameters_free(&outCodecPara);
        avcodec_parameters_free(&codecPara);
        avformat_close_input(&inVFmtCtx);//关闭一个AVFormatContext
        avformat_free_context(inVFmtCtx);//销毁函数
        avio_close(outFmtCtx->pb);

}


