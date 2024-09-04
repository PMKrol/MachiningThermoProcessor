# MachiningThermoProcessor
A tool for analyzing .tc0 files to extract and process temperature data for scientific research. It converts raw data into thermal images, annotates key regions, calculates average temperatures in specified areas, and exports results in various formats.

## Chat GPT Generated description (mainly accurate ;))

MachiningThermoProcessor is a comprehensive tool designed for processing and analyzing thermal images in machining environments. It performs several key functions:

- Data Loading and Parsing: Loads binary data files containing both image and temperature data, extracting and organizing this data into usable formats.
- Temperature Conversion: Converts raw temperature data into Celsius and scales it to generate images representing thermal distributions. The tool produces both 16-bit and 8-bit grayscale images, with one version linearly scaled to fit within the minimum and maximum temperature range.
- Image Annotation: Overlays critical regions, such as drill positions and hot spots, onto the thermal images. The drill and hot spot regions are outlined with clear frames to facilitate visual analysis.
- Temperature Analysis: Computes the average temperature within specified regions of interest, such as hot spots, and compares it to a predefined threshold to determine if it meets certain criteria.
- Data Export: Saves the processed images and temperature matrices to files in various formats (PNG, JPG, TXT), complete with metadata for further analysis.

Features:

- Reads and parses configuration files to tailor processing to specific needs.
- Provides visual representations of temperature data with detailed annotations.
- Calculates and evaluates temperature data to identify critical thermal conditions.
- Exports data in multiple formats for comprehensive reporting and analysis.

## Description added by human:

So I have this thermal camera with VERY BAD software from chinese manufacturer it runs on Windows and it's basicly everything it does. Recording, taking a photo or anything simply does not work.  I modified other software to take images (frames as png+raw) and store it for further analysis (https://github.com/PMKrol/ThermalCamSnap). This software will be used for those analysis ;).

It is WORK IN PROGRESS currently, so no warranty %).
