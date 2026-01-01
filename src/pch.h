#define WIN32_LEAN_AND_MEAN

#include <plugin.h>
#include <string>
#include <filesystem>
#include <windows.h>
#include "ini.hpp"

// Customize these here
#define MODNAME "$comp.injector"
#define MODNAME_EXT MODNAME ".asi"

extern CIniReader gConfig;
