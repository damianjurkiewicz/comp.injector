#include "pch.h"
#include "radar_blip_sprite_filenames.h"
#include <sstream>
#include <unordered_set>

CFLARadarBlipSpriteFilenamesLoader FLARadarBlipSpriteFilenamesLoader;

void CFLARadarBlipSpriteFilenamesLoader::UpdateRadarBlipSpriteFilenamesFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/gtasa_radarBlipSpriteFilenames.dat");
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
        const std::string marker = "; comp.injector added gtasa_radarBlipSpriteFilenames";

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

void CFLARadarBlipSpriteFilenamesLoader::Process()
{
    if (!store.empty())
    {
        UpdateRadarBlipSpriteFilenamesFile();
    }
}

void CFLARadarBlipSpriteFilenamesLoader::AddLine(const std::string &line)
{
    store.push_back(line);
}

void CFLARadarBlipSpriteFilenamesLoader::Parse(const std::string &line)
{
    std::istringstream stream(line);
    int index = 0;
    std::string name;
    std::string texture;

    if (!(stream >> index >> name >> texture))
    {
        return;
    }

    const bool isNull = name == "NULL";
    const bool isRadar = name.starts_with("radar");
    const bool isArrow = name.starts_with("arrow");

    if (isNull || isRadar || isArrow)
    {
        store.push_back(line);
    }
}
