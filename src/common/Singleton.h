#pragma once

#include <iostream>

template <typename T>
class Singleton
{
protected:
    Singleton() = default;
    virtual ~Singleton()
    {
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
public:
    static T &getInstance()
    {
        static T instance;
        return instance;
    }

    static T *getInstancePtr()
    {
        return &getInstance();
    }

    void printAddress()
    {
    }
};