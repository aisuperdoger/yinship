/**
 * @author 秦城季
 * @email xhunmon@126.com
 * @Blog https://qincji.gitee.io
 * @date 2021/01/03
 * description: FFmpeg Demuxing（解封装）  <br>
 */
extern "C" {
#include <libavformat/avformat.h>
}

int main() {
    const char *src_filename = "source/Kobe.flv";
    const char *out_filename_v = "output/kobe3.h264";//Output file URL
    const char *out_filename_a = "output/kobe.mp3";
    AVFormatContext *fmt_ctx;

    /**(1)*/
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }

    /**(2)*/
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    FILE *fp_audio = fopen(out_filename_a, "wb+");
    FILE *fp_video = fopen(out_filename_v, "wb+");

    AVPacket pkt;
    int video_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    int audio_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    //使用AVBitStreamFilterContext对FLV视频解封装数据进行处理，封装成 NALU 形式的。
    //如果不使用工具，可以参照这篇文章自行封装：https://qincji.gitee.io/2021/01/02/ffmpeg/07_flv/
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");
    //fmt_ctx->streams[video_idx]->codec->extradata包含了sps和pps的数据，需要组装成NALU
    /**(3)*/
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        AVPacket orig_pkt = pkt;
        //针对每一个AVPacket进行处理。
        if(orig_pkt.stream_index == video_idx){
            //封装成 NALU 形式的
            av_bitstream_filter_filter(h264bsfc, fmt_ctx->streams[video_idx]->codec, NULL, &orig_pkt.data, &orig_pkt.size, orig_pkt.data, orig_pkt.size, 0);
            fprintf(stderr,"Write Video Packet. size:%d\tpts:%lld\n", orig_pkt.size, orig_pkt.pts);
            fwrite(orig_pkt.data, 1, orig_pkt.size, fp_video);
        } else if(orig_pkt.stream_index == audio_idx){
            fprintf(stderr,"Write Audio Packet. size:%d\tpts:%lld\n", orig_pkt.size, orig_pkt.pts);

        }

        av_packet_unref(&orig_pkt);
    }
    av_bitstream_filter_close(h264bsfc);
    /**(4)*/
    avformat_close_input(&fmt_ctx);
}