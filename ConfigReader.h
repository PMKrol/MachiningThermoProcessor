#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <string>

// Struktura Config zawierająca wszystkie konfiguracje
struct Config {
    int drill_start_x;
    int drill_start_y;
    int drill_end_x;
    int drill_end_y;
    int drill_width;            // Dodaj drill_width tutaj
    int hotspot_x;
    int hotspot_y;
    int hotspot_size;
    float hotspot_temp_threshold;
};

// Deklaracja funkcji readConfig
Config readConfig(const std::string &filename);

#endif // CONFIGREADER_H
