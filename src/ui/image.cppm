module;
#include <string>
#include <memory>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Imlib2.h>  // Для работы с изображениями
#include <fstream>
#include <algorithm>

export module ui.image;

import ui.widget;
import ui.render_buffer;
import services.logger_service;
import core.ioc_container;

export class Image : public Widget {
private:
    std::string imagePath;
    Pixmap pixmap = 0;
    bool imageLoaded = false;
    int imageWidth = 0;
    int imageHeight = 0;
    Display* currentDisplay = nullptr;
    std::shared_ptr<LoggerService> logger;

public:
    Image(const std::string& widgetId, int x, int y, int w, int h, const std::string& path)
        : Widget(widgetId) {
        setPosition(x, y);
        setSize(w, h);
        imagePath = path;
        logger = std::make_shared<LoggerService>();
        logger->info("Creating Image widget with path: " + path);

        // Проверяем существование файла
        std::ifstream file(path);
        if (!file.good()) {
            logger->error("Image file does not exist or cannot be accessed: " + path);
        } else {
            logger->info("Image file exists and is accessible");
        }
    }

    ~Image() {
        if (pixmap && currentDisplay) {
            XFreePixmap(currentDisplay, pixmap);
        }
    }

    void loadImage(Display* display) {
        logger->info("loadImage called for: " + imagePath);

        if (!display) {
            logger->error("Display is null, cannot load image");
            return;
        }

        currentDisplay = display;
        logger->info("Display set, attempting to load image with Imlib2");

        // Сохраняем текущий контекст Imlib
        Imlib_Context prev_context = imlib_context_get();
        logger->info("Saved previous Imlib context");

        // Используем Imlib2 для загрузки изображения
        try {
            // Инициализируем Imlib для работы с этим дисплеем
            imlib_context_set_display(display);
            imlib_context_set_visual(DefaultVisual(display, DefaultScreen(display)));
            imlib_context_set_colormap(DefaultColormap(display, DefaultScreen(display)));
            logger->info("Imlib context initialized for display");

            logger->info("Calling imlib_load_image");
            Imlib_Image img = imlib_load_image(imagePath.c_str());
            if (!img) {
                logger->error("Failed to load image: " + imagePath);
                return;
            }
            logger->info("Image loaded successfully with Imlib2");

            logger->info("Setting image context");
            imlib_context_set_image(img);

            // Получаем размеры изображения
            imageWidth = imlib_image_get_width();
            imageHeight = imlib_image_get_height();
            logger->info("Image dimensions: " + std::to_string(imageWidth) + "x" + std::to_string(imageHeight));

            // Создаем Pixmap для X11
            logger->info("Creating X11 Pixmap");
            pixmap = XCreatePixmap(display, DefaultRootWindow(display),
                                imageWidth, imageHeight, DefaultDepth(display, DefaultScreen(display)));
            if (!pixmap) {
                logger->error("Failed to create pixmap");
                imlib_free_image();
                return;
            }
            logger->info("Pixmap created successfully");

            // Рендерим изображение в Pixmap
            logger->info("Setting drawable and rendering image");
            imlib_context_set_drawable(pixmap);
            imlib_render_image_on_drawable(0, 0);
            logger->info("Image rendered to drawable");

            // Освобождаем ресурсы Imlib
            logger->info("Freeing Imlib image resources");
            imlib_free_image();

            imageLoaded = true;
            markDirty();
            logger->info("Image loaded and marked dirty");
        } catch (const std::exception& e) {
            logger->error("Exception during image loading: " + std::string(e.what()));
        } catch (...) {
            logger->error("Unknown exception during image loading");
        }
    }

    void paintToBuffer(Display* display) override {
        logger->info("paintToBuffer called");

        if (!display) {
            logger->error("Display is null, cannot paint to buffer");
            return;
        }

        // Сохраняем указатель на дисплей для использования в деструкторе
        currentDisplay = display;

        // Если изображение еще не загружено, загружаем его
        if (!imageLoaded) {
            logger->info("Image not loaded, calling loadImage");
            loadImage(display);
        }

        if (!imageLoaded || !pixmap) {
            logger->error("Image not loaded or pixmap not created after loading attempt");
            return;
        }

        // Получаем буфер для рисования
        RenderBuffer* buffer = getBuffer();
        if (!buffer) {
            logger->error("No render buffer available");
            return;
        }
        logger->info("Render buffer obtained");

        try {
            // Используем Imlib2 для масштабирования изображения
            logger->info("Using Imlib2 for image scaling");

            // Сохраняем текущий контекст Imlib
            Imlib_Context prev_context = imlib_context_get();

            // Загружаем изображение снова для масштабирования
            Imlib_Image img = imlib_load_image(imagePath.c_str());
            if (!img) {
                logger->error("Failed to load image for scaling");
                return;
            }

            imlib_context_set_image(img);

            // Вычисляем параметры для режима "cover"
            double widgetRatio = (double)getWidth() / getHeight();
            double imageRatio = (double)imageWidth / imageHeight;

            int targetWidth = getWidth();
            int targetHeight = getHeight();

            // Масштабируем изображение с сохранением пропорций
            Imlib_Image scaled_img;
            if (imageRatio > widgetRatio) {
                // Изображение шире, чем виджет (относительно)
                int newWidth = (int)(imageHeight * widgetRatio);
                int offsetX = (imageWidth - newWidth) / 2;

                // Обрезаем и масштабируем
                scaled_img = imlib_create_cropped_scaled_image(
                    offsetX, 0, newWidth, imageHeight,
                    targetWidth, targetHeight
                );
            } else {
                // Изображение выше, чем виджет (относительно)
                int newHeight = (int)(imageWidth / widgetRatio);
                int offsetY = (imageHeight - newHeight) / 2;

                // Обрезаем и масштабируем
                scaled_img = imlib_create_cropped_scaled_image(
                    0, offsetY, imageWidth, newHeight,
                    targetWidth, targetHeight
                );
            }

            // Освобождаем исходное изображение
            imlib_free_image();

            if (!scaled_img) {
                logger->error("Failed to scale image");
                return;
            }

            // Устанавливаем масштабированное изображение как текущее
            imlib_context_set_image(scaled_img);

            // Настраиваем контекст для рендеринга
            imlib_context_set_display(display);
            imlib_context_set_visual(DefaultVisual(display, DefaultScreen(display)));
            imlib_context_set_colormap(DefaultColormap(display, DefaultScreen(display)));
            imlib_context_set_drawable(buffer->getPixmap());

            // Рендерим масштабированное изображение прямо в буфер виджета
            imlib_render_image_on_drawable(0, 0);

            // Освобождаем масштабированное изображение
            imlib_free_image();

            // Восстанавливаем предыдущий контекст Imlib
            imlib_context_free(prev_context);

            logger->info("Image scaled and rendered to buffer successfully");
        } catch (const std::exception& e) {
            logger->error("Exception during painting: " + std::string(e.what()));
        } catch (...) {
            logger->error("Unknown exception during painting");
        }
    }
};