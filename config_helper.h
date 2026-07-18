#ifndef CONFIG_HELPER_H
#define CONFIG_HELPER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define CONFIG_FILE_NAME "config.ini"
#define LINE_BUFFER_SIZE 256

/**
 * @brief Saves the operational Vendor ID and Product ID to config.ini.
 * 
 * @param vid The hardware Vendor ID.
 * @param pid The hardware Product ID.
 * @return true if written successfully, false otherwise.
 */
bool saveConfigIDs(int vid, int pid) {
    FILE *file = fopen(CONFIG_FILE_NAME, "w");
    if (!file) {
        perror("Warning: Could not open config.ini for writing");
        return false;
    }

    // Write a clean, human-readable INI format
    fprintf(file, "[Device]\n");
    fprintf(file, "vendor_id = 0x%04X\n", vid);
    fprintf(file, "product_id = 0x%04X\n", pid);
    
    fclose(file);
    return true;
}

/**
 * @brief Reads the hardware identity tags from config.ini if it exists.
 * 
 * @param vid Pointer to store the extracted Vendor ID.
 * @param pid Pointer to store the extracted Product ID.
 * @return true if both IDs were parsed successfully, false otherwise.
 */
bool loadConfigIDs(int *vid, int *pid) {
    FILE *file = fopen(CONFIG_FILE_NAME, "r");
    if (!file) {
        return false; // File doesn't exist or isn't readable
    }

    char line[LINE_BUFFER_SIZE];
    int parsed_vid = -1;
    int parsed_pid = -1;
    bool has_device_section = false;

    while (fgets(line, sizeof(line), file)) {
        // Simple section heading verification
        if (strstr(line, "[Device]")) {
            has_device_section = true;
            continue;
        }

        // Use sscanf to grab hex or standard integers safely
        if (strstr(line, "vendor_id")) {
            sscanf(line, "vendor_id = %i", &parsed_vid);
        } else if (strstr(line, "product_id")) {
            sscanf(line, "product_id = %i", &parsed_pid);
        }
    }

    fclose(file);

    // Validate that we actually found correct values
    if (has_device_section && parsed_vid != -1 && parsed_pid != -1) {
        *vid = parsed_vid;
        *pid = parsed_pid;
        return true;
    }

    return false;
}

#endif