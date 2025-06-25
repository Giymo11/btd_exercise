#ifndef BTDS_STATS_H
#define BTDS_STATS_H

#include <stdint.h> 
#include "esp_err.h"

typedef struct 
{
    uint32_t session_id;        // Unique identifier for the session
    uint32_t duration_seconds;  // Duration of the session in seconds
    char name[32];              // Name of the location
    uint8_t mic_level;          // Microphone level during the session (0-100)
} session_stats_t;


esp_err_t record_work_session(session_stats_t *stats);
esp_err_t get_all_work_sessions(session_stats_t *sessions, size_t *count);

#endif // BTDS_STATS_H
