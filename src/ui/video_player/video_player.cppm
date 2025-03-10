module;
#include <vector>
#include <memory>
#include <string>
#include <algorithm> // For std::max and std::min
// X11 headers
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
// FFmpeg headers
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
// If you don't have AV_TIME_BASE defined, uncomment this:
// #define AV_TIME_BASE 1000000

export module ui.video_player.video_player;

import ui.widget;
import ui.render_buffer;
import ui.event;
import services.xdisplay_service;
import services.logger_service;
import core.ioc_container;
import ui.video_player.video_decoder;
import ui.video_player.video_renderer;

export class VideoPlayer : public Widget {
private:
    std::string videoPath;
    bool isPlaying = false;
    XftFont* font = nullptr;
    XftColor textColor;
    std::shared_ptr<LoggerService> logger;
    Display* currentDisplay = nullptr;

    // Добавляем переменные для отслеживания прогресса
    float currentPosition = 0.0f; // от 0.0 до 1.0
    int duration = 100; // Длительность видео в секундах (пока заглушка)

    // Области для элементов управления
    struct {
        int playButtonX, playButtonY;
        unsigned int playButtonWidth, playButtonHeight;

        int progressBarX, progressBarY;
        unsigned int progressBarWidth, progressBarHeight;
    } controls;

    // Добавляем декодер и рендерер
    std::unique_ptr<VideoDecoder> decoder;
    std::unique_ptr<VideoRenderer> renderer;
    std::vector<uint8_t> currentFrame;

    // Add these variables
    unsigned long lastFrameTime = 0;
    float frameRate = 30.0f; // Default frame rate
    float frameInterval = 1.0f / 30.0f; // Time between frames in seconds

public:
    VideoPlayer(const std::string& name, int x, int y, int w, int h, const std::string& path)
        : Widget(name) {
        setPosition(x, y);
        setSize(w, h);
        videoPath = path;
        logger = std::make_shared<LoggerService>();
        logger->info("Creating VideoPlayer widget with path: " + path);

        // Initialize frame rate
        frameRate = 30.0f; // Default, will be updated when video loads
        frameInterval = 1.0f / frameRate;
        lastFrameTime = 0;

        // Инициализация размеров и позиций элементов управления
        updateControlsLayout();

        // Создаем декодер и рендерер
        decoder = std::make_unique<VideoDecoder>();
        renderer = std::make_unique<VideoRenderer>();

        // Устанавливаем размер области отображения для рендерера
        renderer->setDisplaySize(w, h - 30); // Оставляем место для элементов управления

        // Если путь к видео задан, пробуем загрузить его
        if (!videoPath.empty()) {
            loadVideo(videoPath);
        }
    }

    ~VideoPlayer() {
        if (font) {
            XftFontClose(currentDisplay, font);
        }
    }

    // Modify loadVideo method to remove getFrameRate calls
    void loadVideo(const std::string& path) {
        if (decoder->loadVideo(path)) {
            // Устанавливаем размеры видео для рендерера
            renderer->setVideoSize(decoder->getWidth(), decoder->getHeight());

            // Загружаем первый кадр
            currentFrame = decoder->getFirstFrame();

            // Обновляем длительность
            duration = decoder->getDuration() / AV_TIME_BASE; // Конвертируем в секунды

            // Use a fixed frame rate since getFrameRate is not available
            frameRate = 30.0f; // Default value
            frameInterval = 1.0f / frameRate;

            markDirty();
        } else {
            logger->error("Failed to load video: " + path);
        }
    }

    // Add a method to update the video playback
    void update(unsigned long currentTime) {
        if (isPlaying && decoder && decoder->isLoaded()) {
            // Check if it's time to show the next frame
            float elapsedSeconds = (currentTime - lastFrameTime) / 1000.0f;

            if (elapsedSeconds >= frameInterval) {
                // Advance position based on elapsed time
                float positionIncrement = elapsedSeconds / duration;
                currentPosition += positionIncrement;

                // Loop back to beginning if we reach the end
                if (currentPosition >= 1.0f) {
                    currentPosition = 0.0f;
                }

                // Get the frame at the new position
                currentFrame = decoder->getFrameAtPosition(currentPosition);

                // Update last frame time
                lastFrameTime = currentTime;

                // Mark widget as dirty to trigger redraw
                markDirty();
            }
        }
    }

    void setVideoPath(const std::string& path) {
        videoPath = path;
        logger->info("Video path set to: " + videoPath);
        markDirty();
    }

    void play() {
        if (decoder->isLoaded()) {
            isPlaying = true;
            logger->info("Playing video: " + videoPath);
            markDirty();
        } else {
            logger->warning("Cannot play: no video loaded");
        }
    }

    void pause() {
        isPlaying = false;
        logger->info("Paused video: " + videoPath);
        markDirty();
    }

    void togglePlayPause() {
        if (decoder->isLoaded()) {
            isPlaying = !isPlaying;
            logger->info(isPlaying ? "Playing video" : "Paused video");
            markDirty();
        } else {
            logger->warning("Cannot toggle play/pause: no video loaded");
        }
    }

    void setSeekPosition(float pos) {
        // Ограничиваем позицию между 0.0 и 1.0
        currentPosition = std::max(0.0f, std::min(1.0f, pos));
        logger->info("Set position to: " + std::to_string(currentPosition));

        if (decoder->isLoaded()) {
            // Получаем кадр для новой позиции
            currentFrame = decoder->getFrameAtPosition(currentPosition);
        }

        markDirty();
    }

    void handleEvent(const Event& event) override {
        if (event.getType() == Event::Type::MouseDown) {
            int mouseX = event.getX();
            int mouseY = event.getY();

            // Проверка клика на кнопку воспроизведения
            if (mouseX >= controls.playButtonX && mouseX < controls.playButtonX + controls.playButtonWidth &&
                mouseY >= controls.playButtonY && mouseY < controls.playButtonY + controls.playButtonHeight) {
                togglePlayPause();
                return;
                }

            // Проверка клика на прогресс-бар
            if (mouseX >= controls.progressBarX && mouseX < controls.progressBarX + controls.progressBarWidth &&
                mouseY >= controls.progressBarY && mouseY < controls.progressBarY + controls.progressBarHeight) {
                // Вычисляем новую позицию в зависимости от места клика
                float clickPosition = static_cast<float>(mouseX - controls.progressBarX) / controls.progressBarWidth;
                setSeekPosition(clickPosition);
                return;
                }
        }

        return;
    }

    void paintToBuffer(Display* display) override {
        currentDisplay = display;

        if (!font) {
            // Инициализируем шрифт при первой отрисовке
            font = XftFontOpen(display, DefaultScreen(display),
                              XFT_FAMILY, XftTypeString, "DejaVu Sans",
                              XFT_SIZE, XftTypeDouble, 12.0,
                              NULL);

            // Инициализируем цвет текста
            XftColorAllocName(display, DefaultVisual(display, DefaultScreen(display)),
                             DefaultColormap(display, DefaultScreen(display)),
                             "#000000", &textColor);
        }

        auto buffer = getBuffer();
        if (!buffer) return;

        // Рисуем фон
        buffer->fillRectangle(0, 0, getWidth(), getHeight(), 0xEEEEEE);

        // Рисуем область предпросмотра видео
        buffer->fillRectangle(0, 0, getWidth(), getHeight() - 40, 0x333333);

        // Если у нас есть декодированный кадр, рендерим его
        if (!currentFrame.empty() && decoder && decoder->isLoaded()) {
            renderer->renderFrame(currentFrame, *buffer);
        } else {
            // Иначе рисуем черный фон
            if (renderer) {
                renderer->renderBlackBackground(*buffer);
            }
        }

        // Рисуем элементы управления
        drawControls(buffer);

        // Если путь к видео не задан, показываем сообщение
        if (videoPath.empty() || !decoder || !decoder->isLoaded()) {
            // Центрируем текст
            int textX = getWidth() / 2 - 80;
            int textY = getHeight() / 2;

            // Рисуем текст
            buffer->drawText(textX, textY, "No video selected", font, &textColor);
        }
    }

    // Обновляем размеры и позиции элементов управления при изменении размера виджета
    void setWidth(unsigned int newWidth) override {
        Widget::setWidth(newWidth);
        updateControlsLayout();
    }

    void setHeight(unsigned int newHeight) override {
        Widget::setHeight(newHeight);
        updateControlsLayout();
    }

    void setFont(XftFont* newFont) {
        font = newFont;
        updateControlsLayout();
        markDirty();
    }

private:
    void updateControlsLayout() {
        // Кнопка воспроизведения/паузы
        controls.playButtonWidth = 30;
        controls.playButtonHeight = 30;
        controls.playButtonX = 10;
        controls.playButtonY = getHeight() - controls.playButtonHeight - 5;

        // Полоса прогресса
        controls.progressBarHeight = 10;
        controls.progressBarX = controls.playButtonX + controls.playButtonWidth + 10;
        controls.progressBarY = getHeight() - controls.progressBarHeight - 15;
        controls.progressBarWidth = getWidth() - controls.progressBarX - 10;

        // Обновляем размер области отображения для рендерера
        if (renderer) {
            renderer->setDisplaySize(getWidth(), getHeight() - 40);
        }
    }

    void drawControls(RenderBuffer* buffer) {
        // Рисуем кнопку воспроизведения/паузы
        buffer->fillRectangle(controls.playButtonX, controls.playButtonY,
                             controls.playButtonWidth, controls.playButtonHeight,
                             0x4488FF);

        // Рисуем иконку в зависимости от состояния
        if (isPlaying) {
            // Рисуем иконку паузы (две вертикальные линии)
            int pauseX1 = controls.playButtonX + controls.playButtonWidth / 3 - 2;
            int pauseX2 = controls.playButtonX + 2 * controls.playButtonWidth / 3 - 2;
            int pauseY1 = controls.playButtonY + controls.playButtonHeight / 4;
            int pauseY2 = controls.playButtonY + 3 * controls.playButtonHeight / 4;

            buffer->fillRectangle(pauseX1, pauseY1, 4, pauseY2 - pauseY1, 0xFFFFFF);
            buffer->fillRectangle(pauseX2, pauseY1, 4, pauseY2 - pauseY1, 0xFFFFFF);
        } else {
            // Рисуем иконку воспроизведения (треугольник)
            int playX = controls.playButtonX + controls.playButtonWidth / 4;
            int playY = controls.playButtonY + controls.playButtonHeight / 4;

            // Draw an actual triangle for play button with proper type casting
            int triangleX[3] = {
                playX,
                playX,
                playX + static_cast<int>(controls.playButtonWidth / 2)
            };

            int triangleY[3] = {
                playY,
                playY + static_cast<int>(controls.playButtonHeight / 2),
                playY + static_cast<int>(controls.playButtonHeight / 4)
            };

            buffer->fillPolygon(triangleX, triangleY, 3, 0xFFFFFF);
        }

        // Рисуем фон полосы прогресса
        buffer->fillRectangle(controls.progressBarX, controls.progressBarY,
                             controls.progressBarWidth, controls.progressBarHeight,
                             0xCCCCCC);

        // Рисуем заполненную часть полосы прогресса
        unsigned int filledWidth = static_cast<unsigned int>(currentPosition * controls.progressBarWidth);
        buffer->fillRectangle(controls.progressBarX, controls.progressBarY,
                             filledWidth, controls.progressBarHeight,
                             0x4488FF);
    }
};