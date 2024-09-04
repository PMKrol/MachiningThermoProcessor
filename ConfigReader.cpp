#include "ConfigReader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

// Implementacja funkcji readConfig
Config readConfig(const std::string &filename) {
    Config config;
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open configuration file: " << filename << std::endl;
        return config;
    }

    std::string line;
    std::map<std::string, std::string> configMap;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;  // Skip empty lines and comments

        std::istringstream iss(line);
        std::string key, equalSign, value;
        if (iss >> key >> equalSign >> value && equalSign == "=") {
            configMap[key] = value;
        }
    }

    // Convert the string values to appropriate types and store in the config struct
    config.drill_start_x = std::stoi(configMap["drill_start_x"]);
    config.drill_start_y = std::stoi(configMap["drill_start_y"]);
    config.drill_end_x = std::stoi(configMap["drill_end_x"]);
    config.drill_end_y = std::stoi(configMap["drill_end_y"]);
    config.drill_width = std::stoi(configMap["drill_width"]);  // Read the drill width
    config.hotspot_x = std::stoi(configMap["hotspot_x"]);
    config.hotspot_y = std::stoi(configMap["hotspot_y"]);
    config.hotspot_size = std::stoi(configMap["hotspot_size"]);
    config.hotspot_temp_threshold = std::stof(configMap["hotspot_temp_threshold"]);

    return config;
}
