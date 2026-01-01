#include "pch.h"
#include "tracks_config.h"
#include <sstream>
#include <unordered_set>

CFLATracksConfigLoader FLATracksConfigLoader;

void CFLATracksConfigLoader::UpdateTracksConfigFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/Paths/gtasa_tracks_config.dat");
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
        const std::string marker = "; comp.injector added gtasa_tracks_config";

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

void CFLATracksConfigLoader::Process()
{
    if (!store.empty())
    {
        UpdateTracksConfigFile();
    }
}

void CFLATracksConfigLoader::Parse(const std::string &line)
{
    std::istringstream stream(line);
    std::string filename;
    std::string extra;

    if (!(stream >> filename))
    {
        return;
    }

    if (stream >> extra)
    {
        return;
    }

    if (filename.size() < 5 || filename.substr(filename.size() - 4) != ".dat")
    {
        return;
    }

    store.push_back(line);
}
