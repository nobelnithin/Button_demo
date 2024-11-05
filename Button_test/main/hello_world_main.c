/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <string.h>
#include "esp_event.h"
#include <inttypes.h>
#include "driver/rtc_io.h"
#include "esp_timer.h"

#define TAG "SSD1306"


//Push button declarations
#define BTN_UP GPIO_NUM_11
// #define BTN_DOWN GPIO_NUM_10
// #define BTN_PLUS GPIO_NUM_9
// #define BTN_MINUS GPIO_NUM_8
// #define BTN_PWR GPIO_NUM_6
// #define BTN_OK GPIO_NUM_7

uint32_t pre_time = 0;
xQueueHandle BTN_UPQueue;

void BTN_UPTask(void *params)
{
    gpio_set_direction(BTN_UP, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN_UP, GPIO_INTR_LOW_LEVEL);
    int BTN_NUMBER = 0;
    //uint32_t time_elapsed = 0;
    while(1)
    {
        
        if(xQueueReceive(BTN_UPQueue, &BTN_NUMBER, portMAX_DELAY))
        {
 

            
            ESP_LOGI(TAG, "Task BTN_UPTask: Button UP pressed (11)n");

            xQueueReset(BTN_UPQueue);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
       //ESP_LOGI(TAG,"Time:  %lu",time);
    }
}

static void IRAM_ATTR BTN_UP_interrupt_handler(void *args)
{
    //ets_printf("ISR: Button 1 pressed!\n");

    //gpio_set_intr_type(BTN_UP,  GPIO_INTR_POSEDGE);
    int cur_time = esp_timer_get_time();
    
    if(cur_time-pre_time>500000)
    {
        int pinNumber = (int)args;
        xQueueSendFromISR(BTN_UPQueue, &pinNumber, NULL);
    }
   pre_time = esp_timer_get_time();

}


// static void IRAM_ATTR BTN_UP_interrupt_handler(void *args)
// { 
    
//     uint32_t time_elapsed = 0;
//     int pinNumber = (int)args;
//     while(1)
//     {
             
            
//             if(esp_timer_get_time()-time_elapsed < 500000)
//             {          
                       
//                        xQueueSendFromISR(BTN_UPQueue, &pinNumber, NULL);
//                        continue;
//             }
       
//            time_elapsed = esp_timer_get_time();
//            vTaskDelay(10/portTICK_PERIOD_MS);
//     }
    

//     }



// void Timer_task(void *params)
// {
//     uint32_t time_elapsed =0;
//     while(1)
//     {

    
//     if(gpio_isr_handler_add(BTN_UP, BTN_UP_interrupt_handler, (void *)BTN_UP))
//     {

//      if(esp_timer_get_time()-time_elapsed < 500000){
//         portDISABLE_INTERRUPTS();
//      }
//       time_elapsed=esp_timer_get_time();

     
     
//     }
//     portENABLE_INTERRUPTS(); 
//      vTaskDelay(10/portTICK_PERIOD_MS);
//     }
// }


void app_main(void)
{

    BTN_UPQueue = xQueueCreate(10, sizeof(int));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_UP, BTN_UP_interrupt_handler, (void *)BTN_UP);
   // xTaskCreate(Timer_task, "Interupt", 2048, NULL, 1, NULL);
    xTaskCreate(BTN_UPTask, "BTN_Task", 2048, NULL, 1, NULL);
   
   //ESP_LOGI(TAG,"Time:  %"PRIu32" ",time);
  
    
   
}
