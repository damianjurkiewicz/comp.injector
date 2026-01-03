#define WIN32_LEAN_AND_MEAN

#include <plugin.h>
#include <string>
#include <filesystem>
#include <windows.h>
#include "ini.hpp"
#include <sstream>  // Do obsługi std::stringstream
#include <cctype>   // Do obsługi std::isalpha
#include <algorithm> // Do std::all_of

// Customize these here
#define MODNAME "COMP.Injector"
#define MODNAME_EXT MODNAME ".asi"

extern CIniReader gConfig;

inline std::filesystem::path GetInjectorBasePath(const std::filesystem::path& originalPath)
{
    char modulePath[MAX_PATH] = {};
    HMODULE moduleHandle = GetModuleHandleA(MODNAME_EXT);
    if (moduleHandle == nullptr)
    {
        moduleHandle = GetModuleHandleA(MODNAME);
    }

    if (moduleHandle != nullptr && GetModuleFileNameA(moduleHandle, modulePath, MAX_PATH) != 0)
    {
        std::filesystem::path pluginDir = std::filesystem::path(modulePath).parent_path();
        return pluginDir / "reference" / originalPath.filename();
    }

    std::filesystem::path gameRoot = GAME_PATH((char*)"");
    if (gameRoot.empty())
    {
        return originalPath;
    }

    return gameRoot / "reference" / originalPath.filename();
}
