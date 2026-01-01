#pragma once
#include <wtypes.h>

class CompInjector
{
private:
    HINSTANCE handle;

    bool IsPluginNameValid();
    void ParseModloader();
    void HandleVanillaDataFiles(); 

public:
    CompInjector(HINSTANCE pluginHandle);
};
