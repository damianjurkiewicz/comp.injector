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
#define MODNAME "$comp.injector"
#define MODNAME_EXT MODNAME ".asi"

extern CIniReader gConfig;

inline std::filesystem::path GetInjectorBasePath(const std::filesystem::path& originalPath)
{
    std::filesystem::path gameRoot = GAME_PATH((char*)"");
    if (gameRoot.empty())
    {
        return originalPath;
    }

    std::filesystem::path injectorRoot = gameRoot / "injector";
    return injectorRoot / originalPath.filename();
}
