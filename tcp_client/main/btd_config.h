#ifndef BTD_CONFIG_H
#define BTD_CONFIG_H

#include <stdint.h>

typedef struct {
    uint16_t workTimeSeconds;      // Work time in seconds
    uint16_t breakTimeSeconds;     // Break time in seconds
    uint16_t longBreakTimeSeconds; // Long break time in seconds
    uint16_t longBreakSessionCount;    // Interval for long breaks
    uint16_t timeoutSeconds;       // Timeout in seconds
    bool breakGestureEnabled;      // Break gesture on/off
} btd_config_t;

static const btd_config_t DEFAULT_CONFIG = {
    .workTimeSeconds = 25 * 60,          // 25 minutes
    .breakTimeSeconds = 5 * 60,          // 5 minutes
    .longBreakTimeSeconds = 15 * 60,     // 15 minutes
    .longBreakSessionCount = 3,          // Every 3 work sessions
    .timeoutSeconds = 0,                 // No timeout by default
    .breakGestureEnabled = true           // Break gesture enabled by default
};

/*
* @brief Reads the configuration from NVS.
* 
* This function reads the configuration from the NVS storage. If no configuration is found,
* it initializes the config with default values and saves it.
* 
* @param config Pointer to the btd_config_t structure to store the configuration.
* @return ESP_OK on success, or an error code on failure.
*/
esp_err_t btd_read_config(btd_config_t *config);

/*
* @brief Saves the configuration to NVS.
* 
* This function saves the provided configuration to the NVS storage.
* @param config Pointer to the btd_config_t structure containing the configuration to save.
* @return ESP_OK on success, or an error code on failure.
*/
esp_err_t btd_save_config(const btd_config_t *config);

/* * @brief Deletes all data from NVS.
* 
* This function erases the NVS storage to reset the configuration to factory defaults.
* It also reinitializes the NVS.
* 
* @return ESP_OK on success, or an error code on failure.
*/
esp_err_t btd_delete_config(void);

#endif // BTD_CONFIG_H