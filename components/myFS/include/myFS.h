#ifndef MYFS_H
#define MYFS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration parameters structure
 */
typedef struct {
    char wifi_ssid[32];
    char wifi_pwd[32];
    int registers;
} params_t;

/**
 * @brief Save parameters to JSON configuration file
 * @param p Pointer to parameters structure
 * @return true on success, false on failure
 */
bool save_params(const params_t *p);

/**
 * @brief Load parameters from JSON configuration file
 * @param p Pointer to parameters structure to fill
 * @return true on success, false on failure
 */
bool load_params(params_t *p);

/**
 * @brief Initialize file system and load configuration
 *
 * This function initializes the NVS flash and mounts the LittleFS filesystem.
 * It then attempts to load configuration from a JSON file, creating default
 * configuration if the file doesn't exist.
 */
void FS_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* MYFS_H */