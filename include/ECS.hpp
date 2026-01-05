#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>

using Entity = uint32_t;

class ComponentPool {
public:
    virtual ~ComponentPool() = default;
};

template<typename T>
class TypedComponentPool : public ComponentPool {
public:
    std::vector<T> components;
    std::unordered_map<Entity, size_t> entityToIndex;
};

class Registry {
public:
    Entity createEntity() {
        return m_nextEntity++;
    }

    template<typename T>
    void addComponent(Entity entity, T component) {
        auto type = std::type_index(typeid(T));
        if (m_pools.find(type) == m_pools.end()) {
            m_pools[type] = std::make_unique<TypedComponentPool<T>>();
        }
        auto pool = static_cast<TypedComponentPool<T>*>(m_pools[type].get());
        pool->entityToIndex[entity] = pool->components.size();
        pool->components.push_back(component);
    }

    template<typename T>
    T* getComponent(Entity entity) {
        auto type = std::type_index(typeid(T));
        if (m_pools.find(type) == m_pools.end()) return nullptr;
        auto pool = static_cast<TypedComponentPool<T>*>(m_pools[type].get());
        if (pool->entityToIndex.find(entity) == pool->entityToIndex.end()) return nullptr;
        return &pool->components[pool->entityToIndex[entity]];
    }

private:
    Entity m_nextEntity = 0;
    std::unordered_map<std::type_index, std::unique_ptr<ComponentPool>> m_pools;
};
