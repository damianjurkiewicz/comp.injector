#pragma once
#include <wtypes.h>

class CompInjector
{
private:
    HINSTANCE handle;

    bool IsPluginNameValid();
    void ParseModloader();

public:
    CompInjector(HINSTANCE pluginHandle);
};
