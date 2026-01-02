#include "pch.h"
#include "mva_loader.h"
#include "logger.h"
#include <fstream>

CMvaLoader MvaLoader;

namespace
{
    std::string ToLower(std::string value)
    {
        for (char& ch : value)
        {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return value;
    }

    bool IsHiddenFolder(const std::filesystem::path& path)
    {
        std::string folderName = path.filename().string();
        return !folderName.empty() && folderName[0] == '.';
    }
}

void CMvaLoader::Process()
{
    const std::filesystem::path modloaderRoot = GAME_PATH((char*)"modloader");
    if (modloaderRoot.empty() || !std::filesystem::exists(modloaderRoot))
    {
        Logger.Log("MVA: modloader folder not found, skipping.");
        return;
    }

    Logger.Log(std::string("MVA: scanning modloader at ") + modloaderRoot.string());

    std::vector<MvaFileEntry> entries;
    CollectMvaFiles(modloaderRoot, entries);
    if (entries.empty())
    {
        Logger.Log("MVA: no .mva files found.");
        return;
    }

    Logger.Log("MVA: found " + std::to_string(entries.size()) + " .mva files.");

    const std::filesystem::path modloaderIni = modloaderRoot / "modloader.ini";
    std::unordered_map<std::string, int> priorities = LoadPriorities(modloaderIni);
    Logger.Log("MVA: loaded " + std::to_string(priorities.size()) + " mod priorities.");

    std::unordered_map<std::string, std::vector<MvaFileEntry>> grouped;
    for (auto& entry : entries)
    {
        auto it = priorities.find(entry.modName);
        if (it != priorities.end())
        {
            entry.priority = it->second;
        }

        std::filesystem::path relativePath = std::filesystem::relative(entry.sourcePath, modloaderRoot);
        auto pathIt = relativePath.begin();
        if (pathIt == relativePath.end())
        {
            continue;
        }

        ++pathIt;
        std::filesystem::path targetRelative;
        for (; pathIt != relativePath.end(); ++pathIt)
        {
            targetRelative /= *pathIt;
        }

        if (targetRelative.empty())
        {
            continue;
        }

        std::string targetPath = GAME_PATH((char*)targetRelative.string().c_str());
        grouped[targetPath].push_back(entry);
    }

    Logger.Log("MVA: grouped into " + std::to_string(grouped.size()) + " target files.");

    for (auto& group : grouped)
    {
        auto& files = group.second;
        Logger.Log("MVA: processing target " + group.first + " with " + std::to_string(files.size()) + " source files.");
        std::stable_sort(files.begin(), files.end(), [](const MvaFileEntry& left, const MvaFileEntry& right)
            {
                if (left.priority != right.priority)
                {
                    return left.priority < right.priority;
                }

                return left.sourcePath.string() < right.sourcePath.string();
            });

        std::string finalContent;
        size_t index = 0;
        while (index < files.size())
        {
            const int priority = files[index].priority;
            Logger.Log("MVA: merging priority " + std::to_string(priority));
            std::string mergedContent;
            while (index < files.size() && files[index].priority == priority)
            {
                std::string content = ReadFileContents(files[index].sourcePath);
                Logger.Log("MVA: reading " + files[index].sourcePath.string());
                AppendFileContents(mergedContent, content);
                ++index;
            }

            finalContent = mergedContent;
        }

        if (finalContent.empty())
        {
            Logger.Log("MVA: final content empty for " + group.first + ", skipping write.");
            continue;
        }

        std::filesystem::path targetPath(group.first);
        if (!targetPath.has_parent_path())
        {
            Logger.Log("MVA: invalid target path " + group.first);
            continue;
        }

        std::filesystem::create_directories(targetPath.parent_path());
        std::ofstream out(targetPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            Logger.Log("MVA: failed to write " + group.first);
            continue;
        }

        out.write(finalContent.data(), static_cast<std::streamsize>(finalContent.size()));
        out.close();
        Logger.Log("MVA: wrote " + group.first);
    }
}

void CMvaLoader::CollectMvaFiles(const std::filesystem::path& modloaderRoot, std::vector<MvaFileEntry>& entries) const
{
    for (const auto& entry : std::filesystem::directory_iterator(modloaderRoot))
    {
        if (entry.is_directory())
        {
            if (IsHiddenFolder(entry.path()))
            {
                continue;
            }

            for (auto it = std::filesystem::recursive_directory_iterator(entry.path());
                it != std::filesystem::recursive_directory_iterator();
                ++it)
            {
                if (it->is_directory())
                {
                    if (IsHiddenFolder(it->path()))
                    {
                        it.disable_recursion_pending();
                        continue;
                    }
                    continue;
                }

                if (!it->is_regular_file())
                {
                    continue;
                }

                if (ToLower(it->path().extension().string()) == ".mva")
                {
                    std::filesystem::path relativePath = std::filesystem::relative(it->path(), modloaderRoot);
                    auto relIt = relativePath.begin();
                    if (relIt == relativePath.end())
                    {
                        continue;
                    }

                    std::string modName = relIt->string();
                    if (modName.empty())
                    {
                        continue;
                    }

                    Logger.Log("MVA: found " + it->path().string() + " in mod " + modName);
                    entries.push_back({ it->path(), modName, 0 });
                }
            }
        }
    }
}

std::unordered_map<std::string, int> CMvaLoader::LoadPriorities(const std::filesystem::path& modloaderIni) const
{
    std::unordered_map<std::string, int> priorities;
    if (!std::filesystem::exists(modloaderIni))
    {
        Logger.Log("MVA: modloader.ini not found, default priorities assumed.");
        return priorities;
    }

    linb::ini ini;
    if (!ini.load_file(modloaderIni.string()))
    {
        Logger.Log("MVA: failed to read modloader.ini.");
        return priorities;
    }

    auto section = ini.find("Profiles.Default.Priority");
    if (section == ini.end())
    {
        Logger.Log("MVA: Profiles.Default.Priority section not found.");
        return priorities;
    }

    for (const auto& kv : section->second)
    {
        try
        {
            int value = std::stoi(kv.second, nullptr, 10);
            priorities[kv.first] = value;
        }
        catch (const std::exception&)
        {
            Logger.Log("MVA: invalid priority for mod " + kv.first);
            continue;
        }
    }

    return priorities;
}

std::string CMvaLoader::ReadFileContents(const std::filesystem::path& path) const
{
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
    {
        return {};
    }

    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    return contents;
}

void CMvaLoader::AppendFileContents(std::string& target, const std::string& content) const
{
    if (content.empty())
    {
        return;
    }

    if (!target.empty())
    {
        if (target.back() != '\n' && content.front() != '\n')
        {
            target += "\n";
        }
    }

    target += content;
}
