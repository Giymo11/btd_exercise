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

#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#define PORT CONFIG_EXAMPLE_PORT
#define RECONNECT_DELAY_MS 5000

#define MAX_RX_SIZE 128
#define MAX_TX_SIZE 128

#define SAMPLING_FREQUENCY 100

static const int DELAY_BETWEEN_SAMPLES = 1000 / SAMPLING_FREQUENCY;

static const char *TAG = "TCP_CLIENT";
static char rx_buffer[MAX_RX_SIZE];
static char tx_buffer[MAX_TX_SIZE];

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

static int connect_to_server(const char *host_ip, int port)
{
    int sock = -1;

    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = addr_family;
    dest_addr.sin_port = htons(port);

    /* Convert IP address from string to binary form */
    if (inet_pton(addr_family, host_ip, &dest_addr.sin_addr) != 1)
    {
        ESP_LOGE(TAG, "Invalid IP address format");
        return -1;
    }

    sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }
    ESP_LOGI(TAG, "Socket created, attempting to connect to %s:%d", host_ip, port);

    /* Connect to server */
    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        close(sock);
        return -1;
    }

    ESP_LOGI(TAG, "Successfully connected to server");
    return sock;
}

static int send_message(int sock, const char *message)
{
    int err = send(sock, message, strlen(message), 0);
    if (err < 0)
    {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        return -1;
    }
    return err;
}

static int receive_response(int sock, char *buffer, size_t buffer_size)
{
    int len = recv(sock, buffer, buffer_size - 1, 0);

    if (len < 0)
    {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
        return -1;
    }
    else if (len == 0)
    {
        ESP_LOGW(TAG, "Connection closed by server");
        return 0;
    }
    else
    {
        buffer[len] = '\0'; // Null-terminate the received data
        ESP_LOGI(TAG, "Received %d bytes: %s", len, buffer);
        return len;
    }
}

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

static int send_orientation(int sock, device_orientation_t current_orientation)
{
    snprintf(tx_buffer, MAX_TX_SIZE, "%s", orientation_names[current_orientation]);

    if (send_message(sock, tx_buffer) < 0)
    {
        return -1;
    }
    return receive_response(sock, rx_buffer, MAX_RX_SIZE);
}

static int task_one(int sock)
{
    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay
    int random_num = esp_random();

    /* Use snprintf instead of itoa for safer, more portable conversion */
    snprintf(tx_buffer, MAX_TX_SIZE, "%d", random_num);

    if (send_message(sock, tx_buffer) < 0)
    {
        return -1;
    }

    return receive_response(sock, rx_buffer, MAX_RX_SIZE);
}

static int task_two(int sock)
{
    static int64_t last_timestamp = 0;
    float ax, ay, az;
    int64_t timestamp;
    int diff;

    if (last_timestamp == 0)
    {
        last_timestamp = esp_timer_get_time();
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // .1 second delay
    getAccelData(&ax, &ay, &az);

    timestamp = esp_timer_get_time(); // Get current time in microseconds
    diff = (timestamp - last_timestamp) / 1000;
    ESP_LOGI(TAG, "acceleration data: %f %f %f, timestamp: %lld, timediff: %d ms", ax, ay, az, timestamp, diff);

    last_timestamp = timestamp;
    return 1;
}

static int task_three(int sock)
{
    static TickType_t last_wake_time = 0;

    int64_t timestamp;
    device_orientation_t current_orientation;
    float ax, ay, az;

    if (last_wake_time == 0)
    {
        last_wake_time = xTaskGetTickCount();
    }

    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));
    timestamp = esp_timer_get_time() / 1000;

    getAccelData(&ax, &ay, &az);
    ESP_LOGI(TAG, "acceleration data: %f %f %f, timestamp: %lld", ax, ay, az, timestamp);

    current_orientation = detect_orientation(ax, ay, az);
    return send_orientation(sock, current_orientation);
}

static int task_four(int sock)
{
    static TickType_t last_wake_time = 0;
    static device_orientation_t last_orientation = ORIENTATION_UNDEFINED;

    int64_t timestamp;
    device_orientation_t current_orientation;
    float ax, ay, az;

    if (last_wake_time == 0)
    {
        last_wake_time = xTaskGetTickCount();
    }

    // delay exactly the right amount, and update last_wake_time
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));
    timestamp = esp_timer_get_time() / 1000;

    getAccelData(&ax, &ay, &az);
    ESP_LOGI(TAG, "acceleration data: %f %f %f, timestamp: %lld", ax, ay, az, timestamp);

    current_orientation = detect_orientation(ax, ay, az);
    if (current_orientation != last_orientation)
    {
        last_orientation = current_orientation;
        return send_orientation(sock, current_orientation);
    }
    return 0;
}

static int send_accel_data(int sock)
{
    static int start_time = 0;
    float ax, ay, az;
    int64_t timestamp;

    if (start_time == 0)
        start_time = esp_timer_get_time() / 1000;
     
    timestamp = esp_timer_get_time() / 1000;
    if (timestamp > start_time + 60 * 1000) // end after 60 seconds
    {
        snprintf(tx_buffer, MAX_TX_SIZE, "end");
        send_message(sock, tx_buffer);
        receive_response(sock, rx_buffer, MAX_RX_SIZE);
        return -1;
    }

    getAccelData(&ax, &ay, &az);
    ESP_LOGI(TAG, "acceleration data: %f %f %f, timestamp: %lld", ax, ay, az, timestamp);

    snprintf(tx_buffer, MAX_TX_SIZE, "%f, %f, %f, ", ax, ay, az);
    return send_message(sock, tx_buffer);
}

static int exercise_two(int sock, bool collecting) 
{
    static TickType_t last_wake_time = 0;

    if (last_wake_time == 0)
    {
        last_wake_time = xTaskGetTickCount();
    }

    // delay exactly the right amount, and update last_wake_time
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(DELAY_BETWEEN_SAMPLES));
    if (collecting)
        return send_accel_data(sock);

    float ax, ay, az;
    getAccelData(&ax, &ay, &az);
    float magnitude = sqrt(ax*ax + ay*ay + az*az);
    



    return 0;
}

void tcp_client(void)
{
    char host_ip[] = HOST_IP_ADDR;
    int sock = -1;
    bool connected = false;

    // init accelerometer
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    init_mpu6886();
    ESP_LOGI(TAG, "MPU6866 intialized successfully");

    ESP_LOGI(TAG, "TCP client task starting...");
    while (1)
    {
        if (!connected)
        {
            sock = connect_to_server(host_ip, PORT);
            if (sock >= 0)
            {
                connected = true;
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(RECONNECT_DELAY_MS)); // Wait before retrying
                continue;
            }
        }

        int task_res = exercise_two(sock, false);
        if (task_res < 0)
        {
            connected = false;
            close(sock);
            break;
        }
    }


    if (sock != -1)
    {
        ESP_LOGI(TAG, "Shutting down socket...");
        shutdown(sock, 0);
        close(sock);
    }
}
