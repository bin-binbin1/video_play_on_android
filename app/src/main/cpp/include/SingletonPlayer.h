//
// Created by Administrator on 2024/5/13.
//

#ifndef ANDROIDPLAYER_SINGLETONPLAYER_H
#define ANDROIDPLAYER_SINGLETONPLAYER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
};
#include <log.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <thread>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <ProductQueue.h>
#include "ANWRender.h"
#include "AAudioRender.h"
#include<unistd.h>

void getAVInformation(const char* filePath);

int getAudioAndVideo(const char* src_filename1,const char* video_dst_filename1,const char* audio_dst_filename1);
const int maxFramesQueueSize=5;
const int maxPacketsQueueSize=5;
const int totalFrameNum=maxFramesQueueSize*3;
const int totalPacketNum=maxPacketsQueueSize*5;
typedef uint8_t byte;
class SingletonPlayer {
private:
    static SingletonPlayer* instance;

    ANWRender* anwRender;

    std::thread demuxThread;
    std::thread videoDecoderThread;
    std::thread videoRenderThread;
    std::thread audioDecoderThread;
    std::thread audioRenderThread;

    std::mutex demuxMutex;
    std::condition_variable demuxCV;
    std::mutex videoDecoderMutex;
    std::condition_variable videoDecoderCV;
    std::mutex videoRenderMutex;
    std::condition_variable videoRenderCV;
    std::mutex audioDecoderMutex;
    std::condition_variable audioDecoderCV;
    std::mutex audioRenderMutex;
    std::condition_variable audioRenderCV;

    const char* file;
    bool pauseAV;
    bool stopThreads = false;
    const int frames_per_second = 24;
    std::chrono::milliseconds frame_duration;
    uint64_t duration_ms;
    uint64_t position_ms;
    bool toThePlace;
    uint64_t target;

    SwsContext* sws_ctx;
    AVFormatContext *fmt_ctx;
    AVCodecContext *video_dec_ctx,*audio_dec_ctx;
    enum AVPixelFormat pix_fmt;
    int width,height;
    byte* rgba_buffer;
    byte *video_dst_data[4] = {NULL};
    int video_dst_linesize[4];
    int video_dst_bufsize;
    int video_stream_idx,audio_stream_idx;
    AVStream* video_stream,*audio_stream;
    AVFrame * rgbaFrame;
    ProductQueue<AVPacket*> *videoPackets,*audioPackets;
    std::vector<AVPacket *> allPackets;
    SwrContext *swr_context;
    AVSampleFormat out_format;
    int out_channels;

    AAudioRender* aAudioRender;

    ProductQueue<int> videoPacketIndex,audioPacketIndex;
    ProductQueue<AVFrame*> *videoFrames,*audioFrames;
    std::vector<AVFrame*> allFrames;
    byte* audioBuffer;
    int video_frame_count;
    int audio_frame_count;

    SingletonPlayer();
    void cleanResource();
    void deadClose();
    void notifyToAll();
    void demuxThreadFunc();
    void videoDecoderThreadFunc();
    void videoRenderThreadFunc();
    void audioDecoderThreadFunc();
    void audioRenderThreadFunc();
public:
    // 获取实例的静态方法
    static SingletonPlayer* getInstance() ;
    void play(const char* fileUri,int w,int h,ANativeWindow* surface);
    void end();
    void stop(bool toStop);
    void pause(bool continuePlay);
    void joinThreads();
    void changeSpeed(float factor);
    uint64_t getDuration(){
        return duration_ms;
    }
    uint64_t getPosition(){
        return position_ms;
    }
    std::chrono::milliseconds getFrameDuration(){
        return frame_duration;
    }
    void seekTo(double percentage);
    ~SingletonPlayer();
};





#endif //ANDROIDPLAYER_SINGLETONPLAYER_H
