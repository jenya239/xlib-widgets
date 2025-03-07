module;
#include <string>
#include <vector>
#include <memory>

export module state.app_signals;

import state.signal;

// Типы данных для сигналов
export struct FileSelectedPayload {
	std::string filePath;
	std::string fileName;
	bool isDirectory;
};

export struct TextChangedPayload {
	std::string text;
	std::string widgetId;
};

export struct ButtonClickedPayload {
	std::string buttonId;
};

// Функции-обёртки для получения сигналов (синглтоны)
export std::shared_ptr<Signal<FileSelectedPayload>> getFileSelectedSignal() {
	static std::shared_ptr<Signal<FileSelectedPayload>> signal =
			std::make_shared<Signal<FileSelectedPayload>>();
	static bool initialized = false;
	if (!initialized) {
		SignalService::registerSignal(signal);
		initialized = true;
	}
	return signal;
}

export std::shared_ptr<Signal<TextChangedPayload>> getTextChangedSignal() {
	static std::shared_ptr<Signal<TextChangedPayload>> signal =
			std::make_shared<Signal<TextChangedPayload>>();
	static bool initialized = false;
	if (!initialized) {
		SignalService::registerSignal(signal);
		initialized = true;
	}
	return signal;
}

export std::shared_ptr<Signal<ButtonClickedPayload>> getButtonClickedSignal() {
	static std::shared_ptr<Signal<ButtonClickedPayload>> signal =
			std::make_shared<Signal<ButtonClickedPayload>>();
	static bool initialized = false;
	if (!initialized) {
		SignalService::registerSignal(signal);
		initialized = true;
	}
	return signal;
}