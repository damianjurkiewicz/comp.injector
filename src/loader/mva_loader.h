#pragma once
#include <filesystem>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

struct MvaFileEntry
{
    std::filesystem::path sourcePath;
    std::string modName;
    int priority = 0;
};

class CMvaLoader
{
public:
    void Process();

private:
    using IniSection = std::map<std::string, std::string>;
    using IniData = std::map<std::string, IniSection>;

    void CollectMvaFiles(const std::filesystem::path& modloaderRoot, std::vector<MvaFileEntry>& entries) const;
    std::unordered_map<std::string, int> LoadPriorities(const std::filesystem::path& modloaderIni) const;
    IniData ReadIniData(const std::filesystem::path& path) const;
    void MergeIniData(IniData& target, const IniData& source) const;
    std::string WriteIniData(const IniData& data) const;
};

extern CMvaLoader MvaLoader;
