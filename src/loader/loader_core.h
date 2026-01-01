#pragma once
#include <wtypes.h>

class FastLoader
{
private:
    HINSTANCE handle;

    bool IsPluginNameValid();
    void ParseModloader();
    void HandleVanillaDataFiles(); 

public:
    FastLoader(HINSTANCE pluginHandle);
};

