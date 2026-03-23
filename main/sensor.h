#ifndef SENSOR_H
#define SENSOR_H

#include "driver/i2s_std.h"

extern i2s_chan_handle_t rx_handle;

void mic_init(void);
#include <stddef.h>
#define SAMPLE_BUFFER_SIZE 512
int sensor_get_latest_volume_str(char *buf, size_t bufsize);
void sensor_task_start(void);

#endif // SENSOR_H