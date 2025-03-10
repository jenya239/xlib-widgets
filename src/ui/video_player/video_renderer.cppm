module;
#include <vector>
#include <memory>
#include <algorithm>

export module ui.video_player.video_renderer;

import ui.render_buffer;
import services.logger_service;

export class VideoRenderer {
private:
    std::shared_ptr<LoggerService> logger;
    int videoWidth = 0;
    int videoHeight = 0;
    int displayWidth = 0;
    int displayHeight = 0;

public:
    VideoRenderer() {
        logger = std::make_shared<LoggerService>();
        logger->info("VideoRenderer created");
    }

    void setVideoSize(int width, int height) {
        videoWidth = width;
        videoHeight = height;
        logger->info("Video size set to: " + std::to_string(width) + "x" + std::to_string(height));
    }

    void setDisplaySize(int width, int height) {
        displayWidth = width;
        displayHeight = height;
        logger->info("Display size set to: " + std::to_string(width) + "x" + std::to_string(height));
    }

    // Рендеринг кадра в буфер
    void renderFrame(const std::vector<uint8_t>& frameData, RenderBuffer& buffer) {
        if (frameData.empty() || videoWidth <= 0 || videoHeight <= 0) {
            logger->warning("Cannot render frame: invalid frame data or dimensions");
            return;
        }

        // Определяем размеры для отображения с сохранением пропорций
        int renderWidth = displayWidth;
        int renderHeight = displayHeight;

        // Если задан размер отображения, вычисляем масштабированные размеры с сохранением пропорций
        if (displayWidth > 0 && displayHeight > 0) {
            double videoAspect = static_cast<double>(videoWidth) / videoHeight;
            double displayAspect = static_cast<double>(displayWidth) / displayHeight;

            if (videoAspect > displayAspect) {
                // Видео шире, чем область отображения
                renderWidth = displayWidth;
                renderHeight = static_cast<int>(displayWidth / videoAspect);
            } else {
                // Видео выше, чем область отображения
                renderHeight = displayHeight;
                renderWidth = static_cast<int>(displayHeight * videoAspect);
            }
        }

        // Вычисляем отступы для центрирования
        int offsetX = (displayWidth - renderWidth) / 2;
        int offsetY = (displayHeight - renderHeight) / 2;

        // Рендерим кадр с масштабированием
        for (int y = 0; y < renderHeight; y++) {
            for (int x = 0; x < renderWidth; x++) {
                // Вычисляем соответствующие координаты в исходном кадре
                int srcX = static_cast<int>(x * videoWidth / renderWidth);
                int srcY = static_cast<int>(y * videoHeight / renderHeight);

                // Вычисляем индекс в массиве данных кадра (RGB формат, 3 байта на пиксель)
                int srcIndex = (srcY * videoWidth + srcX) * 3;

                // Проверяем, что индекс в пределах массива
                if (srcIndex + 2 < frameData.size()) {
                    // Получаем RGB компоненты
                    uint8_t r = frameData[srcIndex];
                    uint8_t g = frameData[srcIndex + 1];
                    uint8_t b = frameData[srcIndex + 2];

                    // Устанавливаем пиксель в буфере
                    // Преобразуем RGB в 32-битный цвет (0xRRGGBB)
                    unsigned long color = (r << 16) | (g << 8) | b;
                    buffer.fillRectangle(offsetX + x, offsetY + y, 1, 1, color);
                }
            }
        }
    }

    // Рендеринг черного фона (для случаев, когда нет кадра)
    void renderBlackBackground(RenderBuffer& buffer) {
        // Черный цвет - 0x000000
        buffer.fillRectangle(0, 0, displayWidth, displayHeight, 0x000000);
    }
};