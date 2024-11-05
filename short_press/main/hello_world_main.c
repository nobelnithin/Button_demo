#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"


#define BUTTON_PIN       GPIO_NUM_18
#define SHORT_PRESS_TIME 1000 // 1000 milliseconds
#define LONG_PRESS_TIME  1000 // 1000 milliseconds

bool isPressing = false;
bool isLongDetected = false;
uint64_t pressedTime = 0;
uint64_t releasedTime = 0;

void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    int level = gpio_get_level(gpio_num);
    
    if (level == 0 && !isPressing) { // button is pressed
        pressedTime = esp_timer_get_time() / 1000;
        isPressing = true;
        isLongDetected = false;
    } else if (level == 1 && isPressing) { // button is released
        releasedTime = esp_timer_get_time() / 1000;
        isPressing = false;

        uint64_t pressDuration = releasedTime - pressedTime;

        if (pressDuration < SHORT_PRESS_TIME) {
            printf("A short press is detected\n");
        }

        if (pressDuration > LONG_PRESS_TIME && !isLongDetected) {
            printf("A long press is detected\n");
            isLongDetected = true;
        }
    }
}

void button_task(void *pvParameter) {
    gpio_config_t io_conf;
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins
    io_conf.pin_bit_mask = (1ULL<<BUTTON_PIN);
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, (void*) BUTTON_PIN);

    while(1) {
        vTaskDelay(10 / portTICK_PERIOD_MS); // debounce delay
    }
}

void app_main() {
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}
