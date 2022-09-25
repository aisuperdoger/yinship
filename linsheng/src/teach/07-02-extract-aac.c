// 本代码将媒体文件中的音频利用AAC ADTS格式进行保存
// gcc src/7_demux_decode/07-02-extract-aac.c -o bin/07-02-extract-aac  -g  -L ffmpeg-4.2.7/lib  -l avformat -l avcodec -l avutil -lm -I ffmpeg-4.2.7/include -Wl,-rpath=ffmpeg-4.2.7/lib # 编译
// bin/07-02-extract-aac sources/believe.mp4  sources/believe_out.aac  # 运行

#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>

#define ADTS_HEADER_LEN 7;

const int sampling_frequencies[] = {
    // aac中有下面几种采样率的方案
    96000, // 0x0
    88200, // 0x1
    64000, // 0x2
    48000, // 0x3
    44100, // 0x4
    32000, // 0x5
    24000, // 0x6
    22050, // 0x7
    16000, // 0x8
    12000, // 0x9
    11025, // 0xa
    8000   // 0xb
    // 0xc d e f是保留的
};

int adts_header(char *const p_adts_header, const int data_length,
                const int profile, const int samplerate,
                const int channels)
{

    int sampling_frequency_index = 3; // 默认使用48000hz
    int adtsLen = data_length + 7;    // 数据+7个字节的固定头部

    int frequencies_size = sizeof(sampling_frequencies) / sizeof(sampling_frequencies[0]);
    int i = 0;
    for (i = 0; i < frequencies_size; i++)
    {
        if (sampling_frequencies[i] == samplerate)
        {
            sampling_frequency_index = i;
            break;
        }
    }
    if (i >= frequencies_size)
    {
        printf("unsupport samplerate:%d\n", samplerate);
        return -1;
    }

    p_adts_header[0] = 0xff;      // syncword:0xfff                          syncword的高8bits
    p_adts_header[1] = 0xf0;      // syncword:0xfff                          syncword的低4bits
    p_adts_header[1] |= (0 << 3); // MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
                                  //  |= (0 << 3)相对于将0放在相应位置。这里直接使用|0代替|= (0 << 3)不行吗？
    p_adts_header[1] |= (0 << 1); // Layer:0                                 2bits
    p_adts_header[1] |= 1;        // protection absent:1                     1bit

    p_adts_header[2] = (profile) << 6;                          // profile:profile              2bits，profile占用的是p_adts_header的高两位
    p_adts_header[2] |= (sampling_frequency_index & 0x0f) << 2; // sampling frequency index:sampling_frequency_index  4bits。
                                                                //  sampling_frequency_index的高四位没有用，所以使用& 0x0f将高四位置零
    p_adts_header[2] |= (0 << 1);               // private bit:0                   1bit
    p_adts_header[2] |= (channels & 0x04) >> 2; // channel configuration:channels  高1bit

    p_adts_header[3] = (channels & 0x03) << 6;      // channel configuration:channels 低2bits
    p_adts_header[3] |= (0 << 5);                   // original：0                1bit
    p_adts_header[3] |= (0 << 4);                   // home：0                    1bit
    p_adts_header[3] |= (0 << 3);                   // copyright id bit：0        1bit
    p_adts_header[3] |= (0 << 2);                   // copyright id start：0      1bit
    p_adts_header[3] |= ((adtsLen & 0x1800) >> 11); // frame length：value   高2bits

    p_adts_header[4] = (uint8_t)((adtsLen & 0x7f8) >> 3); // frame length:value    中间8bits
    p_adts_header[5] = (uint8_t)((adtsLen & 0x7) << 5);   // frame length:value    低3bits
    p_adts_header[5] |= 0x1f;                             // buffer fullness:0x7ff 高5bits
    p_adts_header[6] = 0xfc;                              //11111100       //buffer fullness:0x7ff 低6bits
    // number_of_raw_data_blocks_in_frame：
    //    表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧。

    return 0;
}

int main(int argc, char *argv[])
{
    int ret = -1;
    char errors[1024];

    char *in_filename = NULL;
    char *aac_filename = NULL;

    FILE *aac_fd = NULL;

    int audio_index = -1;
    int len = 0;

    AVFormatContext *ifmt_ctx = NULL;
    AVPacket pkt;

    if (argc < 3)
    {
        printf("the count of parameters should be more than three!\n");
        return -1;
    }

    in_filename = argv[1];  // 输入文件
    aac_filename = argv[2]; // 输出文件

    if (in_filename == NULL || aac_filename == NULL)
    {
        printf("src or dts file is null, plz check them!\n");
        return -1;
    }

    aac_fd = fopen(aac_filename, "wb");
    if (!aac_fd)
    {
        printf("Could not open destination file %s\n", aac_filename);
        return -1;
    }

    // 打开输入文件
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, NULL, NULL)) < 0)
    {
        printf("打开输入文件错误\n");
        return -1;
    }


    // 初始化packet
    av_init_packet(&pkt);

    // 查找audio对应的steam index
    audio_index = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audio_index < 0)
    {
        printf("无音频流\n");
        return AVERROR(EINVAL);
    }


    if (ifmt_ctx->streams[audio_index]->codecpar->codec_id != AV_CODEC_ID_AAC)
    {
        printf("the media file no contain AAC stream, it's codec_id is %d\n",
               ifmt_ctx->streams[audio_index]->codecpar->codec_id);
        goto failed;
    }
    // 读取媒体文件，并把aac数据帧写入到本地文件
    while (av_read_frame(ifmt_ctx, &pkt) >= 0) // 每次循环读取一个包，这个包有可能是音频，也有可能是视频
    {
        if (pkt.stream_index == audio_index)
        {
            char adts_header_buf[7] = {0};         // ADTS头部为7个字节
            adts_header(adts_header_buf, pkt.size, // 生成ADTS头部adts_header_buf
                        ifmt_ctx->streams[audio_index]->codecpar->profile,
                        ifmt_ctx->streams[audio_index]->codecpar->sample_rate,
                        ifmt_ctx->streams[audio_index]->codecpar->channels);
            fwrite(adts_header_buf, 1, 7, aac_fd);       // 写adts header , ts流不适用，ts流分离出来的packet带了adts header
            len = fwrite(pkt.data, 1, pkt.size, aac_fd); // 写adts data
            if (len != pkt.size)
            {
                av_log(NULL, AV_LOG_DEBUG, "warning, length of writed data isn't equal pkt.size(%d, %d)\n",
                       len,
                       pkt.size);
            }
        }
        av_packet_unref(&pkt);
    }

failed:
    // 关闭输入文件
    if (ifmt_ctx)
    {
        avformat_close_input(&ifmt_ctx);
    }
    if (aac_fd)
    {
        fclose(aac_fd);
    }

    return 0;
}