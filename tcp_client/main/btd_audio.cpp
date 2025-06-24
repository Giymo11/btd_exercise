#include "driver/i2s.h"
#include "esp_log.h"

static const char *TAG = "BTD_MICROPHONE_AUDIO";

static const float AUDIO_THRESHOLD = 2500; // normal quietness at about 1700

static bool currently_loud = false;
static int64_t loud_start_time_ms = 0;
static int64_t total_loud_duration_ms = 0; 

#define I2S_MIC_PORT I2S_NUM_0
#define I2S_MIC_SERIAL_CLK 26  
#define I2S_MIC_DATA       34  
#define SAMPLE_RATE        44100
#define BUFFER_LEN         1024

void init_microphone() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB, 
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_MIC_SERIAL_CLK,
        .ws_io_num = -1,  
        .data_out_num = -1,
        .data_in_num = I2S_MIC_DATA
    };

    i2s_driver_install(I2S_MIC_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_MIC_PORT, &pin_config);
}

int read_microphone_sample() {
    int16_t buffer[BUFFER_LEN];
    size_t bytes_read;

    i2s_read(I2S_MIC_PORT, (char *)buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);
    int samples_read = bytes_read / sizeof(int16_t);

    int64_t sum = 0;
    for (int i = 0; i < samples_read; i++) {
        sum += abs(buffer[i]);
    }

    int average = sum / samples_read;
    return average;
}
 
bool is_volume_above_threshold(int64_t current_time_ms) {
    int sample = read_microphone_sample();
    bool loud = abs(sample) > AUDIO_THRESHOLD;

    if (loud && !currently_loud) {
        loud_start_time_ms = current_time_ms;
        currently_loud = true;
    } else if (!loud && currently_loud) { // <- end of disturbance
        int64_t duration = current_time_ms - loud_start_time_ms;
        total_loud_duration_ms += duration;
        currently_loud = false;
        loud_start_time_ms = 0;
    }
    return loud;
}

float get_loud_percentage(int64_t session_start_ms, int64_t session_end_ms) {
    int64_t session_duration_ms = session_end_ms - session_start_ms;
    if (session_duration_ms == 0) return 0.0f;
    return (total_loud_duration_ms * 100.0f) / session_duration_ms;
}

int64_t get_total_loud_duration_ms(){
    return total_loud_duration_ms;
}
