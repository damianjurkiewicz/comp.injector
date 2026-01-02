#pragma once
#include <filesystem>
#include <string>

class CLogger
{
public:
    void Init(const std::filesystem::path& logPath);
    void Log(const std::string& message);

private:
    std::filesystem::path path;
};

extern CLogger Logger;
