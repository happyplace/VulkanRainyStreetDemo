#pragma once

#include "GameUtils.h"

template<class T>
class Singleton
{
public:
    static void Create();
    static void Destroy();
    static T* Get();

private:
    static T* m_value;
};

template<class T>
T* Singleton<T>::m_value = nullptr;

template<class T>
void Singleton<T>::Create()
{
    GAME_ASSERT(!m_value);
    m_value = new T;
}

template<class T>
void Singleton<T>::Destroy()
{
    GAME_ASSERT(m_value);
    delete m_value;
    m_value = nullptr;
}

template<class T>
T* Singleton<T>::Get()
{
    GAME_ASSERT(m_value);
    return m_value;
}

