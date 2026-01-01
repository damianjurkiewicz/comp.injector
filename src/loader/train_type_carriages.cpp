#include "pch.h"
#include "train_type_carriages.h"
#include <sstream>
#include <unordered_set>

CFLATrainTypeCarriagesLoader FLATrainTypeCarriagesLoader;

void CFLATrainTypeCarriagesLoader::UpdateTrainTypeCarriagesFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/gtasa_trainTypeCarriages.dat");
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
        const std::string marker = "; comp.injector added gtasa_trainTypeCarriages";

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

void CFLATrainTypeCarriagesLoader::Process()
{
    if (!store.empty())
    {
        UpdateTrainTypeCarriagesFile();
    }
}

void CFLATrainTypeCarriagesLoader::AddLine(const std::string &line)
{
    store.push_back(line);
}

void CFLATrainTypeCarriagesLoader::Parse(const std::string &line)
{
    std::istringstream stream(line);
    int trainType = 0;

    if (!(stream >> trainType))
    {
        return;
    }

    std::string carriage;
    int count = 0;

    while (stream >> carriage)
    {
        ++count;
        if (count > 12)
        {
            break;
        }
    }

    if (count >= 1 && count <= 12)
    {
        store.push_back(line);
    }
}
