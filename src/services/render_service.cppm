module;
#include <X11/Xlib.h>
#include <iostream>
#include <vector>

export module services.render_service;

import ui.widget;
import ui.render_buffer;

export class RenderService {
public:
	// Repaint all dirty widgets
	static void repaintDirtyWidgets(Display* display, Window window, GC gc, Widget* rootWidget) {
		if (!rootWidget) return;

		// First render to buffers
		rootWidget->render(display, window);

		// Then paint to window
		rootWidget->paint(display, window, gc);
	}

	// Find all dirty widgets in the hierarchy
	static std::vector<Widget*> findDirtyWidgets(Widget* rootWidget) {
		std::vector<Widget*> dirtyWidgets;
		collectDirtyWidgets(rootWidget, dirtyWidgets);
		return dirtyWidgets;
	}

private:
	// Helper method to collect dirty widgets
	static void collectDirtyWidgets(Widget* widget, std::vector<Widget*>& dirtyWidgets) {
		if (!widget) return;

		if (widget->needsRepaint()) {
			dirtyWidgets.push_back(widget);
		}

		// Check children
		for (auto* child : widget->getChildren()) {
			collectDirtyWidgets(child, dirtyWidgets);
		}
	}
};