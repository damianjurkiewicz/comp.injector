#pragma once
#include <filesystem>
#include <string>
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
    void CollectMvaFiles(const std::filesystem::path& modloaderRoot, std::vector<MvaFileEntry>& entries) const;
    std::unordered_map<std::string, int> LoadPriorities(const std::filesystem::path& modloaderIni) const;
    std::string ReadFileContents(const std::filesystem::path& path) const;
    void AppendFileContents(std::string& target, const std::string& content) const;
};

extern CMvaLoader MvaLoader;
