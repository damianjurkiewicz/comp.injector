#include "pch.h"
#include "loader_core.h"
#include "audio.h"
#include "default_audio_data.h" 

CompInjector::CompInjector(HINSTANCE pluginHandle)
{

    handle = pluginHandle;

    HandleVanillaDataFiles();
    ParseModloader();

    if (gConfig.ReadInteger("MAIN", "FLAAudioLoader", 1) == 1)
    {
        FLAAudioLoader.Process();
    }
}


void CompInjector::HandleVanillaDataFiles()
{
   

    int flaAudioLoaderSetting = gConfig.ReadInteger("MAIN", "FLAAudioLoader", 1);

    std::string settingsPath = GAME_PATH((char*)"data/gtasa_vehicleAudioSettings.cfg");
    std::string backupPath = settingsPath + ".comp.injector.bak";


    

    if (flaAudioLoaderSetting == 1) 
    {
      
        if (std::filesystem::exists(settingsPath) && !std::filesystem::exists(backupPath))
        {
            int result = MessageBox(NULL,
                "Comp.Injector (FLAAudioLoader) is about to modify 'gtasa_vehicleAudioSettings.cfg' to add new vehicle sounds.\n\n"
                "Do you want to create a one-time backup of the original file? (Recommended)",
                MODNAME,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);

            if (result == IDYES)
            {
                try {
                    std::filesystem::copy_file(settingsPath, backupPath, std::filesystem::copy_options::overwrite_existing);
                }
                catch (const std::exception& e) {
                    MessageBox(NULL, ("Failed to create audio backup: " + std::string(e.what())).c_str(), MODNAME, MB_OK | MB_ICONERROR);
                }
            }
        }
    }
  
   
    std::function<void(const std::filesystem::path&)> traverse;
    traverse = [&](const std::filesystem::path& dir)
        {
            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (entry.is_directory())
                {
                    std::string folderName = entry.path().filename().string();
                    if (!folderName.empty() && folderName[0] == '.')
                    {
                        continue;
                    }
                    traverse(entry.path());
                    continue;
                }
                if (!entry.is_regular_file())
                {
                    continue;
                }
                std::string fileName = entry.path().filename().string();


              
            }
        };

    traverse(GAME_PATH((char*)"modloader"));
}




void CompInjector::ParseModloader()
{
  
    std::function<void(const std::filesystem::path&)> traverse;
    traverse = [&](const std::filesystem::path& dir)
        {
            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (entry.is_directory())
                {
                    std::string folderName = entry.path().filename().string();
                    if (!folderName.empty() && folderName[0] == '.')
                    {
                        continue;
                    }
                    traverse(entry.path());
                    continue;
                }
                if (!entry.is_regular_file())
                {
                    continue;
                }
                std::string ext = entry.path().extension().string();
                std::string path = entry.path().string();

                if (ext == ".comp.injector")
                {
                    std::ifstream in(path);
                    std::string line;
                    while (getline(in, line))
                    {
                        if (line.starts_with(";") || line.starts_with("//") || line.starts_with("#"))
                        {
                            continue;
                        }
                        if (gConfig.ReadInteger("MAIN", "FLAAudioLoader", 1) == 1)
                        {
                            FLAAudioLoader.Parse(line);
                        }
                    }
                    in.close();
                }
            }
        };

    traverse(GAME_PATH((char*)"modloader"));
}
