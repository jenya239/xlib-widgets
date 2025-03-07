module;
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <any>

export module state.signal;

// Уникальный идентификатор сигнала
export class SignalId {
private:
    // Приватный конструктор гарантирует, что каждый экземпляр уникален
    SignalId() = default;

public:
    // Фабричный метод для создания уникальных идентификаторов
    static std::shared_ptr<SignalId> create() {
        return std::shared_ptr<SignalId>(new SignalId());
    }

    // Запрещаем копирование и перемещение
    SignalId(const SignalId&) = delete;
    SignalId& operator=(const SignalId&) = delete;
    SignalId(SignalId&&) = delete;
    SignalId& operator=(SignalId&&) = delete;
};

// Класс для создания и управления сигналами
export template<typename PayloadType>
class Signal {
private:
    using SignalHandler = std::function<void(const PayloadType&)>;
    std::vector<SignalHandler> handlers;
    std::shared_ptr<SignalId> id;

public:
    Signal() : id(SignalId::create()) {}

    // Получить уникальный идентификатор сигнала
    std::shared_ptr<SignalId> getId() const {
        return id;
    }

    // Отправить сигнал с данными
    void emit(const PayloadType& payload) const {
        for (const auto& handler : handlers) {
            handler(payload);
        }
    }

    // Подписаться на сигнал
    template<typename Handler>
    void connect(Handler&& handler) {
        handlers.push_back(std::forward<Handler>(handler));
    }
};

// Сервис для управления сигналами
export class SignalService {
private:
    // Хранилище для сигналов по их идентификаторам
    std::unordered_map<std::shared_ptr<SignalId>, std::any> signals;

    // Синглтон
    static SignalService& getInstance() {
        static SignalService instance;
        return instance;
    }

    SignalService() = default;

public:
    // Регистрация сигнала в сервисе
    template<typename PayloadType>
    static void registerSignal(const std::shared_ptr<Signal<PayloadType>>& signal) {
        auto& instance = getInstance();
        instance.signals[signal->getId()] = signal;
    }

    // Получить сигнал по идентификатору
    template<typename PayloadType>
    static std::shared_ptr<Signal<PayloadType>> getSignal(const std::shared_ptr<SignalId>& id) {
        auto& instance = getInstance();
        auto it = instance.signals.find(id);
        if (it != instance.signals.end()) {
            return std::any_cast<std::shared_ptr<Signal<PayloadType>>>(it->second);
        }
        return nullptr;
    }
};