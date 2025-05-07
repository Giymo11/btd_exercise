/*
 * ESP32 TCP Client - Improved version with CONFIG_EXAMPLE_IPV4 assumption
 */

 #include <stdio.h>  // For snprintf
 #include <stdlib.h> // Standard library functions (not strictly needed here anymore)
 
 #include <string.h>
 #include <unistd.h>
 #include <sys/socket.h>
 #include <errno.h>
 #include <netdb.h>     // struct addrinfo, gai_strerror() if using DNS names
 #include <arpa/inet.h> // inet_pton()
 #include <driver/i2c.h>
 #include <math.h>
 
 #include "sdkconfig.h" // For Kconfig options
 #include "esp_netif.h" // Networking Interface
 #include "esp_log.h"   // Logging framework
 
 #include "freertos/FreeRTOS.h" // FreeRTOS API
 #include "freertos/task.h"     // Task management
 #include "esp_random.h"        // ESP32 Random number generator"
 #include "esp_timer.h"
 
 // Configuration checks
 #if !defined(CONFIG_EXAMPLE_IPV4)
 #error "This application requires CONFIG_EXAMPLE_IPV4 to be defined"
 #endif
 
 #define I2C_MASTER_SCL_IO 22        /*!< GPIO number used for I2C master clock */
 #define I2C_MASTER_SDA_IO 21        /*!< GPIO number used for I2C master data  */
 #define I2C_MASTER_NUM 0            /*!< I2C master i2c port number */
 #define I2C_MASTER_FREQ_HZ 400000   /*!< I2C master clock frequency */
 #define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
 #define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
 #define I2C_MASTER_TIMEOUT_MS 1000
 
 #define MPU6886_SENSOR_ADDR 0x68       /*!< Slave address of the MPU6866 sensor */
 #define MPU6886_WHO_AM_I_REG_ADDR 0x75 /*!< Register addresses of the "who am I" register */
 #define MPU6886_SMPLRT_DIV_REG_ADDR 0x19
 #define MPU6886_CONFIG_REG_ADDR 0x1A
 #define MPU6886_ACCEL_CONFIG_REG_ADDR 0x1C
 #define MPU6886_ACCEL_CONFIG_2_REG_ADDR 0x1D
 #define MPU6886_FIFO_EN_REG_ADDR 0x23
 #define MPU6886_INT_PIN_CFG_REG_ADDR 0x37
 #define MPU6886_INT_ENABLE_REG_ADDR 0x38
 #define MPU6886_ACCEL_XOUT_REG_ADDR 0x3B
 #define MPU6886_USER_CRTL_REG_ADDR 0x6A
 #define MPU6886_PWR_MGMT_1_REG_ADDR 0x6B
 #define MPU6886_PWR_MGMT_2_REG_ADDR 0x6C
 
 
 static const char *TAG = "IMU";
 
 typedef enum
 {
     ORIENTATION_UNDEFINED = 0,
     ORIENTATION_NEGATIVE_X,
     ORIENTATION_POSITIVE_X,
     ORIENTATION_NEGATIVE_Y,
     ORIENTATION_POSITIVE_Y,
     ORIENTATION_NEGATIVE_Z,
     ORIENTATION_POSITIVE_Z
 } device_orientation_t;
 
 static const char *orientation_names[] = {
     "undefined",
     "-X",
     "+X",
     "-Y",
     "+Y",
     "-Z",
     "+Z"};
 
 static esp_err_t i2c_master_init(void)
 {
     int i2c_master_port = I2C_MASTER_NUM;
 
     i2c_config_t conf = {
         .mode = I2C_MODE_MASTER,
         .sda_io_num = I2C_MASTER_SDA_IO,
         .scl_io_num = I2C_MASTER_SCL_IO,
         .sda_pullup_en = GPIO_PULLUP_ENABLE,
         .scl_pullup_en = GPIO_PULLUP_ENABLE,
         .master.clk_speed = I2C_MASTER_FREQ_HZ,
     };
 
     i2c_param_config(i2c_master_port, &conf);
 
     return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
 }
 
 static esp_err_t mpu6886_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
 {
     return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6886_SENSOR_ADDR, &reg_addr, 1, data, len, 10 * I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
 }
 
 static esp_err_t mpu6886_register_write_byte(uint8_t reg_addr, uint8_t data)
 {
     int ret;
     uint8_t write_buf[2] = {reg_addr, data};
 
     ret = i2c_master_write_to_device(I2C_MASTER_NUM, MPU6886_SENSOR_ADDR, write_buf, sizeof(write_buf), 10 * I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
 
     return ret;
 }
 
 static void init_mpu6886(void)
 {
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_PWR_MGMT_1_REG_ADDR, 0x00));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_PWR_MGMT_1_REG_ADDR, (0x01 << 7)));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_PWR_MGMT_1_REG_ADDR, (0x01 << 0)));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_ACCEL_CONFIG_REG_ADDR, 0x18));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_CONFIG_REG_ADDR, 0x01));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_SMPLRT_DIV_REG_ADDR, 0x05));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_INT_ENABLE_REG_ADDR, 0x00));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_ACCEL_CONFIG_2_REG_ADDR, 0x00));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_USER_CRTL_REG_ADDR, 0x00));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_FIFO_EN_REG_ADDR, 0x00));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_INT_PIN_CFG_REG_ADDR, 0x22));
     vTaskDelay(10 / portTICK_PERIOD_MS);
     ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_INT_ENABLE_REG_ADDR, 0x01));
     vTaskDelay(10 / portTICK_PERIOD_MS);
 }
 
 static void getAccelAdc(int16_t *ax, int16_t *ay, int16_t *az)
 {
     uint8_t buf[6];
     mpu6886_register_read(MPU6886_ACCEL_XOUT_REG_ADDR, buf, 6);
 
     *ax = ((int16_t)buf[0] << 8) | buf[1];
     *ay = ((int16_t)buf[2] << 8) | buf[3];
     *az = ((int16_t)buf[4] << 8) | buf[5];
 }
 
 static void getAccelData(float *ax, float *ay, float *az)
 {
     int16_t accX = 0;
     int16_t accY = 0;
     int16_t accZ = 0;
     getAccelAdc(&accX, &accY, &accZ);
     float aRes = 16.0 / 32768.0;
 
     *ax = (float)accX * aRes;
     *ay = (float)accY * aRes;
     *az = (float)accZ * aRes;
 }

 float getAccelMagnitude(void)
 {
    float ax, ay, az;
    getAccelData(&ax, &ay, &az);
    return (float) sqrt(ax*ax + ay*ay + az*az);
 }

 
 bool is_gravity(float abs_value)
 {
     return abs_value > 0.8 && abs_value < 1.2;
 }
 
 device_orientation_t detect_orientation(float ax, float ay, float az)
 {
     // Take absolute values to simplify comparisons
     float abs_ax = fabsf(ax);
     float abs_ay = fabsf(ay);
     float abs_az = fabsf(az);
 
     // Find which axis has the dominant acceleration
     if (abs_ax > abs_ay && abs_ax > abs_az)
     { // X axis is dominant
         if (is_gravity(abs_ax))
         {
             return (ax > 0) ? ORIENTATION_POSITIVE_X : ORIENTATION_NEGATIVE_X;
         }
     }
     else if (abs_ay > abs_ax && abs_ay > abs_az)
     { // Y axis is dominant
         if (is_gravity(abs_ay))
         {
             return (ay > 0) ? ORIENTATION_POSITIVE_Y : ORIENTATION_NEGATIVE_Y;
         }
     }
     else if (abs_az > abs_ax && abs_az > abs_ay)
     { // Z axis is dominant
         if (is_gravity(abs_az))
         {
             return (az > 0) ? ORIENTATION_POSITIVE_Z : ORIENTATION_NEGATIVE_Z;
         }
     }
 
     // If no clear dominant axis or values not in threshold
     return ORIENTATION_UNDEFINED;
 }
 
 
 void init_imu(void)
 {
     // init accelerometer
    //  ESP_ERROR_CHECK(i2c_master_init());
    //  ESP_LOGI(TAG, "I2C initialized successfully");
 
     init_mpu6886();
     ESP_LOGI(TAG, "MPU6866 intialized successfully");
 }
 