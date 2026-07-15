#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H
#define MAX_LED_COUNT 1

typedef struct {
    int led_pin;
    int timer_down;
    int timer_up;
    int occurrences; // Number of times the timer has been triggered
} led_timer_message_t;

void init_led(int led_pin);
void led_on(int led_pin);
void led_off(int led_pin);
void stop_led_processing_task();

QueueHandle_t get_led_timer_queue();
void led_processing_task(void *pvParameters);

#endif // LED_CONTROLLER_H