#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"

#include "axp192.h"
#include "sgm2578.h"
#include "st7789.h"
#include "fontx.h"
#include "bmpfile.h"
#include "decode_jpeg.h"
#include "decode_png.h"
#include "pngle.h"

#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)

// M5stickC-Plus stuff
#if CONFIG_M5STICK_C_PLUS
#define CONFIG_WIDTH 135
#define CONFIG_HEIGHT 240
#define CONFIG_MOSI_GPIO 15
#define CONFIG_SCLK_GPIO 13
#define CONFIG_CS_GPIO 5
#define CONFIG_DC_GPIO 23
#define CONFIG_RESET_GPIO 18
#define CONFIG_BL_GPIO -1
#define CONFIG_LED_GPIO 10
#define CONFIG_OFFSETX 52
#define CONFIG_OFFSETY 40
#endif

// M5stickC-Plus2 stuff
#if CONFIG_M5STICK_C_PLUS2
#define CONFIG_WIDTH 135
#define CONFIG_HEIGHT 240
#define CONFIG_MOSI_GPIO 15
#define CONFIG_SCLK_GPIO 13
#define CONFIG_CS_GPIO 5
#define CONFIG_DC_GPIO 14
#define CONFIG_RESET_GPIO 12
#define CONFIG_BL_GPIO -1
#define CONFIG_LED_GPIO 19
#define CONFIG_OFFSETX 52
#define CONFIG_OFFSETY 40
#endif

static const char *TAG = "MAIN";

static void listSPIFFS(char *path)
{
    DIR *dir = opendir(path);
    assert(dir != NULL);
    while (true)
    {
        struct dirent *pe = readdir(dir);
        if (!pe)
            break;
        ESP_LOGI(__FUNCTION__, "d_name=%s d_ino=%d d_type=%x", pe->d_name, pe->d_ino, pe->d_type);
    }
    closedir(dir);
}

esp_err_t mountSPIFFS(char *path, char *label, int max_files)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = path,
        .partition_label = label,
        .max_files = max_files,
        .format_if_mount_failed = true};

    // Use settings defined above toinitialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is anall-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

#if 0
	ESP_LOGI(TAG, "Performing SPIFFS_check().");
	ret = esp_spiffs_check(conf.partition_label);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
		return ret;
	} else {
			ESP_LOGI(TAG, "SPIFFS_check() successful");
	}
#endif

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Mount %s to %s success", path, label);
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret;
}

void test_screen(void *pvParameters)
{

#if CONFIG_M5STICK_C_PLUS
    // power on
    AXP192_PowerOn();
    AXP192_ScreenBreath(11);
#endif

#if CONFIG_M5STICK_C_PLUS2
// power hold
#define POWER_HOLD_GPIO 4
    gpio_reset_pin(POWER_HOLD_GPIO);
    gpio_set_direction(POWER_HOLD_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(POWER_HOLD_GPIO, 1);
// Enable SGM2578. VLED is supplied by SGM2578
#define SGM2578_ENABLE_GPIO 27
    sgm2578_Enable(SGM2578_ENABLE_GPIO);
#endif

    TFT_t dev;
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
    lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY, true);

    while (1)
    {
        lcdFillScreen(&dev, RED);
        lcdDrawFinish(&dev);
        ESP_LOGI(__FUNCTION__, "red");
        WAIT;
        lcdFillScreen(&dev, GREEN);
        lcdDrawFinish(&dev);
        WAIT;
        lcdFillScreen(&dev, BLUE);
        lcdDrawFinish(&dev);
        WAIT;
        lcdFillScreen(&dev, BLACK);
        lcdDrawFinish(&dev);
        WAIT;
        lcdDrawCircle(&dev, CONFIG_WIDTH / 2, CONFIG_HEIGHT / 2, 30, BLUE);
        lcdDrawFinish(&dev);
        WAIT;
        lcdDrawFillCircle(&dev, CONFIG_WIDTH / 2, CONFIG_HEIGHT / 2, 10, RED);
        lcdDrawFinish(&dev);
        WAIT;
    }
}

void app_main(void)
{
    // Mount SPIFFS File System on FLASH
    ESP_LOGI(TAG, "Initializing SPIFFS");
    ESP_ERROR_CHECK(mountSPIFFS("/fonts", "storage1", 8));
    listSPIFFS("/fonts");

    ESP_ERROR_CHECK(mountSPIFFS("/images", "storage2", 1));
    listSPIFFS("/images");

    ESP_ERROR_CHECK(mountSPIFFS("/icons", "storage3", 1));
    listSPIFFS("/icons");

    // Initialize i2c
    i2c_master_init();
    
	xTaskCreate(test_screen, "test", 1024*6, NULL, 2, NULL);
}
