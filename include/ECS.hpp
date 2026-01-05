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
    std::vector<Entity> entities;
    std::unordered_map<Entity, size_t> entityToIndex;
};

class Registry {
public:
    Entity createEntity() {
        Entity e = m_nextEntity++;
        m_entities.push_back(e);
        return e;
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
        pool->entities.push_back(entity);
    }

    template<typename T>
    T* getComponent(Entity entity) {
        auto type = std::type_index(typeid(T));
        if (m_pools.find(type) == m_pools.end()) return nullptr;
        auto pool = static_cast<TypedComponentPool<T>*>(m_pools[type].get());
        if (pool->entityToIndex.find(entity) == pool->entityToIndex.end()) return nullptr;
        return &pool->components[pool->entityToIndex[entity]];
    }

    template<typename T>
    TypedComponentPool<T>* getPool() {
        auto type = std::type_index(typeid(T));
        if (m_pools.find(type) == m_pools.end()) return nullptr;
        return static_cast<TypedComponentPool<T>*>(m_pools[type].get());
    }

    const std::vector<Entity>& view() const {
        return m_entities;
    }

private:
    Entity m_nextEntity = 0;
    std::vector<Entity> m_entities;
    std::unordered_map<std::type_index, std::unique_ptr<ComponentPool>> m_pools;
};
