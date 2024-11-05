#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "driver/i2c.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <inttypes.h>

#define CONFIG_SDA_GPIO 1
#define CONFIG_SCL_GPIO 2
#define CONFIG_RESET_GPIO 0

#define BTN_UP GPIO_NUM_14

xQueueHandle BTN_UPQueue;

uint64_t pre_time = 0;
uint64_t intr_time = 0;
uint64_t curr_time = 0;
int lastState = 1;
int currentState;
bool long_press_detected = false;



void BTN_UPTask(void *params)
{
    gpio_set_direction(BTN_UP, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN_UP, GPIO_INTR_NEGEDGE);
    int BTN_NUMBER = 0;

    while(1)
    {
        if(xQueueReceive(BTN_UPQueue, &BTN_NUMBER, portMAX_DELAY))
        {

            long_press_detected = false; // Reset long press flag
            
            // Wait for long press or button release
            while(gpio_get_level(BTN_UP) == 0 && !long_press_detected)
            {
                curr_time = esp_timer_get_time();
                
                if (curr_time - intr_time >= 1000000) // Check for long press duration
                {
                    ESP_LOGI("NO TAG","Long Press Detected");
                    long_press_detected = true; // Set long press flag
                }

                if(gpio_get_level(BTN_UP) == 1)
                {
                    //ESP_LOGI("NO TAG","level 1 Detected");
                    if(curr_time - intr_time <1000000)
                    {
                        ESP_LOGI("NO TAG","Short Press Detected");
                        long_press_detected = true;

                    }
                }

                //vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            
            xQueueReset(BTN_UPQueue);
            //vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}

static void IRAM_ATTR BTN_UP_interrupt_handler(void *args)
{
    int pinNumber = (int)args;
    
    if(esp_timer_get_time() - pre_time > 400000)
    {
        xQueueSendFromISR(BTN_UPQueue, &pinNumber, NULL);
        intr_time = esp_timer_get_time();
    }
    
    pre_time = esp_timer_get_time();
}

void app_main(void)
{
    BTN_UPQueue = xQueueCreate(10, sizeof(int));

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_UP, BTN_UP_interrupt_handler, (void *)BTN_UP);

    xTaskCreate(BTN_UPTask, "BTN_Task", 2048, NULL, 1, NULL);
}
