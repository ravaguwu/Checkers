#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        reload();
    }

    // Overwrites data from the config
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        config = json::parse(fin, nullptr, true, true);
        fin.close();
    }

    // For more convenient access to the config
    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        return config[setting_dir][setting_name];
    }

  private:
    json config;
};
