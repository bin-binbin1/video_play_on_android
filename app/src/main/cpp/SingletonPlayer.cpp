#include "SingletonPlayer.h"



SingletonPlayer::SingletonPlayer() :
                                     videoPacketIndex(maxPacketsQueueSize),
                                     audioPacketIndex(maxPacketsQueueSize),
                                     allPackets(totalPacketNum),
                                     allFrames(totalFrameNum),
                                     frame_duration(1000 / frames_per_second) {

    fmt_ctx = NULL;
    sws_ctx = NULL;

    video_stream_idx = -1;
    audio_stream_idx = -1;

    video_frame_count = 0;
    audio_frame_count = 0;
    video_stream = NULL;
    audio_stream = NULL;
    pauseAV = true;

    instance = this;
    duration_ms = 1;
    position_ms = 0;
    toThePlace=false;
    for (int i = 0; i < totalPacketNum; i++) {
        allPackets[i] = av_packet_alloc();
        if (allPackets[i] == NULL) {
            LOGD("aaa", "allocate faild");
        }
    }
    for (int i = 0; i < totalFrameNum; i++) {
        allFrames[i] = av_frame_alloc();
        if (allFrames[i] == NULL) {
            LOGD("bbb", "allocate failed");
        }
    }
}

SingletonPlayer *SingletonPlayer::instance = nullptr;

int channels;
int audioBufferSize;
std::chrono::milliseconds t(1000/50);
int callBack(AAudioStream* stream, void* userData, void* audioData, int32_t numFrames) {
    byte* data = static_cast<byte*>(userData);
    size_t size=numFrames*sizeof(int16_t)*2;
    LOGD("callback","%d",audioBufferSize);
            memcpy(audioData, data, size);
//    std::this_thread::sleep_for(t);
//    std::this_thread::sleep_for(SingletonPlayer::getInstance()->getFrameDuration());
    return AAUDIO_CALLBACK_RESULT_CONTINUE; // 返回0表示继续调用回调函数
}

void SingletonPlayer::play(const char *fileUri, int w, int h, ANativeWindow *surface) {
    if(surface!= nullptr)
        anwRender = new ANWRender(surface);
    position_ms=0;
    file = fileUri;
    width = w;
    height = h;
    if (avformat_open_input(&fmt_ctx, file, NULL, NULL) < 0) {
        LOGD("wuwu", "open file failed, uri:%s", file);
        return;
    }
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        LOGD("wuwu", "could not find stream information");
        return;
    }
    duration_ms = (double )fmt_ctx->duration*1000/AV_TIME_BASE;

    for (int index = 0; index < fmt_ctx->nb_streams; index++) {
        if (fmt_ctx->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = index;
            video_stream=fmt_ctx->streams[index];
            break;
        }
    }
    AVCodec *vCodec = avcodec_find_decoder(fmt_ctx->streams[video_stream_idx]->codecpar->codec_id);
    if (vCodec == NULL) {
        LOGD("wuwu", "codec not found");
        cleanResource();
        return;
    }
    video_dec_ctx = avcodec_alloc_context3(vCodec);
    avcodec_parameters_to_context(video_dec_ctx, fmt_ctx->streams[video_stream_idx]->codecpar);
    if (avcodec_open2(video_dec_ctx, vCodec, NULL) < 0) {
        LOGD("wuwu", "could not open codec");
        cleanResource();
        return;
    }
    width = video_dec_ctx->width;
    height = video_dec_ctx->height;
    pix_fmt = video_dec_ctx->pix_fmt;
    int ret = av_image_alloc(video_dst_data, video_dst_linesize,
                             width, height, pix_fmt, 1);
    if (ret < 0) {
        LOGD("wuwu", "can not allocate raw video buffer");
        cleanResource();
        return;
    }
    video_dst_bufsize = ret;

    anwRender->init(width, height);

    for (int index = 0; index < fmt_ctx->nb_streams; index++) {
        if (fmt_ctx->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = index;
            audio_stream=fmt_ctx->streams[index];
            break;
        }
    }

    if (audio_stream_idx == -1) {
        LOGD("wuwu", "No audio stream found");
    }
    AVCodec *aCodec = avcodec_find_decoder(fmt_ctx->streams[audio_stream_idx]->codecpar->codec_id);
    if (aCodec == NULL) {
        LOGD("wuwu", "Audio codec not found");
    } else {
        audio_dec_ctx = avcodec_alloc_context3(aCodec);
        avcodec_parameters_to_context(audio_dec_ctx, fmt_ctx->streams[audio_stream_idx]->codecpar);
        if (avcodec_open2(audio_dec_ctx, aCodec, NULL) < 0) {
            LOGD("wuwu", "Could not open audio codec");
            cleanResource();
            return;
        }
    }
    swr_context=swr_alloc();
    aAudioRender=new AAudioRender();
//    aAudioRender->configure(audio_dec_ctx->sample_rate,audio_dec_ctx->channels,audio_dec_ctx->fmt);
    channels=audio_dec_ctx->channels;
    audioBuffer =(byte*) av_malloc(44100*2);
    audioBufferSize=1764;

    aAudioRender->setCallback(callBack, audioBuffer);



    sws_ctx = sws_getContext(w, h, pix_fmt,
                             w, h, AV_PIX_FMT_RGBA,
                             SWS_BICUBIC, NULL, NULL, NULL);
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    // 输出采样位数 16位
    out_format = AV_SAMPLE_FMT_S16;
    // 输出的采样率必须与输入相同
    int out_sample_rate = audio_dec_ctx->sample_rate;
    //swr_alloc_set_opts 将PCM源文件的采样格式转换为自己希望的采样格式
    swr_alloc_set_opts(swr_context,
                       out_channel_layout, out_format, out_sample_rate,
                       audio_dec_ctx->channel_layout, audio_dec_ctx->sample_fmt, audio_dec_ctx->sample_rate,
                       0, NULL);
    swr_init(swr_context);
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);



    av_dump_format(fmt_ctx, 0, file, 0);
    if (!sws_ctx) {
        LOGD("wuwu", "sws_ctx get error");
        cleanResource();
        return;
    }
    int rgba_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
    rgba_buffer = static_cast<byte *>(av_malloc(rgba_buffer_size));
    rgbaFrame = av_frame_alloc();
    av_image_fill_arrays(rgbaFrame->data, rgbaFrame->linesize, rgba_buffer,
                         AV_PIX_FMT_RGBA, width, height, 1);

    videoFrames=new ProductQueue<AVFrame*>(maxFramesQueueSize);
    audioFrames=new ProductQueue<AVFrame*>(maxFramesQueueSize);
    videoPackets=new ProductQueue<AVPacket*>(maxPacketsQueueSize);
    audioPackets=new ProductQueue<AVPacket*>(maxPacketsQueueSize);
    stopThreads= false;
    demuxThread = std::thread(&SingletonPlayer::demuxThreadFunc, this);
    videoDecoderThread = std::thread(&SingletonPlayer::videoDecoderThreadFunc, this);
    videoRenderThread = std::thread(&SingletonPlayer::videoRenderThreadFunc, this);
    audioDecoderThread = std::thread(&SingletonPlayer::audioDecoderThreadFunc, this);
    aAudioRender->start();
//    audioRenderThread = std::thread(&SingletonPlayer::audioRenderThreadFunc, this);
    notifyToAll();
}

void SingletonPlayer::stop(bool toStop) {
    pauseAV=toStop;
//    aAudioRender->pause(toStop);
}

void SingletonPlayer::joinThreads() {
    if (demuxThread.joinable()) demuxThread.detach();
    if (videoDecoderThread.joinable()) videoDecoderThread.detach();
    if (videoRenderThread.joinable()) videoRenderThread.detach();
    if (audioDecoderThread.joinable()) audioDecoderThread.detach();
//    if (audioRenderThread.joinable()) audioRenderThread.join();
}
void SingletonPlayer::demuxThreadFunc() {

    int ret = 0;
    int circleIndex = 0;
    AVPacket *currentPacket=av_packet_alloc();
    if(toThePlace){
        while(av_read_frame(fmt_ctx,currentPacket)>=0){
            if(currentPacket->stream_index==video_stream_idx){
                ret= avcodec_send_packet(video_dec_ctx,currentPacket);
                if(ret<0){
                    LOGD("wuwu","error in send package while tracing frame");
                    continue;
                }
                AVFrame *temp=av_frame_alloc();
                while(ret>=0){
                    ret= avcodec_receive_frame(video_dec_ctx,temp);
                    if(ret<0){
                        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                            break;
                        LOGD("wuwu", "error during video decoding");
                        deadClose();
                        break;
                    }
                    position_ms += frame_duration.count();
                    if(position_ms>=target){
                        if (temp->width != width || temp->height != height ||
                            temp->format != pix_fmt) {
                            LOGD("wuwu", "width, height, or pixel format changed");
                            continue;

                            deadClose();
                            return;
                        }
                        sws_scale(sws_ctx, temp->data,temp->linesize, 0, height, rgbaFrame->data,
                                  rgbaFrame->linesize);
                        anwRender->render(rgba_buffer);
                        std::this_thread::sleep_for(frame_duration);
                        LOGD("videoPlayer", "now is %d:", position_ms);
                    }
                }
                av_frame_free(&temp);

            }
            av_packet_unref(currentPacket);
        }
    }
    while (av_read_frame(fmt_ctx, currentPacket) >= 0 &&
           !stopThreads) {
        LOGD("demux", "now is %d:", circleIndex);
//        currentPacket=allPackets[circleIndex];
        if (currentPacket->stream_index == video_stream_idx) {
            LOGD("demux", "video");
            videoPackets->push(currentPacket);

//            videoDecoderThreadFunc();

        } else if (currentPacket->stream_index == audio_stream_idx) {
            LOGD("demux", "audio");
//            av_packet_unref(currentPacket);
            audioPackets->push(currentPacket);
        }
        currentPacket=av_packet_alloc();
        circleIndex=(circleIndex+1)%(totalPacketNum);
    }
}
void SingletonPlayer::videoDecoderThreadFunc() {
    AVPacket *currentPacket = NULL;
    int ret;
    int circleIndex = 0;
    AVFrame *videoFrame = NULL;
    while (!stopThreads) {
        LOGD("video decoder", "now is %d:", circleIndex);
        currentPacket = videoPackets->pop();
        ret = avcodec_send_packet(video_dec_ctx, currentPacket);
        if (ret < 0) {
            LOGD("wuwu", "error submit a packet for decoding in video");
            AVERROR(EAGAIN);
//            deadClose();
            continue;
        }
        videoFrame = allFrames[circleIndex];
        circleIndex = (circleIndex + 1) % maxFramesQueueSize;
        while (ret >= 0) {
            ret = avcodec_receive_frame(video_dec_ctx, videoFrame);
            if (ret < 0) {
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    break;
                LOGD("wuwu", "error during video decoding");
                deadClose();
                break;
            }

            videoFrames->push(videoFrame);
            position_ms += frame_duration.count();
            std::this_thread::sleep_for(frame_duration);
            static int times=0;
            LOGD("aaa","produce %d, %p,%d",times++,videoFrame,videoFrame->width);
//            videoRenderThreadFunc();
        }

        av_packet_unref(currentPacket);

    }

}

void SingletonPlayer::videoRenderThreadFunc() {
    AVFrame *videoFrame = NULL;
    while (!stopThreads) {
        while (pauseAV) {
            std::this_thread::sleep_for(frame_duration);
        }
        videoFrame = videoFrames->pop();
        static int times=0;
        LOGD("aaa", "reduce %d, %p,%d", times, videoFrame,videoFrame->width);
        if (videoFrame->width != width || videoFrame->height != height ||
            videoFrame->format != pix_fmt) {
            LOGD("wuwu", "width, height, or pixel format changed");
            continue;

            deadClose();
            return;
        }
        sws_scale(sws_ctx, videoFrame->data, videoFrame->linesize, 0, height, rgbaFrame->data,
                  rgbaFrame->linesize);
        anwRender->render(rgba_buffer);
        LOGD("videoPlayer", "now is %d:", position_ms);


//        av_frame_unref(videoFrame);
    }
}



void SingletonPlayer::audioDecoderThreadFunc() {

    AVPacket *currentPacket = NULL;
    int ret = 0;
    int circleIndex = 0;
    AVFrame *audioFrame;
    // 实现音频解码相关逻辑
    while (!stopThreads) {
        LOGD("audio decoder", "now is %d:", circleIndex);
        currentPacket = audioPackets->pop();
        ret = avcodec_send_packet(audio_dec_ctx, currentPacket);
        if (ret < 0) {
            continue;
            LOGD("wuwu", "error submit a packet for decoding in audio");
            deadClose();
            break;
        }
        // video -> 0-99     audio -> 100-199
        audioFrame = av_frame_alloc();
        circleIndex = (circleIndex + 1) % maxFramesQueueSize;
        while (ret >= 0) {
            ret = avcodec_receive_frame(audio_dec_ctx, audioFrame);
            if (ret < 0) {
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    break;
                LOGD("wuwu", "error during audio decoding");
                deadClose();
                break;
            }
            size_t unpadded_linesize=audioFrame->nb_samples * av_get_bytes_per_sample(
                    static_cast<AVSampleFormat>(audioFrame->format));

            static int times=0;
            LOGD("audio","%d",times++);
            swr_convert(swr_context,&audioBuffer,44100*2,
                        (const byte**)audioFrame->extended_data,audioFrame->nb_samples);
            while (pauseAV) {
                std::this_thread::sleep_for(frame_duration);
            }
            std::this_thread::sleep_for(frame_duration);
        }

        av_packet_unref(currentPacket);
    }
}

void SingletonPlayer::audioRenderThreadFunc() {
    AVFrame *audioFrame = NULL;
    while (!stopThreads) {
        while (pauseAV) {
            std::this_thread::sleep_for(frame_duration);
        }
        LOGD("audio player", "now is %d:", position_ms);
        audioFrame = audioFrames->pop();

        std::this_thread::sleep_for(frame_duration);
        av_frame_unref(audioFrame);
    }
}

void SingletonPlayer::cleanResource() {
    stopThreads=true;
    joinThreads();
//    avcodec_free_context(&video_dec_ctx);
//    avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    sws_freeContext(sws_ctx);
    av_free(video_dst_data[0]);
    delete aAudioRender;
    delete audioPackets;
    delete audioFrames;
    delete videoPackets;
    delete videoFrames;
}

SingletonPlayer::~SingletonPlayer() {
    joinThreads();
    cleanResource();
    delete anwRender;
    for (auto aPacket: allPackets) {
        av_packet_free(&aPacket);
    }
    for (auto aFrame: allFrames) {
        av_frame_free(&aFrame);
    }
    instance = nullptr;
}

void SingletonPlayer::pause(bool continuePlay) {
    if (continuePlay) {
        pauseAV = false;
        notifyToAll();
    } else {
        pauseAV = true;
    }

}

void SingletonPlayer::deadClose() {
    delete instance;
}

void SingletonPlayer::changeSpeed(float factor) {
    frame_duration = std::chrono::milliseconds(static_cast<int>( 1000 / (25 * factor)));
}

SingletonPlayer *SingletonPlayer::getInstance() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        instance = new SingletonPlayer();
    });
    return instance;
}

void SingletonPlayer::notifyToAll() {
//    audioDecoderCV.notify_all();
//    audioRenderCV.notify_all();
//    videoDecoderCV.notify_all();
//    videoRenderCV.notify_all();
//    demuxCV.notify_all();
    pauseAV = false;
}

void SingletonPlayer::end() {
    pauseAV=true;
//    aAudioRender->pause(true);
}

void SingletonPlayer::seekTo(double percentage) {

    if(percentage<(double)position_ms/duration_ms){
//        cleanResource();
        play(file,width,height, nullptr);
    }
    target=duration_ms*percentage;
    toThePlace=true;
}
