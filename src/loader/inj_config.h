#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class InjModifier
{
    Replace,
    Merge
};

struct InjEntry
{
    InjModifier modifier{};
    std::string iniFile;
    std::string section;
    std::string key;
    std::string value;
    std::filesystem::path sourcePath;
};

class CInjConfigLoader
{
public:
    void Process(const std::filesystem::path& pluginDir);

private:
    void CollectInjFiles(const std::filesystem::path& dir, std::vector<std::filesystem::path>& files) const;
    void ParseFile(const std::filesystem::path& path);
    void ApplyEntriesToFile(const std::filesystem::path& iniPath, const std::vector<InjEntry>& entries) const;
    std::filesystem::path LocateIniFile(
        const InjEntry& entry,
        const std::filesystem::path& gameRoot,
        const std::filesystem::path& modloaderRoot,
        std::unordered_map<std::string, std::filesystem::path>& cache,
        std::unordered_set<std::string>& missing) const;

    std::vector<InjEntry> entries;
};

extern CInjConfigLoader InjConfigLoader;
