#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <opencv2/opencv.hpp>  // OpenCV library for image processing
#include <png.h>
#include <string>
#include <iomanip> // for std::setprecision
#include <sstream>
#include <map>

#include "ConfigReader.h"

// // Struct to hold configuration data
// struct Config {
//     int drill_start_x;
//     int drill_start_y;
//     int drill_end_x;
//     int drill_end_y;
//     int drill_width;  // New field for drill width
//     int hotspot_x;
//     int hotspot_y;
//     int hotspot_size;
//     float hotspot_temp_threshold;
// };
// 
// // Function to read the configuration file and populate the Config struct
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
//     config.drill_width = std::stoi(configMap["drill_width"]);  // Read the drill width
//     config.hotspot_x = std::stoi(configMap["hotspot_x"]);
//     config.hotspot_y = std::stoi(configMap["hotspot_y"]);
//     config.hotspot_size = std::stoi(configMap["hotspot_size"]);
//     config.hotspot_temp_threshold = std::stof(configMap["hotspot_temp_threshold"]);
// 
//     return config;
// }

// Function to load raw data from the file and split it into image and temperature matrices
bool loadDataFromFile(const std::string &filename, std::vector<uint16_t> &imageData, std::vector<uint16_t> &temperatureData, int &rows, int &cols) {
    // Open the binary file
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file." << std::endl;
        return false;
    }

    // Read the entire file as uint16_t (2 bytes)
    std::vector<uint16_t> val;
    uint16_t temp;
    while (file.read(reinterpret_cast<char*>(&temp), sizeof(uint16_t))) {
        val.push_back(temp);
    }
    file.close();

    // Read the headers (rows, cols, type, channels)
    rows = val[0];    // Number of rows in the matrix
    cols = val[1];    // Number of columns in the matrix
    //int type = val[2];  // Data type (not used here)
    //int chan = val[3];  // Number of channels (not used here)

    // Split the data into image (upper half) and temperature (lower half)
    imageData.assign(val.begin() + 4, val.begin() + 4 + (rows / 2) * cols);  // Upper half of the data
    temperatureData.assign(val.begin() + 4 + (rows / 2) * cols, val.end());  // Lower half of the data

    return true;
}

// Function to convert raw temperature data to a temperature matrix in Celsius
std::vector<std::vector<float>> convertToTemperature(const std::vector<uint16_t> &temperatureData, int rows, int cols) {
    std::vector<std::vector<float>> temperatureMatrix(rows / 2, std::vector<float>(cols));

    // Convert each value using the formula: t = x / 64 - 273.15
    for (int i = 0; i < rows / 2; ++i) {
        for (int j = 0; j < cols; ++j) {
            temperatureMatrix[i][j] = static_cast<float>(temperatureData[i * cols + j]) / 64.0f - 273.15f;
        }
    }

    return temperatureMatrix;
}

// Function to save a 16-bit or 8-bit PNG image with metadata
void saveImageWithMetadata(const std::string &filename, const cv::Mat &image, const std::string &metadata, int depth) {
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        std::cerr << "Could not open file for writing: " << filename << std::endl;
        return;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return;
    }

    png_init_io(png, fp);

    // Set the PNG metadata
    png_text text;
    text.compression = PNG_TEXT_COMPRESSION_NONE;
    text.key = const_cast<char*>("Description");
    text.text = const_cast<char*>(metadata.c_str());
    text.text_length = metadata.length();
    png_set_text(png, info, &text, 1);

    // Set the image info based on depth
    int colorType = PNG_COLOR_TYPE_GRAY;
    if (depth == 16) {
        png_set_IHDR(png, info, image.cols, image.rows, 16, colorType,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    } else {
        png_set_IHDR(png, info, image.cols, image.rows, 8, colorType,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    }

    png_write_info(png, info);

    // Handle endianness for 16-bit images
    if (depth == 16) {
        png_set_swap(png);  // Swap endianness if the image is stored in little-endian format
    }
    
    // Write the image data
    for (int y = 0; y < image.rows; ++y) {
        if (depth == 16) {
            // If the image is 16-bit, treat each pixel as a 16-bit value
            png_bytep row = (png_bytep)(image.ptr<uint16_t>(y));
            png_write_row(png, row);
        } else {
            // If the image is 8-bit, treat each pixel as an 8-bit value
            png_bytep row = (png_bytep)(image.ptr<uint8_t>(y));
            png_write_row(png, row);
        }
    }

    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

void drawFrames(cv::Mat &img_lin, const Config &config) {
    // Calculate the drill height based on the drill width
    int drill_height = config.drill_width;
    int margin = drill_height / 2;  // Calculate the margin above and below the drill

    // Define the four corner points of the rectangle around the drill
    cv::Point topLeft(config.drill_start_x, config.drill_start_y + margin);
    cv::Point topRight(config.drill_end_x, config.drill_start_y + margin);
    cv::Point bottomRight(config.drill_end_x, config.drill_start_y - margin);
    cv::Point bottomLeft(config.drill_start_x, config.drill_start_y - margin);

    // Draw the white frame around the drill using lines connecting the corner points
    cv::line(img_lin, topLeft, topRight, cv::Scalar(255), 1); // Top edge
    cv::line(img_lin, topRight, bottomRight, cv::Scalar(255), 1); // Right edge
    cv::line(img_lin, bottomRight, bottomLeft, cv::Scalar(255), 1); // Bottom edge
    cv::line(img_lin, bottomLeft, topLeft, cv::Scalar(255), 1); // Left edge

    // Draw a white frame around the hotspot
    cv::rectangle(img_lin,
                  cv::Point(config.hotspot_x, config.hotspot_y),
                  cv::Point(config.hotspot_x + config.hotspot_size, config.hotspot_y + config.hotspot_size),
                  cv::Scalar(255), 1);
}

// Function to convert temperature matrix to images and save them
void convertTemperatureToImage(const std::vector<std::vector<float>> &temperatureMatrix, const std::string &outputFilename, const Config &config) {
    int rows = temperatureMatrix.size();
    int cols = temperatureMatrix[0].size();

    // Create OpenCV matrices to store the image data
    cv::Mat img_16bit(rows, cols, CV_16UC1);  // 16-bit image for the 0-256 range
    cv::Mat img_8bit(rows, cols, CV_8UC1);    // 8-bit image for the 0-128 range
    cv::Mat img_lin(rows, cols, CV_8UC1);     // 8-bit image for the min-max range

    // Find the minimum and maximum temperature in the matrix
    float minTemp = std::numeric_limits<float>::max();
    float maxTemp = std::numeric_limits<float>::min();
    for (const auto& row : temperatureMatrix) {
        float row_min = *std::min_element(row.begin(), row.end());
        float row_max = *std::max_element(row.begin(), row.end());
        minTemp = std::min(minTemp, row_min);
        maxTemp = std::max(maxTemp, row_max);
    }

    // Convert temperature values to grayscale for all three images
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            float temp = temperatureMatrix[i][j];

            // For the 16-bit image: scale temperature to 0-65535 (16-bit)
            uint16_t pixelValue16 = static_cast<uint16_t>(std::clamp(temp/255.0f*65535.0f, 0.0f, 65535.0f));
            img_16bit.at<uint16_t>(i, j) = pixelValue16;

            // For the 8-bit image: scale temperature to 0-255 (8-bit, range 0-127)
            uint8_t pixelValue8 = static_cast<uint8_t>(std::clamp(temp * 2.0f, 0.0f, 255.0f));
            img_8bit.at<uint8_t>(i, j) = pixelValue8;

            // For the linear scaled image: scale temperature to 0-255 based on min-max range
            uint8_t pixelValueLin = static_cast<uint8_t>(std::clamp((temp - minTemp) / (maxTemp - minTemp) * 255.0f, 0.0f, 255.0f));
            img_lin.at<uint8_t>(i, j) = pixelValueLin;
        }
    }

    // Save the 16-bit image with metadata
    std::string outputFilename16bit = outputFilename + "_16bit.png";
    saveImageWithMetadata(outputFilename16bit, img_16bit, "Temperature Range: 0-256", 16);

    // Save the 8-bit image with metadata
    std::string outputFilename8bit = outputFilename + "_8bit.png";
    saveImageWithMetadata(outputFilename8bit, img_8bit, "Temperature Range: 0-128", 8);

    // Save the linear scaled image with metadata
    std::string outputFilenameLin = outputFilename + "_lin.png";
    std::string minMaxText = "Min: " + std::to_string(minTemp) + " Max: " + std::to_string(maxTemp);
    drawFrames(img_lin, config);
    saveImageWithMetadata(outputFilenameLin, img_lin, minMaxText, 8);
}

// Function to save the temperature matrix to a tab-delimited text file
void saveTemperatureMatrixToFile(const std::vector<std::vector<float>> &temperatureMatrix, const std::string &outputFilename) {
    // Append ".txt" to the output filename
    std::string fullFilename = outputFilename + ".csv";
    
    std::ofstream outFile(fullFilename);

    // Check if the file opened successfully
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file " << fullFilename << " for writing." << std::endl;
        return;
    }

    // Iterate through the temperature matrix and write each value to the file
    for (const auto &row : temperatureMatrix) {
        for (size_t j = 0; j < row.size(); ++j) {
            outFile << std::fixed << std::setprecision(2) << row[j]; // Write temperature with two decimal places
            if (j < row.size() - 1) {
                outFile << "\t"; // Add tab except for the last element in the row
            }
        }
        outFile << "\n"; // New line at the end of each row
    }

    // Close the file
    outFile.close();
}

// Function to convert raw image data to a YUYV422 image format and save it as JPG
void convertImageDataToImage(const std::vector<uint16_t> &imageData, int rows, int cols, const std::string &outputFilename) {
    // Create an OpenCV matrix to store the YUYV422 image data
    cv::Mat img_yuyv(rows / 2, cols, CV_8UC2);  // YUYV422 format: 2 bytes per pixel (YUYV)

    for (int i = 0; i < rows / 2; ++i) {
        for (int j = 0; j < cols; ++j) {
            uint16_t value = imageData[i * cols + j];
            uint8_t y = static_cast<uint8_t>(value >> 8);  // Extract the Y (luminance) component
            uint8_t uv = static_cast<uint8_t>(value & 0xFF);  // Extract the U or V component (we'll treat as YUV422)

            // Assuming YUYV format:
            if (j % 2 == 0) {
                img_yuyv.at<cv::Vec2b>(i, j) = cv::Vec2b(y, uv);  // Y component with U/V
            } else {
                img_yuyv.at<cv::Vec2b>(i, j) = cv::Vec2b(y, uv);  // Y component only
            }
        }
    }

    // Convert the YUYV422 image to BGR (color) format for saving as JPG
    cv::Mat img_bgr;
    cv::cvtColor(img_yuyv, img_bgr, cv::COLOR_YUV2BGR_YUYV);

    // Save the image as JPG
    cv::imwrite(outputFilename + ".jpg", img_bgr);
}

/* HOT SPOT */

float calculateHotspotAverage(const std::vector<std::vector<float>> &temperatureMatrix, const Config &config) {
    std::vector<std::vector<float>>::size_type startX = config.hotspot_x;
    std::vector<std::vector<float>>::size_type startY = config.hotspot_y;
    std::vector<std::vector<float>>::size_type size = config.hotspot_size;

    float sum = 0.0;
    int count = 0;

    // Sum the temperatures in the square region
    for (std::vector<std::vector<float>>::size_type i = startY; i < startY + size && i < temperatureMatrix.size(); ++i) {
        for (std::vector<std::vector<float>>::size_type j = startX; j < startX + size && j < temperatureMatrix[i].size(); ++j) {
            sum += temperatureMatrix[i][j];
            ++count;
        }
    }

    if (count == 0) return 0.0f;  // Avoid division by zero

    return sum / count;  // Return the average temperature
}

void compareTemperatureWithThreshold(float averageTemp, const Config &config) {
    if (averageTemp >= config.hotspot_temp_threshold) {
        std::cout << "1" << std::endl;  // Hot spot is above the threshold
    } else {
        std::cout << "0" << std::endl;  // Hot spot is below the threshold
    }
}
// Function to remove the file extension from a filename
std::string removeFileExtension(const std::string &filename) {
    size_t lastDot = filename.find_last_of(".");
    if (lastDot == std::string::npos) return filename;
    return filename.substr(0, lastDot);
}

int main(int argc, char *argv[]) {
    // Check if the user provided a filename argument
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_filename>" << std::endl;
        return 1;
    }

    // Get the input filename and remove its extension
    std::string inputFilename = argv[1];
    std::string baseFilename = removeFileExtension(inputFilename);

    // Read configuration data
    Config config = readConfig("config.txt");

    // Variables to store image and temperature data
    std::vector<uint16_t> imageData, temperatureData;
    int rows, cols;

    // Load data from the binary file
    if (!loadDataFromFile(inputFilename, imageData, temperatureData, rows, cols)) {
        return 1;
    }

    // Convert the temperature data to a temperature matrix in Celsius
    std::vector<std::vector<float>> temperatureMatrix = convertToTemperature(temperatureData, rows, cols);

    // Convert the temperature matrix to an image and save as PNG
    convertTemperatureToImage(temperatureMatrix, baseFilename, config);

    // Save the temperature matrix to a file named "temperature_data.csv"
    saveTemperatureMatrixToFile(temperatureMatrix, baseFilename);

    // Convert the raw image data to a YUYV422 image and save as JPG
    convertImageDataToImage(imageData, rows, cols, baseFilename);

    // Calculate the average temperature in the hot spot
    float averageTemp = calculateHotspotAverage(temperatureMatrix, config);

    // Compare the average temperature with the threshold and print result
    compareTemperatureWithThreshold(averageTemp, config);

    return 0;
}
