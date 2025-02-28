module;
#include <unordered_map>
#include <memory>
#include <string>
#include <typeinfo>
#include <functional>
#include <stdexcept>

export module core.ioc_container;

export class IoC {
private:
    struct TypeInfoHash {
        std::size_t operator()(const std::type_info* ti) const {
            return ti->hash_code();
        }
    };

    struct TypeInfoEqual {
        bool operator()(const std::type_info* lhs, const std::type_info* rhs) const {
            return *lhs == *rhs;
        }
    };

    std::unordered_map<const std::type_info*, std::shared_ptr<void>, TypeInfoHash, TypeInfoEqual> instances;
    std::unordered_map<const std::type_info*, std::function<std::shared_ptr<void>()>, TypeInfoHash, TypeInfoEqual> factories;

public:
    // Register a singleton instance
    template<typename T>
    void registerSingleton(std::shared_ptr<T> instance) {
        instances[&typeid(T)] = std::static_pointer_cast<void>(instance);
    }

    // Register a factory for creating instances
    template<typename T>
    void registerFactory(std::function<std::shared_ptr<T>()> factory) {
        factories[&typeid(T)] = [factory]() {
            return std::static_pointer_cast<void>(factory());
        };
    }

    // Get or create an instance
    template<typename T>
    std::shared_ptr<T> resolve() {
        const std::type_info* typeId = &typeid(T);
        
        // Check if we have a singleton instance
        auto it = instances.find(typeId);
        if (it != instances.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        
        // Check if we have a factory
        auto factoryIt = factories.find(typeId);
        if (factoryIt != factories.end()) {
            auto instance = std::static_pointer_cast<T>(factoryIt->second());
            return instance;
        }
        
        throw std::runtime_error("Type not registered: " + std::string(typeId->name()));
    }

    // Singleton pattern for the container itself
    static IoC& getInstance() {
        static IoC instance;
        return instance;
    }
};