#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "version.h"

static char* TAG = "VER";

void show_version() {
    ESP_LOGI(TAG, "Starting application in version %s", SOFT_VERSION);
}

int get_version_string(char* buffer, size_t buffer_size) {
    if (buffer_size < VERSION_STRING_SIZE) {
        ESP_LOGW(TAG, "Buffer size is too small for version string");
        return -1;
    }
    snprintf(buffer, buffer_size, "%s", SOFT_VERSION);
    return 0;
}