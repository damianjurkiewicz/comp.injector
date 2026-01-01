#include "pch.h"
#include "melee_config.h"
#include <unordered_set>

CFLAMeleeConfigLoader FLAMeleeConfigLoader;

void CFLAMeleeConfigLoader::UpdateMeleeConfigFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/gtasa_melee_config.dat");
    std::string settingsPathTemp = settingsPath + ".bak";

    if (!std::filesystem::exists(settingsPath))
    {
        return;
    }

    std::unordered_set<std::string> linesToAdd(store.begin(), store.end());

    std::ifstream in(settingsPath);
    std::ofstream out(settingsPathTemp);

    if (in.is_open() && out.is_open())
    {
        std::string line;
        bool ignoreLines = false;
        const std::string marker = "; comp.injector added gtasa_melee_config";

        while (getline(in, line))
        {
            if (line.find(marker) != std::string::npos)
            {
                ignoreLines = true;
                continue;
            }

            if (ignoreLines)
            {
                continue;
            }

            if (linesToAdd.count(line))
            {
                continue;
            }

            out << line << "\n";
        }

        out << marker << "\n";

        for (const auto &e : store)
        {
            out << e << "\n";
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

void CFLAMeleeConfigLoader::Process()
{
    if (!store.empty())
    {
        UpdateMeleeConfigFile();
    }
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
