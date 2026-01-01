#include "pch.h"
#include "weapon_config.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <unordered_set>

CFLAWeaponConfigLoader FLAWeaponConfigLoader;

namespace {
std::string ToLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string TrimCopy(const std::string &value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

bool IsEndMarker(const std::string &line) {
    std::string lowered = ToLowerCopy(TrimCopy(line));
    return lowered == "end" || lowered == "the end" || lowered == ";the end";
}
}

void CFLAWeaponConfigLoader::UpdateWeaponConfigFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/gtasa_weapon_config.dat");
    std::string settingsPathTemp = settingsPath + ".bak";
    auto isCommentOrEmpty = [](const std::string &value)
        {
            const auto start = value.find_first_not_of(" \t\r\n");
            if (start == std::string::npos)
            {
                return true;
            }

            const std::string_view trimmed(value.c_str() + start, value.size() - start);

            return trimmed.starts_with(";") || trimmed.starts_with("#") || trimmed.starts_with("//");
        };

    if (!std::filesystem::exists(settingsPath))
    {
        return;
    }

    std::unordered_set<std::string> linesToAdd(store.begin(), store.end());
    std::unordered_set<std::string> writtenLines;

    std::ifstream in(settingsPath);
    std::ofstream out(settingsPathTemp);

    if (in.is_open() && out.is_open())
    {
        std::string line;
        bool ignoreLines = false;
        bool foundEndMarker = false;
        std::string endMarker;
        const std::string marker = "; comp.injector added weapons";

        while (getline(in, line))
        {
            if (line.find(marker) != std::string::npos)
            {
                ignoreLines = true;
                continue;
            }

            if (IsEndMarker(line))
            {
                foundEndMarker = true;
                endMarker = line;
                break;
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

            if (!linesToAdd.count(line))
            {
                continue;
            }

            if (writtenLines.insert(line).second)
            {
                out << line << "\n";
            }
        }

        out << marker << "\n";

        for (auto &e : store)
        {
            if (writtenLines.insert(e).second)
            {
                out << e << "\n";
            }
        }

        if (foundEndMarker)
        {
            out << endMarker << "\n";
        }
        else
        {
            out << ";the end\n";
        }

        in.close();
        out.close();

        std::filesystem::remove(settingsPath);
        std::filesystem::rename(settingsPathTemp, settingsPath);
    }
    else
    {
        if (in.is_open()) in.close();
        if (out.is_open()) out.close();
    }
}

void CFLAWeaponConfigLoader::Process()
{
    if (!store.empty())
    {
        UpdateWeaponConfigFile();
    }
}

void CFLAWeaponConfigLoader::AddLine(const std::string &line)
{
    store.push_back(line);
}

void CFLAWeaponConfigLoader::Parse(const std::string &line)
{
    int index = 0;
    char name[256] = {};
    int ammoClip = 0;
    int damage = 0;
    int accuracy = 0;
    int flags = 0;
    int animGroup = 0;
    int modelId1 = 0;
    int modelId2 = 0;
    float range = 0.0f;

    int count = sscanf(line.c_str(), "%d %255s %d %d %d %d %d %d %d %f",
        &index,
        name,
        &ammoClip,
        &damage,
        &accuracy,
        &flags,
        &animGroup,
        &modelId1,
        &modelId2,
        &range);

    if (count == 10 && strnlen(name, sizeof(name)) > 0)
    {
        store.push_back(line);
    }
}
