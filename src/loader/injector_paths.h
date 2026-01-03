#pragma once

#include "logger.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace InjectorPaths
{
    inline std::string ToLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
        return value;
    }

    inline std::filesystem::path TrimModloaderPrefix(const std::filesystem::path& relativePath)
    {
        if (relativePath.empty())
        {
            return {};
        }

        auto it = relativePath.begin();
        if (it == relativePath.end())
        {
            return {};
        }

        if (ToLower(it->string()) != "modloader")
        {
            return {};
        }

        ++it;
        if (it == relativePath.end())
        {
            return {};
        }

        ++it;
        if (it == relativePath.end())
        {
            return {};
        }

        std::filesystem::path trimmed;
        for (; it != relativePath.end(); ++it)
        {
            trimmed /= *it;
        }

        return trimmed;
    }

    inline std::filesystem::path GetInjectorPathFor(const std::filesystem::path& targetPath)
    {
        std::filesystem::path injectorDir = Logger.GetInjectorDirectory();
        if (injectorDir.empty() || targetPath.empty())
        {
            return {};
        }

        if (targetPath.is_relative())
        {
            return injectorDir / targetPath;
        }

        std::filesystem::path gameRoot = Logger.GetGameDirectory();
        if (!gameRoot.empty())
        {
            std::error_code ec;
            std::filesystem::path relativePath = std::filesystem::relative(targetPath, gameRoot, ec);
            if (!ec && !relativePath.empty())
            {
                std::filesystem::path trimmed = TrimModloaderPrefix(relativePath);
                if (!trimmed.empty())
                {
                    return injectorDir / trimmed;
                }

                const std::string firstComponent = ToLower(relativePath.begin()->string());
                if (firstComponent != "..")
                {
                    return injectorDir / relativePath;
                }
            }
        }

        return injectorDir / targetPath.filename();
    }
}
