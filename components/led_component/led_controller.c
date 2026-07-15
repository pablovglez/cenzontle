#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "led_controller.h"

// LED commands
#define LED_CMD_ON     0x00
#define LED_CMD_OFF    0x01

static const char *TAG = "LED_CONTROLLER";

typedef struct {
    led_timer_message_t msg;
    int led_pin;
} led_task_ctx_t;


QueueHandle_t led_message_queue = NULL;
static SemaphoreHandle_t led_task_lock = NULL;
static TaskHandle_t led_task_handles[MAX_LED_COUNT] = { 0 };
// For the moment, we will only support one LED, but this can be expanded in the future
static bool led_initialized = false;
static led_task_ctx_t led_task_ctx[MAX_LED_COUNT];

void init_led(int led_pin) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << led_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(led_pin, LED_CMD_OFF); // Ensure the led is off
    ESP_LOGD(TAG, "Led initialized on GPIO %d", led_pin);

    led_message_queue = xQueueCreate(20, sizeof(led_timer_message_t));
    if (led_message_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create led_message_queue");
        return;
    }

    led_task_lock = xSemaphoreCreateMutex();
    if (led_task_lock == NULL) {
        ESP_LOGE(TAG, "Failed to create led_task_lock");
        return;
    }

    BaseType_t ok = xTaskCreate(led_processing_task, "led_processing_task", 4096, NULL, 4, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed creating led_processing_task");
    }

    // Set the LED as initialized
    led_initialized = true;
    ESP_LOGI(TAG, "LED initialized");
}

void led_on(int led_pin){
    gpio_set_level(led_pin, LED_CMD_ON);
}

void led_off(int led_pin){
    gpio_set_level(led_pin, LED_CMD_OFF);
}

// LED timer
void led_timer(int led_pin, int timer_up, int timer_down, int occurrences) {
    for (int i = 0; i < occurrences; i++) {
        // Turn on the LED
        led_on(led_pin);
        // Wait for the specified "up" time
        vTaskDelay(timer_up / portTICK_PERIOD_MS);
        // Turn off the LED
        led_off(led_pin);
        // Wait for the specified "down" time before allowing next command
        vTaskDelay(timer_down / portTICK_PERIOD_MS);
    }
}

QueueHandle_t get_led_timer_queue() {
    return led_message_queue;
}

esp_err_t get_led_timer_queue_message(led_timer_message_t *msg) {
    if (xQueueReceive(led_message_queue, msg, portMAX_DELAY) == pdTRUE) {
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

void led_timer_task(void *pvParameters) {
    led_task_ctx_t *ctx = (led_task_ctx_t *)pvParameters;  // Cast void* back to led_timer_message_t*
    led_timer(ctx->msg.led_pin, ctx->msg.timer_up, ctx->msg.timer_down, ctx->msg.occurrences);
    if (led_task_lock != NULL) {
        xSemaphoreTake(led_task_lock, portMAX_DELAY);
        led_task_handles[ctx->led_pin] = NULL;
        xSemaphoreGive(led_task_lock);
    } else {
        led_task_handles[ctx->led_pin] = NULL;
    }
    vTaskDelete(NULL);  // Delete the task when done
}


void led_processing_task(void *pvParameters) {
    // In the future, this task will be taken out to the main application for modularity
    led_timer_message_t msg;
    while (1) {
        if (get_led_timer_queue_message(&msg) != ESP_OK) {
            continue;
        }
        // Check if the LED is initialized, otherwise return
        if (!led_initialized) {
            ESP_LOGE(TAG, "LED not initialized, skipping command");
            continue;
        }
        int idx = 0;

        // Handle the LED timer message
        if (led_task_handles[idx] != NULL) {
            xSemaphoreGive(led_task_lock);
            ESP_LOGW(TAG, "LED %d timer task already running, skipping new command", msg.led_pin);
            continue; // Skip if a timer task is already running for this LED
        }
        else {
            led_task_ctx[idx].msg = msg;
            led_task_ctx[idx].led_pin = idx;
            xSemaphoreGive(led_task_lock);

            char task_name[10]; //led_01, led_02, etc up to 2 digits
            snprintf(task_name, sizeof(task_name), "led_%02d", msg.led_pin);

            BaseType_t ok = xTaskCreate(
                led_timer_task,
                task_name,
                4096,
                &led_task_ctx[idx],
                4,
                &led_task_handles[idx]
            );

            if (ok != pdPASS) {
                xSemaphoreTake(led_task_lock, portMAX_DELAY);
                led_task_handles[idx] = NULL;
                xSemaphoreGive(led_task_lock);
                ESP_LOGE(TAG, "Failed creating task for led %d", msg.led_pin);
            }
        }
    }
}

void stop_led_processing_task() {
    if (led_task_handles[0] != NULL) {
        xSemaphoreTake(led_task_lock, portMAX_DELAY);
        vTaskDelete(led_task_handles[0]);
        led_task_handles[0] = NULL;
        xSemaphoreGive(led_task_lock);
        ESP_LOGI(TAG, "LED task stopped");
    }
}