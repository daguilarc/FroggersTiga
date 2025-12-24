#pragma once

#include "DaisyIO.hpp"

template<typename T>
struct App
{
    DaisyIO m_daisyIO;
    T m_app;
    static App* s_instance;

    void Config()
    {
        m_app.Config(&m_daisyIO.m_pageManager);
    }

    static void StaticProcess(daisy::AudioHandle::InputBuffer in, daisy::AudioHandle::OutputBuffer out, size_t size)
    {
        if (s_instance)
        {
            s_instance->Process(in, out, size);
        }
    }

    static void StaticButtonCallback(int button)
    {
        if (s_instance)
        {
            s_instance->ButtonCallback(button);
        }
    }

    void ButtonCallback(int button)
    {
        m_app.ButtonCallback(button);
    }

    void Process(daisy::AudioHandle::InputBuffer& in, daisy::AudioHandle::OutputBuffer& out, size_t size)
    {
        m_app.Process(in, out, size);
    }

    void Init()
    {
        Config();
        m_daisyIO.m_buttonCallback = StaticButtonCallback;
        s_instance = this;
        m_daisyIO.Init(StaticProcess);
    }

    void MainLoop()
    {
        m_daisyIO.MainLoop();
    }

    void LetsFuckingDoThisShit()
    {
        Init();
        MainLoop();
    }
};

template<typename T>
App<T>* App<T>::s_instance = nullptr;