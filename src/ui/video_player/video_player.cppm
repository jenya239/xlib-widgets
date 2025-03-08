module;
#include <string>
#include <memory>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

export module ui.video_player.video_player;

import ui.widget;
import ui.render_buffer;
import services.xdisplay_service;
import services.logger_service;
import core.ioc_container;

export class VideoPlayer : public Widget {
private:
    std::string videoPath;
    bool isPlaying = false;
    XftFont* font = nullptr;
    XftColor textColor;
    std::shared_ptr<LoggerService> logger;
    Display* currentDisplay = nullptr;

public:
    VideoPlayer(const std::string& name, int x, int y, int w, int h, const std::string& path)
        : Widget(name) {
        setPosition(x, y);
        setSize(w, h);
        videoPath = path;
        logger = std::make_shared<LoggerService>();
        logger->info("Creating VideoPlayer widget with path: " + path);
    }

    ~VideoPlayer() {
        if (font) {
            XftFontClose(currentDisplay, font);
        }
    }

    void setFont(XftFont* newFont) {
        font = newFont;
        markDirty();
    }

    void paintToBuffer(Display* display) override {
        if (!display) {
            logger->error("Display is null, cannot paint video player");
            return;
        }

        currentDisplay = display;

        // Get render buffer
        RenderBuffer* buffer = getBuffer();
        if (!buffer) {
            logger->error("No render buffer available for video player");
            return;
        }

        // Draw background
        XGCValues values;
        values.foreground = WhitePixel(display, DefaultScreen(display));
        values.background = BlackPixel(display, DefaultScreen(display));

        GC gc = XCreateGC(display, buffer->getPixmap(), GCForeground | GCBackground, &values);

        // Fill background
        XFillRectangle(display, buffer->getPixmap(), gc, 0, 0, getWidth(), getHeight());

        // Draw border
        values.foreground = BlackPixel(display, DefaultScreen(display));
        XChangeGC(display, gc, GCForeground, &values);
        XDrawRectangle(display, buffer->getPixmap(), gc, 0, 0, getWidth() - 1, getHeight() - 1);

        // Draw video placeholder text
        if (font) {
            // Initialize XftDraw for text rendering
            XftDraw* draw = XftDrawCreate(display, buffer->getPixmap(),
                                         DefaultVisual(display, DefaultScreen(display)),
                                         DefaultColormap(display, DefaultScreen(display)));

            // Set text color
            XRenderColor xrcolor;
            xrcolor.red = 0;
            xrcolor.green = 0;
            xrcolor.blue = 0;
            xrcolor.alpha = 0xffff;

            XftColorAllocValue(display,
                              DefaultVisual(display, DefaultScreen(display)),
                              DefaultColormap(display, DefaultScreen(display)),
                              &xrcolor, &textColor);

            // Draw placeholder text
            std::string text = "Video: " + videoPath;
            XftDrawStringUtf8(draw, &textColor, font, 10, 30,
                             (const FcChar8*)text.c_str(), text.length());

            // Draw play/pause status
            std::string status = isPlaying ? "Playing" : "Paused";
            XftDrawStringUtf8(draw, &textColor, font, 10, 60,
                             (const FcChar8*)status.c_str(), status.length());

            // Free resources
            XftDrawDestroy(draw);
        }

        XFreeGC(display, gc);
    }

    void togglePlayback() {
        isPlaying = !isPlaying;
        markDirty();
        logger->info(isPlaying ? "Video playback started" : "Video playback paused");
    }

    bool getPlaybackStatus() const {
        return isPlaying;
    }

    std::string getVideoPath() const {
        return videoPath;
    }

    void setVideoPath(const std::string& path) {
        videoPath = path;
        markDirty();
        logger->info("Video path changed to: " + path);
    }
};