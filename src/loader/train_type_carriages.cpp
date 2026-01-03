#include "pch.h"
#include "train_type_carriages.h"
#include "logger.h"
#include <sstream>
#include <unordered_set>

CFLATrainTypeCarriagesLoader FLATrainTypeCarriagesLoader;

namespace
{
    const char* kLogPrefix = "TRAIN_TYPE_CARRIAGES";
    const char* kMarker = "; comp.injector added gtasa_trainTypeCarriages";

    std::filesystem::path GetBackupPath(const std::filesystem::path& settingsPath)
    {
        std::filesystem::path backupPath = settingsPath;
        backupPath += ".back";

        std::filesystem::path cacheDir = Logger.GetCacheDirectory();
        if (!cacheDir.empty())
        {
            std::filesystem::path relativePath = settingsPath.is_absolute()
                ? settingsPath.relative_path()
                : settingsPath;
            backupPath = cacheDir / relativePath;
            backupPath += ".back";
            std::error_code ec;
            std::filesystem::create_directories(backupPath.parent_path(), ec);
        }

        return backupPath;
    }

    std::filesystem::path GetBasePathWithBackup(const std::filesystem::path& settingsPath)
    {
        std::filesystem::path backupPath = GetBackupPath(settingsPath);
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

void CFLATrainTypeCarriagesLoader::UpdateTrainTypeCarriagesFile()
{
    std::filesystem::path settingsPath = GAME_PATH((char*)"data/gtasa_trainTypeCarriages.dat");
    std::filesystem::path settingsPathTemp = settingsPath;
    settingsPathTemp += ".tmp";
    std::filesystem::path basePath = GetBasePathWithBackup(settingsPath);
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

void CFLATrainTypeCarriagesLoader::Process()
{
    if (store.empty() && !HasMarker(GAME_PATH((char*)"data/gtasa_trainTypeCarriages.dat")))
    {
        Logger.Log(std::string(kLogPrefix) + ": no entries and no marker, skipping.");
        return;
    }

    Logger.Log(std::string(kLogPrefix) + ": processing train type carriages.");
    UpdateTrainTypeCarriagesFile();
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
