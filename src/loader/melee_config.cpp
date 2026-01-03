#include "pch.h"
#include "melee_config.h"
#include "logger.h"
#include <unordered_set>

CFLAMeleeConfigLoader FLAMeleeConfigLoader;

namespace
{
    const char* kLogPrefix = "MELEE_CONFIG";
    const char* kMarker = "; comp.injector added gtasa_melee_config";

    std::filesystem::path GetBasePathFromInjector(const std::filesystem::path& settingsPath)
    {
        return GetInjectorBasePath(settingsPath);
    }

    bool HasMarker(const std::string& settingsPath)
    {
        std::ifstream in(settingsPath);
        if (!in.is_open())
        {
            return false;
        }

        std::string line;
        while (getline(in, line))
        {
            if (line.find(kMarker) != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }
}

void CFLAMeleeConfigLoader::UpdateMeleeConfigFile()
{
    std::filesystem::path settingsPath = GAME_PATH((char*)"data/gtasa_melee_config.dat");
    std::filesystem::path settingsPathTemp = settingsPath;
    settingsPathTemp += ".tmp";
    std::filesystem::path basePath = GetBasePathFromInjector(settingsPath);
    auto isCommentOrEmpty = [](const std::string &value)
        {
            const auto firstNonWhitespace = value.find_first_not_of(" \t\r\n");
            if (firstNonWhitespace == std::string::npos)
            {
                return true;
            }

            const std::string_view trimmed(value.c_str() + firstNonWhitespace, value.size() - firstNonWhitespace);

            return trimmed.starts_with(";") || trimmed.starts_with("#") || trimmed.starts_with("//");
        };

    if (!std::filesystem::exists(basePath))
    {
        Logger.Log(std::string(kLogPrefix) + ": base file not found at " + basePath.string());
        return;
    }

    if (store.empty())
    {
        std::ifstream in(basePath, std::ios::binary);
        std::ofstream out(settingsPathTemp, std::ios::binary | std::ios::trunc);
        if (!in.is_open() || !out.is_open())
        {
            return;
        }

        out << in.rdbuf();
        in.close();
        out.close();

        std::filesystem::remove(settingsPath);
        std::filesystem::rename(settingsPathTemp, settingsPath);
        Logger.Log(std::string(kLogPrefix) + ": refreshed " + settingsPath.string());
        return;
    }

    std::unordered_set<std::string> writtenLines;
    std::unordered_set<std::string> existingLines;

    std::ifstream in(basePath);
    std::ofstream out(settingsPathTemp);

    if (in.is_open() && out.is_open())
    {
        std::string line;
        bool ignoreLines = false;
        while (getline(in, line))
        {
            if (line.find(kMarker) != std::string::npos)
            {
                ignoreLines = true;
                continue;
            }

            if (ignoreLines)
            {
                continue;
            }

            if (isCommentOrEmpty(line))
            {
                out << line << "\n";
                continue;
            }

            out << line << "\n";
            existingLines.insert(line);
        }

        out << kMarker << "\n";

        for (const auto &e : store)
        {
            if (existingLines.count(e) > 0)
            {
                continue;
            }

            if (writtenLines.insert(e).second)
            {
                out << e << "\n";
            }
        }

        in.close();
        out.close();

        std::filesystem::remove(settingsPath);
        std::filesystem::rename(settingsPathTemp, settingsPath);
        Logger.Log(std::string(kLogPrefix) + ": updated " + settingsPath.string());
    }
    else
    {
        if (in.is_open()) in.close();
        if (out.is_open()) out.close();
    }
}

void CFLAMeleeConfigLoader::Process()
{
    if (store.empty() && !HasMarker(GAME_PATH((char*)"data/gtasa_melee_config.dat")))
    {
        Logger.Log(std::string(kLogPrefix) + ": no entries and no marker, skipping.");
        return;
    }

    Logger.Log(std::string(kLogPrefix) + ": processing melee config.");
    UpdateMeleeConfigFile();
}

void CFLAMeleeConfigLoader::AddLine(const std::string &line)
{
    store.push_back(line);
}

void CFLAMeleeConfigLoader::Parse(const std::string &line)
{
    int index = 0;
    char name[256] = {};

    int count = sscanf(line.c_str(), "%d %255s", &index, name);

    if (count == 2 && index > 4 && strnlen(name, sizeof(name)) > 0)
    {
        store.push_back(line);
    }
}
