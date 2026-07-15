#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "relay_controller.h"


#ifdef NUM_RELAYS_1
#define RELAY_COUNT 1

typedef enum {
    RELAY_1 = 0,
} relay_num_t;


// Define Relay pin numbers
int relay_pins[RELAY_COUNT] = {
    1, // GPIO for RELAY_1
};

// Define Relay states by number
int relay_states[RELAY_COUNT] = {
    0, // State for RELAY_1 (0 = OFF, 1 = ON)
};

// Relay commands
#define RELAY_CMD_ON     0x01
#define RELAY_CMD_OFF    0x00

#endif

#ifdef NUM_RELAYS_4 

#define RELAY_COUNT 4

typedef enum {
    RELAY_1 = 0,
    RELAY_2,
    RELAY_3,
    RELAY_4
} relay_num_t;


// Define Relay pin numbers
int relay_pins[RELAY_COUNT] = {
    23, // GPIO for RELAY_1
    5,  // GPIO for RELAY_2
    4,  // GPIO for RELAY_3
    13 // GPIO for RELAY_4
};

// Define Relay states by number
int relay_states[RELAY_COUNT] = {
    0, // State for RELAY_1 (0 = OFF, 1 = ON)
    0, // State for RELAY_2 (0 = OFF, 1 = ON)
    0, // State for RELAY_3 (0 = OFF, 1 = ON)
    0  // State for RELAY_4 (0 = OFF, 1 = ON)
};

// Relay commands
#define RELAY_CMD_ON     0x00
#define RELAY_CMD_OFF    0x01

#endif

typedef struct {
    relay_timer_message_t msg;
    int relay_index;   // 0-based index for handle map cleanup
} relay_task_ctx_t;

static const char *TAG = "RELAY_CONTROLLER";

QueueHandle_t relay_message_queue = NULL;
static SemaphoreHandle_t relay_task_lock = NULL;
static TaskHandle_t relay_task_handles[RELAY_COUNT] = { 0 };
static relay_task_ctx_t relay_task_ctx[RELAY_COUNT];

// Initialize relay control
void relay_init(){
    for (int i = 0; i < RELAY_COUNT; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << relay_pins[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
        gpio_set_level(relay_pins[i], RELAY_CMD_OFF); // Ensure all relays are off at initialization
        ESP_LOGI(TAG, "Relay %d initialized on GPIO %d", i + 1, relay_pins[i]);
    }
    relay_message_queue = xQueueCreate(20, sizeof(relay_timer_message_t));
    xTaskCreate(relay_processing_task, "relay_processing_task", 4096, NULL, 4, NULL);
    if (relay_message_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create relay_message_queue");
        return;
    }

    relay_task_lock = xSemaphoreCreateMutex();
    if (relay_task_lock == NULL) {
        ESP_LOGE(TAG, "Failed to create relay_task_lock");
        return;
    }

    BaseType_t ok = xTaskCreatePinnedToCore(
        relay_processing_task,
        "relay_processing_task",
        4096,
        NULL,
        4,
        NULL,
        0
    );
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed creating relay_processing_task");
    }
}

// Enable relay
void relay_on(int relay_num){
    gpio_set_level(relay_pins[relay_num - 1], RELAY_CMD_ON);
}   

// Disable relay
void relay_off(int relay_num){
    gpio_set_level(relay_pins[relay_num - 1], RELAY_CMD_OFF); 
}

// Toggle relay state
int relay_toggle(int relay_num) {
    // Validate relay number
    if (relay_num < 1 || relay_num > RELAY_COUNT) return -1;
    int index = relay_num - 1; // Convert to 0-based index
    // Set GPIO level and update state accordingly
    if (relay_states[index] == 0) {
        gpio_set_level(relay_pins[index], RELAY_CMD_ON);
        relay_states[index] = 1; // Update state to ON
    } else {
        gpio_set_level(relay_pins[index], RELAY_CMD_OFF);
        relay_states[index] = 0; // Update state to OFF
    }
    return relay_states[index]; // Return new state
}

// Relay timer
void relay_timer(int relay_num, int timer_up, int timer_down, int occurrences) {
    for (int i = 0; i < occurrences; i++) {
        // Turn on the relay
        relay_on(relay_num);
        // Wait for the specified "up" time
        vTaskDelay(timer_up / portTICK_PERIOD_MS);
        // Turn off the relay
        relay_off(relay_num);
        // Wait for the specified "down" time before allowing next command
        vTaskDelay(timer_down / portTICK_PERIOD_MS);
    }
}

// Handling task for relay timer messages

QueueHandle_t get_relay_timer_queue() {
    return relay_message_queue;
}

esp_err_t get_relay_timer_queue_message(relay_timer_message_t *msg) {
    if (xQueueReceive(relay_message_queue, msg, portMAX_DELAY) == pdTRUE) {
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

void relay_timer_task(void *pvParameters) {
    relay_task_ctx_t *ctx = (relay_task_ctx_t *)pvParameters;  // Cast void* back to relay_timer_message_t*
    relay_timer(ctx->msg.relay_number, ctx->msg.timer_up, ctx->msg.timer_down, ctx->msg.occurrences);
    if (relay_task_lock != NULL) {
        xSemaphoreTake(relay_task_lock, portMAX_DELAY);
        relay_task_handles[ctx->relay_index] = NULL;
        xSemaphoreGive(relay_task_lock);
    } else {
        relay_task_handles[ctx->relay_index] = NULL;
    }
    vTaskDelete(NULL);  // Delete the task when done
}

void relay_processing_task(void *pvParameters) {
    // In the future, this task will be taken out to the main application for modularity
    relay_timer_message_t msg;
    while (1) {
        if (get_relay_timer_queue_message(&msg) != ESP_OK) {
            continue;
        }
        // Check for new messages from the relay timer task
        if (msg.relay_number < 1 || msg.relay_number > RELAY_COUNT) {
            ESP_LOGW(TAG, "Invalid relay_number=%d", msg.relay_number);
            continue;
        }

        // Handle the relay timer message
        int idx = msg.relay_number - 1;

        xSemaphoreTake(relay_task_lock, portMAX_DELAY);
        if (relay_task_handles[idx] != NULL) {
            xSemaphoreGive(relay_task_lock);
            ESP_LOGW(TAG, "Relay %d timer task already running, skipping new command", msg.relay_number);
            continue; // Skip if a timer task is already running for this relay
        }
        else {
            relay_task_ctx[idx].msg = msg;
            relay_task_ctx[idx].relay_index = idx;
            xSemaphoreGive(relay_task_lock);

            char task_name[12]; //relay_01, relay_02, etc up to 2 digits
            snprintf(task_name, sizeof(task_name), "relay_%02d", msg.relay_number);

            BaseType_t ok = xTaskCreatePinnedToCore(
                relay_timer_task,
                task_name,
                4096,
                &relay_task_ctx[idx],
                4,
                &relay_task_handles[idx],
                0
            );

            if (ok != pdPASS) {
                xSemaphoreTake(relay_task_lock, portMAX_DELAY);
                relay_task_handles[idx] = NULL;
                xSemaphoreGive(relay_task_lock);
                ESP_LOGE(TAG, "Failed creating task for relay %d", msg.relay_number);
            }
        }
        
    }
}