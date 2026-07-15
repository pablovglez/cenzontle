#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

typedef struct {
    int relay_number;
    int timer_down; 
    int timer_up;
    int occurrences; // Number of times the timer has been triggered
} relay_timer_message_t;

void relay_init();
void relay_on(int relay_num);
void relay_off(int relay_num);
int relay_toggle(int relay_num);

QueueHandle_t get_relay_timer_queue();
void relay_processing_task(void *pvParameters);

#endif // RELAY_CONTROLLER_H