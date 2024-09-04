#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <limits>
#include <opencv2/opencv.hpp>
#include <algorithm> // For std::sort
#include <stdexcept>

#define silentMTPF 1  // Set to 1 to suppress mtpf error messages
#define ZERO_COUNT 4    //how many frames has to be without hotspot 
                        // to be considered as a gap (oneShot mode) 
                        // between beforeShot and afterShot
                        
#include "ConfigReader.h"

// // Define the Config struct
// struct Config {
//     int drill_start_x;
//     int drill_start_y;
//     int drill_end_x;
//     int drill_end_y;
//     int drill_width;
//     int hotspot_x;
//     int hotspot_y;
//     int hotspot_size;
//     float hotspot_temp_threshold;
// };

// Function to check if a command exists in the system
bool commandExists(const std::string &command) {
    std::string cmd = "which " + command + " > /dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

// Function to execute mtpf and return the result (0 or 1)
int runMtpf(const std::string &filename) {
    // Check if mtpf command exists in the system
    if (!commandExists("mtpFrame")) {
        // Check if local file './mtpf' exists
        if (!std::filesystem::exists("./mtpFrame")) {
            if (!silentMTPF) {
                std::cerr << "Error: mtpFrame command or local './mtpFrame' file not found." << std::endl;
            }
            std::exit(EXIT_FAILURE); // Exit the program if neither is found
        }
    }

    // Execute mtpf command or local file './mtpf' with the provided filename
    std::string command = "mtpFrame " + filename;
    if (std::filesystem::exists("./mtpFrame")) {
        command = "./mtpFrame " + filename;
    }

    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        if (!silentMTPF) {
            std::cerr << "Error: Could not execute mtpFrame for file: " << filename << std::endl;
        }
        return -1;
    }

    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    int returnValue;
    try {
        returnValue = std::stoi(result);
    } catch (const std::exception &e) {
        if (!silentMTPF) {
            std::cerr << "Error: Could not parse mtpFrame output for file: " << filename << std::endl;
        }
        returnValue = -1;
    }

    if (!silentMTPF) {
        std::cout << filename << ": " << returnValue << std::endl;
    }

    return returnValue;
}

// // Function to read configuration file and return config values
// Config readConfig(const std::string &filename) {
//     Config config;
//     std::ifstream file(filename);
//     if (!file) {
//         std::cerr << "Error: Could not open configuration file: " << filename << std::endl;
//         return config;
//     }
// 
//     std::string line;
//     std::map<std::string, std::string> configMap;
//     while (std::getline(file, line)) {
//         if (line.empty() || line[0] == '#') continue;  // Skip empty lines and comments
// 
//         std::istringstream iss(line);
//         std::string key, equalSign, value;
//         if (iss >> key >> equalSign >> value && equalSign == "=") {
//             configMap[key] = value;
//         }
//     }
// 
//     // Convert the string values to appropriate types and store in the config struct
//     config.drill_start_x = std::stoi(configMap["drill_start_x"]);
//     config.drill_start_y = std::stoi(configMap["drill_start_y"]);
//     config.drill_end_x = std::stoi(configMap["drill_end_x"]);
//     config.drill_end_y = std::stoi(configMap["drill_end_y"]);
//     config.drill_width = std::stoi(configMap["drill_width"]);
//     config.hotspot_x = std::stoi(configMap["hotspot_x"]);
//     config.hotspot_y = std::stoi(configMap["hotspot_y"]);
//     config.hotspot_size = std::stoi(configMap["hotspot_size"]);
//     config.hotspot_temp_threshold = std::stof(configMap["hotspot_temp_threshold"]);
// 
//     return config;
// }

// Function to find the first and second shots based on mtpf results
bool findShots(const std::vector<std::string> &tc0Files, std::string &beforeShot, std::string &afterShot) {
    bool foundFirst = false;
    int zeroCount = 0;
    for (const auto &file : tc0Files) {
        if (runMtpf(file) == 1) {
            if (!foundFirst) {
                beforeShot = file;
                foundFirst = true;
                zeroCount = 0;
            } else if (zeroCount >= ZERO_COUNT) {
                afterShot = file;
                return true;
            }
        } else {
            if (foundFirst) {
                ++zeroCount;
            }
        }
    }
    return false;
}

// Function to calculate the maximum temperature along the drill line
float calculateMaxTemperatureOnDrillLine(const std::string &imageFilename, const Config &config) {
    cv::Mat img = cv::imread(imageFilename, cv::IMREAD_UNCHANGED); // Read 16-bit image
    if (img.empty()) {
        std::cerr << "Error: Could not read image file: " << imageFilename << std::endl;
        return std::numeric_limits<float>::lowest();
    }

    int xStart = config.drill_start_x;
    int xEnd = config.drill_end_x;
    int y = config.drill_start_y; // Assuming the drill is horizontal

    float maxTemp = std::numeric_limits<float>::lowest();
    for (int x = xStart; x <= xEnd; ++x) {
        uint16_t temp = img.at<uint16_t>(y, x);
        float temperature = static_cast<float>(temp) / 65535.0f * 256.0f; // Convert 16-bit value to temperature
        maxTemp = std::max(maxTemp, temperature);
    }
    return maxTemp;
}

// Function to sort files in a vector
void sortTc0Files(std::vector<std::string> &files) {
    std::sort(files.begin(), files.end());
}

void checkDirectoryExists(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        throw std::runtime_error("Directory does not exist: " + directory);
    }
    if (!std::filesystem::is_directory(directory)) {
        throw std::runtime_error("Not a directory: " + directory);
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory> [-os|--oneShot]" << std::endl;
        return 1;
    }

    std::string directory;
    bool oneShotMode = false;

    // Parse command line arguments
    std::vector<std::string> args(argv + 1, argv + argc);
    auto dirIt = std::find_if(args.begin(), args.end(), [](const std::string& arg) {
        return std::filesystem::is_directory(arg);
    });

    if (dirIt != args.end()) {
        directory = *dirIt;
        args.erase(dirIt);
    } else {
        std::cerr << "Error: No valid directory argument found." << std::endl;
        return 1;
    }
    
    auto osIt = std::find_if(args.begin(), args.end(), [](const std::string& arg) {
        return arg == "-os" || arg == "--oneShot";
    });

    if (osIt != args.end()) {
        oneShotMode = true;
        args.erase(osIt);
    }
    
    std::cout << "Dir: " << directory << ", oneShot mode: " << oneShotMode << std::endl;
    
    // Read config file
    Config config = readConfig("config.txt");

    try {
        checkDirectoryExists(directory);
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::vector<std::string> tc0Files;
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".tc0") {
            tc0Files.push_back(entry.path().string());
        }
    }

    // Sort the files
    sortTc0Files(tc0Files);

    if (oneShotMode) {
        std::string beforeShot, afterShot;
        if (findShots(tc0Files, beforeShot, afterShot)) {
            std::cout << "Before shot: " << beforeShot << std::endl;
            std::cout << "After shot: " << afterShot << std::endl;

            std::string beforeShotImage = beforeShot.substr(0, beforeShot.find_last_of('.')) + "_16bit.png";
            std::string afterShotImage = afterShot.substr(0, afterShot.find_last_of('.')) + "_16bit.png";

            float maxTempBefore = calculateMaxTemperatureOnDrillLine(beforeShotImage, config);
            float maxTempAfter = calculateMaxTemperatureOnDrillLine(afterShotImage, config);

            std::cout << "Max temperature before shot: " << maxTempBefore << std::endl;
            std::cout << "Max temperature after shot: " << maxTempAfter << std::endl;
        } else {
            std::cout << "No suitable shots found." << std::endl;
        }
    } else {
        for (const auto &file : tc0Files) {
            int result = runMtpf(file);
            std::cout << file << ": " << result << std::endl;
        }
    }

    return 0;
}
