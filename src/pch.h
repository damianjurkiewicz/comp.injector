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
    std::error_code ec;
    std::filesystem::path relativePath = originalPath.is_absolute()
        ? std::filesystem::relative(originalPath, gameRoot, ec)
        : originalPath;
    if (ec || relativePath.empty())
    {
        return originalPath;
    }

    const std::string relativeStr = relativePath.generic_string();
    if (relativeStr.rfind("..", 0) == 0)
    {
        return originalPath;
    }

    return injectorRoot / relativePath;
}
