#!/bin/bash

# Check if assets/audio directory exists
if [ ! -d "/home/dame/FactoryProtocol/assets/audios" ]; then
    echo "Directory 'assets/audio' not found!"
    exit 1
fi

echo "// Generated audio files vector:"
echo "vector<pair<string, string>> soundFiles = {"

# Get all files in assets/audio, extract name without extension, format as C++ vector entries
for file in assets/audios/*; do
    if [ -f "$file" ]; then
        # Extract filename without path
        filename=$(basename "$file")
        # Extract name without extension
        name="${filename%.*}"
        # Print in the required format
        echo "    {\"$name\", \"$file\"},"
    fi
done

echo "};"
