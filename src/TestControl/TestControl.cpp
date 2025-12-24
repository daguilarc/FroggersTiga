#include "../common/Include.hpp"
#include "../common/App.hpp"
#include <tuple>
#include <cstdio>

using namespace daisy;

struct TestControl
{
    Page* m_page[2];

    void Config(PageManager* pageManager)
    {
        m_page[0] = pageManager->AddPage();
        m_page[1] = pageManager->AddPage();

        for (size_t i = 0; i < ::Parameter::x_numParameters; i++)
        {
            char buf[3];
            buf[0] = 'S';
            buf[1] = '0' + i;
            buf[2] = '\0';
            m_page[0]->InitParam(buf, i, static_cast<float>(i) / 8.0f);
            buf[0] = 'T';
            m_page[1]->InitParam(buf, i, static_cast<float>(i) / 8.0f);
        }
    }

    void Process(AudioHandle::InputBuffer& in, AudioHandle::OutputBuffer& out, size_t size)
    {
        for (size_t i = 0; i < size; i++)
        {
            out[0][i] = in[0][i];
            out[1][i] = in[1][i];
        }
    }
};

int main(void)
{
    App<TestControl> app;
    app.LetsFuckingDoThisShit();
    return 0;
}