#include "pch.h"
#include "train_type_carriages.h"
#include <sstream>
#include <unordered_set>

CFLATrainTypeCarriagesLoader FLATrainTypeCarriagesLoader;

namespace
{
    std::string GetBasePathWithBackup(const std::string& settingsPath)
    {
        std::string backupPath = settingsPath + ".back";
        if (std::filesystem::exists(settingsPath) && !std::filesystem::exists(backupPath))
        {
            try
            {
                std::filesystem::copy_file(settingsPath, backupPath, std::filesystem::copy_options::overwrite_existing);
            }
            catch (const std::exception&)
            {
            }
        }

        if (std::filesystem::exists(backupPath))
        {
            return backupPath;
        }

        return settingsPath;
    }
}

void CFLATrainTypeCarriagesLoader::UpdateTrainTypeCarriagesFile()
{
    std::string settingsPath = GAME_PATH((char*)"data/gtasa_trainTypeCarriages.dat");
    std::string settingsPathTemp = settingsPath + ".tmp";
    std::string basePath = GetBasePathWithBackup(settingsPath);
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
        return;
    }

    std::unordered_set<std::string> linesToAdd(store.begin(), store.end());
    std::unordered_set<std::string> writtenLines;

    std::ifstream in(basePath);
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

        for (const auto &e : store)
        {
            if (writtenLines.insert(e).second)
            {
                out << e << "\n";
            }
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
