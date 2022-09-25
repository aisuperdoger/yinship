//.h
#ifndef RTSPSTREAMMUXTASK_H
#define RTSPSTREAMMUXTASK_H

#include <QMainWindow>
#include <QThread>
#include <QImage>
#include <iostream>
#include <stdio.h>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/mathematics.h"
}


class RtspStreamMuxTask : public QMainWindow
{
    Q_OBJECT

public:
    explicit RtspStreamMuxTask(QWidget *parent = 0);
    ~RtspStreamMuxTask();
    void rtsp();
};

#endif // RTSPSTREAMMUXTASK_H


