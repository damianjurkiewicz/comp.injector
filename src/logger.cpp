#include "pch.h"
#include "logger.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

CLogger Logger;

void CLogger::Init(const std::filesystem::path& logPath)
{
    path = logPath;

    if (path.empty())
    {
        return;
    }

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open())
    {
        return;
    }
}

void CLogger::Log(const std::string& message)
{
    if (path.empty())
    {
        return;
    }

    std::ofstream out(path, std::ios::app);
    if (!out.is_open())
    {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = {};
    localtime_s(&localTime, &nowTime);

    std::ostringstream stamp;
    stamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

    out << "[" << stamp.str() << "] " << message << "\n";
    out.close();
}

std::filesystem::path CLogger::GetCacheDirectory() const
{
    if (path.empty())
    {
        return {};
    }

    std::filesystem::path cacheDir = path.parent_path() / "cache";
    std::error_code ec;
    std::filesystem::create_directories(cacheDir, ec);
    return cacheDir;
}
