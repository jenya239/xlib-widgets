module;
#include <string>
#include <memory>
#include <vector>
#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

export module ui.video_player.video_decoder;

import services.logger_service;

export class VideoDecoder {
private:
    std::shared_ptr<LoggerService> logger;
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* frameRGB = nullptr;
    SwsContext* swsContext = nullptr;
    int videoStreamIndex = -1;
    uint8_t* buffer = nullptr;
    bool videoLoaded = false;
    int width = 0;
    int height = 0;
    int64_t duration = 0;

public:
    VideoDecoder() {
        logger = std::make_shared<LoggerService>();
        logger->info("VideoDecoder created");
    }

    ~VideoDecoder() {
        closeVideo();
    }

    bool loadVideo(const std::string& path) {
        // Закрываем предыдущее видео, если оно было открыто
        closeVideo();

        logger->info("Loading video: " + path);

        // Открываем файл
        formatContext = avformat_alloc_context();
        if (avformat_open_input(&formatContext, path.c_str(), nullptr, nullptr) != 0) {
            logger->error("Could not open video file: " + path);
            return false;
        }

        // Получаем информацию о потоках
        if (avformat_find_stream_info(formatContext, nullptr) < 0) {
            logger->error("Could not find stream information");
            closeVideo();
            return false;
        }

        // Находим видеопоток
        videoStreamIndex = -1;
        for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
            if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;
            }
        }

        if (videoStreamIndex == -1) {
            logger->error("Could not find video stream");
            closeVideo();
            return false;
        }

        // Получаем кодек
        const AVCodec* codec = avcodec_find_decoder(formatContext->streams[videoStreamIndex]->codecpar->codec_id);
        if (!codec) {
            logger->error("Unsupported codec");
            closeVideo();
            return false;
        }

        // Создаем контекст кодека
        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            logger->error("Could not allocate codec context");
            closeVideo();
            return false;
        }

        // Копируем параметры кодека
        if (avcodec_parameters_to_context(codecContext, formatContext->streams[videoStreamIndex]->codecpar) < 0) {
            logger->error("Could not copy codec parameters");
            closeVideo();
            return false;
        }

        // Открываем кодек
        if (avcodec_open2(codecContext, codec, nullptr) < 0) {
            logger->error("Could not open codec");
            closeVideo();
            return false;
        }

        // Получаем размеры видео
        width = codecContext->width;
        height = codecContext->height;
        duration = formatContext->duration;

        // Создаем фреймы
        frame = av_frame_alloc();
        frameRGB = av_frame_alloc();
        if (!frame || !frameRGB) {
            logger->error("Could not allocate frames");
            closeVideo();
            return false;
        }

        // Выделяем буфер для RGB данных
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
        buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

        // Настраиваем frameRGB для использования этого буфера
        av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer,
                            AV_PIX_FMT_RGB24, width, height, 1);

        // Создаем контекст для конвертации
        swsContext = sws_getContext(
            width, height, codecContext->pix_fmt,
            width, height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );

        if (!swsContext) {
            logger->error("Could not initialize conversion context");
            closeVideo();
            return false;
        }

        videoLoaded = true;
        logger->info("Video loaded successfully. Width: " + std::to_string(width) +
                    ", Height: " + std::to_string(height));
        return true;
    }

    void closeVideo() {
        videoLoaded = false;

        if (buffer) {
            av_free(buffer);
            buffer = nullptr;
        }

        if (frameRGB) {
            av_frame_free(&frameRGB);
            frameRGB = nullptr;
        }

        if (frame) {
            av_frame_free(&frame);
            frame = nullptr;
        }

        if (codecContext) {
            avcodec_free_context(&codecContext);
            codecContext = nullptr;
        }

        if (formatContext) {
            avformat_close_input(&formatContext);
            formatContext = nullptr;
        }

        if (swsContext) {
            sws_freeContext(swsContext);
            swsContext = nullptr;
        }

        videoStreamIndex = -1;
        logger->info("Video resources released");
    }

    bool isLoaded() const {
        return videoLoaded;
    }

    int getWidth() const {
        return width;
    }

    int getHeight() const {
        return height;
    }

    int64_t getDuration() const {
        return duration;
    }

    // Получить первый кадр видео
    std::vector<uint8_t> getFirstFrame() {
        if (!videoLoaded) {
            logger->error("Cannot get first frame: video not loaded");
            return {};
        }

        // Перемотка в начало
        av_seek_frame(formatContext, videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);

        AVPacket packet;
        bool frameDecoded = false;

        // Читаем пакеты, пока не декодируем кадр
        while (av_read_frame(formatContext, &packet) >= 0 && !frameDecoded) {
            if (packet.stream_index == videoStreamIndex) {
                // Отправляем пакет декодеру
                int response = avcodec_send_packet(codecContext, &packet);
                if (response < 0) {
                    logger->error("Error sending packet to decoder");
                    av_packet_unref(&packet);
                    continue;
                }

                // Получаем декодированный кадр
                response = avcodec_receive_frame(codecContext, frame);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    av_packet_unref(&packet);
                    continue;
                } else if (response < 0) {
                    logger->error("Error during decoding");
                    av_packet_unref(&packet);
                    continue;
                }

                // Конвертируем кадр в RGB формат
                sws_scale(swsContext, frame->data, frame->linesize, 0,
                         height, frameRGB->data, frameRGB->linesize);

                frameDecoded = true;
            }
            av_packet_unref(&packet);
        }

        if (!frameDecoded) {
            logger->error("Could not decode first frame");
            return {};
        }

        // Копируем данные кадра в вектор
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
        std::vector<uint8_t> frameData(buffer, buffer + numBytes);

        return frameData;
    }

    // Получить кадр по временной позиции (0.0 - 1.0)
    std::vector<uint8_t> getFrameAtPosition(float position) {
        if (!videoLoaded) {
            logger->error("Cannot get frame: video not loaded");
            return {};
        }

        // Вычисляем временную метку
        int64_t timestamp = static_cast<int64_t>(position * duration);

        // Перемотка к нужной позиции
        av_seek_frame(formatContext, videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);

        // Очищаем буферы декодера
        avcodec_flush_buffers(codecContext);

        AVPacket packet;
        bool frameDecoded = false;

        // Читаем пакеты, пока не декодируем кадр
        while (av_read_frame(formatContext, &packet) >= 0 && !frameDecoded) {
            if (packet.stream_index == videoStreamIndex) {
                // Отправляем пакет декодеру
                int response = avcodec_send_packet(codecContext, &packet);
                if (response < 0) {
                    logger->error("Error sending packet to decoder");
                    av_packet_unref(&packet);
                    continue;
                }

                // Получаем декодированный кадр
                response = avcodec_receive_frame(codecContext, frame);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    av_packet_unref(&packet);
                    continue;
                } else if (response < 0) {
                    logger->error("Error during decoding");
                    av_packet_unref(&packet);
                    continue;
                }

                // Конвертируем кадр в RGB формат
                sws_scale(swsContext, frame->data, frame->linesize, 0,
                         height, frameRGB->data, frameRGB->linesize);

                frameDecoded = true;
            }
            av_packet_unref(&packet);
        }

        if (!frameDecoded) {
            logger->error("Could not decode frame at position " + std::to_string(position));
            return {};
        }

        // Копируем данные кадра в вектор
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
        std::vector<uint8_t> frameData(buffer, buffer + numBytes);

        return frameData;
    }
};