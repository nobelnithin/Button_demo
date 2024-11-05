#include <stdio.h>
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
#include "ssd1306.h"
#include "font8x8_basic.h"
#include "img_resources.h"
#include <string.h>
#include "esp_timer.h"

#define TAG "SSD1306"

//I2C declarations
#define CONFIG_SDA_GPIO 1
#define CONFIG_SCL_GPIO 2
#define CONFIG_RESET_GPIO 0

//Push button declarations
#define BTN_UP GPIO_NUM_11
#define BTN_DOWN GPIO_NUM_10
#define BTN_PLUS GPIO_NUM_9
#define BTN_MINUS GPIO_NUM_8
#define BTN_PWR GPIO_NUM_6
#define BTN_OK GPIO_NUM_7

//Queue Declarations
xQueueHandle BTN_UPQueue;
xQueueHandle BTN_DOWNQueue;
xQueueHandle BTN_PLUSQueue;
xQueueHandle BTN_MINUSQueue;
xQueueHandle BTN_PWRQueue;
xQueueHandle BTN_OKQueue;
xQueueHandle DISPQueue;

uint32_t pre_time_up = 0;
uint32_t pre_time_down = 0;
uint32_t pre_time_plus = 0;
uint32_t pre_time_minus = 0;
uint32_t pre_time_pwr = 0;
uint32_t pre_time_ok = 0;

//Global Declarations
SSD1306_t dev;
int disp_state=0;
bool isDisp_logo = false;
bool isDisp_menu = false;
bool isDisp_Nav = false;
bool isDisp_str = false;
bool pause_play = false;
bool play_flag      = true;
uint64_t pre_time = 0;
uint64_t intr_time = 0;
uint64_t curr_time = 0;
int lastState = 1;
int currentState;
bool long_press_detected = false;

int freq_list[13] = {0, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70};
int timer_list[12] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20};
int str_list[10] = {1,2,3,4,5,6,7,8,9};
int str_list_index = 0;
int freq_list_index = 0;
int timer_list_index = 0;
int screen_id=0;
unsigned short minutes = 0;
unsigned short seconds = 0;
// unsigned short var_min = 0;
// unsigned short var_sec = 0;



void display_logo()
{
    isDisp_logo = true;

    ssd1306_init(&dev, 128, 64);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    ssd1306_bitmaps(&dev, 0, 0, biostim_logo, 128, 64, false);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ssd1306_clear_screen(&dev, false);
    
}

void display_menu()
{
    isDisp_logo = false;
    isDisp_menu = true;
    isDisp_Nav = false;
    isDisp_Nav = false;
    
    ssd1306_clear_screen(&dev, false);
    disp_state = 1;
    char combined_str[10]; // Assuming max length of combined string is 10
    strcpy(combined_str, "CH1");
    strcat(combined_str, " CH2");
    int frequency = freq_list[freq_list_index];
    int timer = timer_list[timer_list_index];
    char str1[4]; 
    char str2[4];
    char display_str1[20];
    char display_str2[20]; 
    snprintf(str1, sizeof(str1), "%d", frequency);
    snprintf(str2, sizeof(str2), "%d", timer);
    snprintf(display_str1, sizeof(display_str1), "Frequency   %sHz", str1);
    snprintf(display_str2, sizeof(display_str2), "Timer      %smin", str2);
    ssd1306_bitmaps(&dev, 0, 0, battery_25, 128, 64, false);
    ssd1306_display_text(&dev, 1, combined_str, strlen(combined_str), false);
    ssd1306_display_text(&dev, 3, display_str1, strlen(display_str1), true);
    ssd1306_display_text(&dev, 5, display_str2, strlen(display_str2), false);
}

void display_navigation(int id)
{
    //ets_printf("Display Navigation Frequency activated");
    isDisp_menu = false;
    isDisp_Nav = true;
    isDisp_str = false;


    if(id==4)
    {
        screen_id = 4;

        //ssd1306_clear_screen(&dev, false);
        ssd1306_bitmaps(&dev, 15, 0, navigation_timer, 128, 64, false);
        //ssd1306_display_text_x3(&dev, 3, " off",4 , false);
        ssd1306_bitmaps(&dev, 46, 26, min_var[timer_list_index], 32,16, false);
        
    }

    if(id==3)
    {

        // int num=freq_list[freq_list_index];
        // char str[3]; 
        // char display_str[5]; 
        // str[0] = '0' + (num / 10);
        // str[1] = '0' + (num % 10);
        // str[2] = '\0';
        // sprintf(display_str, " %s", str);
        screen_id=3;
        ssd1306_bitmaps(&dev, 0, 0, navigation_freq, 128, 64, false);

       // ssd1306_clear_screen(&dev, false);
        
        ssd1306_bitmaps(&dev, 46, 26, num_20[freq_list_index], 32,16, false);
        //ssd1306_display_text_x3(&dev, 3,display_str ,strlen(display_str), false);
        
    }


}

// void display_navigation_timer()
// {
//     ets_printf("Display Navigation Timer activated");
//     //isDisp_NavTimer = true;
//     isDisp_menu = false;

//     ssd1306_clear_screen(&dev, false);
//     ssd1306_bitmaps(&dev, 15, 0, navigation_timer, 128, 64, false);
//     ssd1306_display_text_x3(&dev, 3, " off",4 , false);
    
// }

void timer_loop()
{
    char lineChar[20];
    unsigned short minutes = timer_list[timer_list_index];
    unsigned short seconds = 0;

    while (minutes >= 0 && seconds >= 0 && !play_flag) {
        

        sprintf(lineChar, "%hu:%hu", minutes, seconds);
        //ssd1306_clear_screen(&dev, false);
        ssd1306_display_text(&dev, 6, lineChar, strlen(lineChar), false);
        
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ssd1306_display_text(&dev, 6,"     ", 5, false);
        
        
        if (seconds == 0) {
            if (minutes > 0) {
                minutes--;
                seconds = 59;
            } else {
                break; 
            }

        } else {
            seconds--;
        }
        // var_min = minutes;
        // var_sec = seconds;
    }

}

void str_change()
{
    
    ssd1306_bitmaps(&dev, 107, 40,str_num[str_list_index], 16,9,false);
}

void str_pause_play()
{
    ESP_LOGI("NO TAG","pause_play %d",pause_play);
    if (pause_play) 
    {
        ssd1306_bitmaps(&dev, 46, 26,pause_small , 32, 16, false);
        play_flag = false;
        timer_loop();
        
    }
    else {
        
        ssd1306_bitmaps(&dev, 46, 26,play_small, 32, 16, false);
        play_flag = true;
    }

}
void togglePlayPause() {
    pause_play = !pause_play;
    str_pause_play();
}


void display_strength()
{
    isDisp_logo = false;
    isDisp_menu = false;
    isDisp_Nav = false;
    isDisp_str = true;
    disp_state = 5;
    // disp_state = 0;
    char str1[5]; 
    char str2[6];
    int freq = freq_list[freq_list_index];
    int time = timer_list[timer_list_index];

    char combined_str[10]; // Assuming max length of combined string is 10
    strcpy(combined_str, "CH1");
    strcat(combined_str, " CH2");

    ssd1306_clear_screen(&dev, false);
    ssd1306_bitmaps(&dev, 0, 0, strength_navigation, 128, 64, false);
    ssd1306_bitmaps(&dev, 46, 26, play_small, 32, 16, false);
    ssd1306_bitmaps(&dev, 100, 0, battery_100, 32, 16, false);
    
    ssd1306_display_text(&dev, 1, combined_str, strlen(combined_str), false);

    ssd1306_bitmaps(&dev, 107, 40,str_num[0], 16,9,false);
    //ssd1306_display_text(&dev, 5, "              3", 16, false); 
    snprintf(str1, sizeof(str1), "%dHz", freq);
    snprintf(str2,sizeof(str2),"%d:00",time);
    ssd1306_display_text(&dev, 4, str1, strlen(str1), false);
    ssd1306_display_text(&dev, 6, str2, strlen(str2), false);
     
}

void highlight_menu(int index)

{   
    isDisp_Nav = false;
    int frequency = freq_list[freq_list_index];
    int timer = timer_list[timer_list_index];
    char str1[4]; 
    char str2[4];
    char display_str1[20];
    char display_str2[20];
    snprintf(str1, sizeof(str1), "%d", frequency);
    snprintf(str2, sizeof(str2), "%d", timer);
    snprintf(display_str1, sizeof(display_str1), "Frequency   %sHz", str1);
    snprintf(display_str2, sizeof(display_str2), "Timer      %smin", str2);

    if(index == 1)
    {
        disp_state = 1;
        ssd1306_display_text(&dev, 3, display_str1, strlen(display_str1), true);
       // ssd1306_display_text(&dev, 5, "Timer       OFF", 17, false);
        ssd1306_display_text(&dev, 5, display_str2, strlen(display_str2), false);
    }
    if(index == 2)
    {
        disp_state = 2;
        ssd1306_display_text(&dev, 3, display_str1, strlen(display_str1), false);
        ssd1306_display_text(&dev, 5, display_str2, strlen(display_str2), true);
    }
}

void BTN_UPTask(void *params)
{
    gpio_set_direction(BTN_UP, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN_UP, GPIO_INTR_NEGEDGE);
    int BTN_NUMBER = 0;
    while(1)
    {
        if(xQueueReceive(BTN_UPQueue, &BTN_NUMBER, portMAX_DELAY))
        {
            if(isDisp_menu)
            {
                disp_state = 1;
                xQueueSend(DISPQueue, &disp_state, portMAX_DELAY);
            }

            ESP_LOGI("NO TAG","Task BTN_UPTask: Button UP pressed (11)!");
            xQueueReset(BTN_UPQueue);
            
            
        }
    }
}

void BTN_DOWNTask(void *params)
{
    gpio_set_direction(BTN_DOWN, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN_DOWN, GPIO_INTR_NEGEDGE);
    int BTN_NUMBER = 0;
    while (1)
    {
        if(xQueueReceive(BTN_DOWNQueue, &BTN_NUMBER, portMAX_DELAY))
        {
            if(isDisp_menu)
            {
                disp_state = 2;
                xQueueSend(DISPQueue, &disp_state, portMAX_DELAY);
            }
            ESP_LOGI("NO TAG","Task BTN_DOWNTask: Button Down pressed (12)!");
            xQueueReset(BTN_DOWNQueue);
            
           
        }
    }

}

void BTN_PLUSTask(void *params)
{
    gpio_set_direction(BTN_PLUS, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN_PLUS, GPIO_INTR_NEGEDGE);
    int BTN_NUMBER = 0;
    
    while(1){
        if(xQueueReceive(BTN_PLUSQueue, &BTN_NUMBER, portMAX_DELAY))
        {
           ESP_LOGI("NO TAG","Task BTN_PLUSTask: Button Plus pressed (13)!");
           ESP_LOGI("NO TAG","Button Plus Screen Id: %d",screen_id);
           if(isDisp_Nav)
           {
              if(screen_id==3){
                freq_list_index++;
                if(freq_list_index>11)
                freq_list_index=0;
                display_navigation(3);
              }
              else if(screen_id==4)
              {
                timer_list_index++;
                if(timer_list_index>10)
                    timer_list_index=0;
                display_navigation(4);
              }
           }
           if(isDisp_str)
           {
            
            if(disp_state== 5)
            {
            if(play_flag)
            {
                str_list_index++;
                if(str_list_index>8)
                    str_list_index=0;
                str_change();
            }

            }
           }
            xQueueReset(BTN_PLUSQueue);
        }
    }

}

void BTN_MINUSTask(void *params)
{
    gpio_set_direction(BTN_MINUS, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN_MINUS, GPIO_INTR_NEGEDGE);
    int BTN_NUMBER = 0;

    while(1)
    {
        if(xQueueReceive(BTN_MINUSQueue, &BTN_NUMBER, portMAX_DELAY))
        {
            ESP_LOGI("NO TAG","Task BTN_MINUSTask: Button Minus pressed (14)!\n");
            ESP_LOGI("NO TAG","Button minus Screen Id: %d",screen_id);
            if(isDisp_Nav)
           {
              if(screen_id == 3)
              {
                freq_list_index--;
                if(freq_list_index<0 )
                  freq_list_index=11;
                display_navigation(3);
              }

              else if(screen_id == 4)
              {
                timer_list_index--;
                if(timer_list_index<0)
                    timer_list_index=10;
                display_navigation(4);
              }
           }

            if(isDisp_str)
            {
                if(disp_state == 5)
                {
                    if(play_flag)
                    {
                        str_list_index--;
                        if(str_list_index<0)
                            str_list_index=8;
                        str_change();
                    }

                }
            }
            xQueueReset(BTN_MINUSQueue);
        }
    }

}

void BTN_OKTask(void *params)
{
    gpio_set_direction(BTN_OK, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN_OK, GPIO_INTR_NEGEDGE);

    int BTN_NUMBER = 0;
    int local_disp_id = 0;
    int local_disp_id_special = 0;
    while(1)
    {
        if(xQueueReceive(BTN_OKQueue, &BTN_NUMBER, portMAX_DELAY))
        {
            ESP_LOGI("NO TAG","Task BTN_OKTask: Button Ok pressed (16)!\n");
            ESP_LOGI("NO TAG","Task BTN_OKTask: disp state = %d\n",disp_state);
            xQueueReset(BTN_OKQueue);
            long_press_detected = false;

            while(gpio_get_level(BTN_OK) == 0 && !long_press_detected)
            {
                curr_time = esp_timer_get_time();
                if (curr_time - intr_time >=1000000) // Check for long press duration
                {
                    ESP_LOGI("NO TAG","Long Press Detected");
                    long_press_detected = true; // Set long press flag
                    if(isDisp_menu){
                        // display_strength();
                        local_disp_id = 5;
                        xQueueSend(DISPQueue, &local_disp_id, portMAX_DELAY);

                    }
                    if(isDisp_str)
                    {

                        local_disp_id = 1;
                        // display_menu();
                        //ssd1306_clear_screen(&dev, false);
                        display_menu();
                        xQueueSend(DISPQueue, &local_disp_id, portMAX_DELAY);
 
                    }
                }

                if(gpio_get_level(BTN_OK) == 1)
                {
                    //ESP_LOGI("NO TAG","level 1 Detected");
                    if(curr_time - intr_time <1000000)
                    {
                        ESP_LOGI("NO TAG","Short Press Detected");
                        long_press_detected = true;
                        if(disp_state == 1)
                        {
                            if(!isDisp_Nav)
                            {
                                local_disp_id = 3;
                                xQueueSend(DISPQueue, &local_disp_id, portMAX_DELAY);
                            }
                            else
                            {
                                local_disp_id = disp_state;
                                display_menu();
                                xQueueSend(DISPQueue, &local_disp_id, portMAX_DELAY);
                            }

                        }
                        if(disp_state == 2)
                        {
                            if(!isDisp_Nav){
                                local_disp_id = 4;
                                ssd1306_clear_screen(&dev, false);
                                ESP_LOGI("NO TAG","screen 4 cleared");
                                xQueueSend(DISPQueue, &local_disp_id, portMAX_DELAY);
                            }
                            else
                            {
                                 local_disp_id = disp_state;
                                 display_menu();
                                 xQueueSend(DISPQueue, &local_disp_id, portMAX_DELAY);
                            }
                        }


                        if(disp_state == 5)
                        {
                            local_disp_id = 6;
                            xQueueSend(DISPQueue, &local_disp_id, portMAX_DELAY);
                        }

                        long_press_detected = true;


                    }
                }


            }
        }
        
    }
}

void BTN_PWRTask(void *params)
{
    gpio_set_direction(BTN_PWR, GPIO_MODE_INPUT);
    gpio_set_intr_type(BTN_PWR, GPIO_INTR_NEGEDGE);
    int BTN_NUMBER = 0;

    while(1)
    {
        if(xQueueReceive(BTN_PWRQueue, &BTN_NUMBER, portMAX_DELAY))
        {
           ESP_LOGI("NO TAG","Task BTN_PWRTask: Button power pressed (15)!\n");
            xQueueReset(BTN_PWRQueue);
        }
    }


}


void DISPLAYTask(void *params)
{
    
    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    int screen_id=0;
    display_logo();
    display_menu();

    while(1)
    {
        if(xQueueReceive(DISPQueue, &screen_id, portMAX_DELAY))
        {
            //xQueueReset(DISPQueue);
           ESP_LOGI("NO TAG","Task DISPLAYTask: screen id = %d\n", screen_id); 
            if(screen_id == 1 || screen_id == 2)
                highlight_menu(screen_id);

            if(screen_id == 3 || screen_id == 4)
                display_navigation(screen_id);

            if(screen_id == 5)
            {
                display_strength();
            }
            if(screen_id == 6)
            {
                togglePlayPause();
            }

        }
    }


}

static void IRAM_ATTR BTN_UP_interrupt_handler(void *args)
{
    //ets_printf("ISR: Button 1 pressed!\n");
    
    int pinNumber = (int)args;
    if(esp_timer_get_time() - pre_time_up > 500000)
    {
        xQueueSendFromISR(BTN_UPQueue, &pinNumber, NULL);
    }
    pre_time_up= esp_timer_get_time();
}


static void IRAM_ATTR BTN_DOWN_interrupt_handler(void *args)
{
    //ets_printf("ISR: Button 1 pressed!\n");
    
    int pinNumber = (int)args;
    if(esp_timer_get_time() - pre_time_down > 500000)
    {
        xQueueSendFromISR(BTN_DOWNQueue, &pinNumber, NULL);
    }
    pre_time_down = esp_timer_get_time();
}
static void IRAM_ATTR BTN_PLUS_interrupt_handler(void *args)
{
    //ets_printf("ISR: Button 1 pressed!\n");
    
    int pinNumber = (int)args;
    if(esp_timer_get_time() - pre_time_plus > 500000){
    xQueueSendFromISR(BTN_PLUSQueue, &pinNumber, NULL);
    }
    pre_time_plus= esp_timer_get_time();
}
static void IRAM_ATTR BTN_MINUS_interrupt_handler(void *args)
{
    //ets_printf("ISR: Button 1 pressed!\n");
    
    int pinNumber = (int)args;
    if(esp_timer_get_time() - pre_time_minus>500000){
    xQueueSendFromISR(BTN_MINUSQueue, &pinNumber, NULL);
    }
    pre_time_minus = esp_timer_get_time();
}
static void IRAM_ATTR BTN_PWR_interrupt_handler(void *args)
{
    //ets_printf("ISR: Button 1 pressed!\n");
    
    int pinNumber = (int)args;
    if(esp_timer_get_time() - pre_time_pwr > 500000){
    xQueueSendFromISR(BTN_PWRQueue, &pinNumber, NULL);
    }
    pre_time_pwr = esp_timer_get_time();
}
static void IRAM_ATTR BTN_OK_interrupt_handler(void *args)
{
    //ets_printf("ISR: Button 1 pressed!\n");
    
    int pinNumber = (int)args;
    if(esp_timer_get_time() - pre_time_ok > 500000){
    xQueueSendFromISR(BTN_OKQueue, &pinNumber, NULL);
    intr_time = esp_timer_get_time();
    }
    pre_time_ok = esp_timer_get_time();
}


void app_main(void)
{

    //last updated 23/04/24, 11:07 AM
    // Queues setup
    DISPQueue = xQueueCreate(10, sizeof(int));
    BTN_UPQueue = xQueueCreate(10, sizeof(int));
    BTN_DOWNQueue = xQueueCreate(10, sizeof(int));
    BTN_PLUSQueue = xQueueCreate(10, sizeof(int));
    BTN_MINUSQueue = xQueueCreate(10, sizeof(int));
    BTN_PWRQueue = xQueueCreate(10, sizeof(int));
    BTN_OKQueue = xQueueCreate(10, sizeof(int));
    
    // Button interrupt setup
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_UP, BTN_UP_interrupt_handler, (void *)BTN_UP);
    gpio_isr_handler_add(BTN_DOWN, BTN_DOWN_interrupt_handler, (void *)BTN_DOWN);
    gpio_isr_handler_add(BTN_PLUS, BTN_PLUS_interrupt_handler, (void *)BTN_PLUS);
    gpio_isr_handler_add(BTN_MINUS, BTN_MINUS_interrupt_handler, (void *)BTN_MINUS);
    gpio_isr_handler_add(BTN_PWR, BTN_PWR_interrupt_handler, (void *)BTN_PWR);
    gpio_isr_handler_add(BTN_OK, BTN_OK_interrupt_handler, (void *)BTN_OK);

    //pushed update - 22/04/2023
    xTaskCreate(BTN_UPTask, "BTN_Task", 2048, NULL, 1, NULL);
    xTaskCreate(BTN_DOWNTask, "BTN_Task", 2048, NULL, 1, NULL);
    xTaskCreate(BTN_PLUSTask, "BTN_Task", 2048, NULL, 1, NULL);
    xTaskCreate(BTN_MINUSTask, "BTN_Task", 2048, NULL, 1, NULL);
    xTaskCreate(BTN_PWRTask, "BTN_Task", 2048, NULL, 1, NULL);
    xTaskCreate(BTN_OKTask, "BTN_Task", 2048, NULL, 1, NULL);
    xTaskCreate(DISPLAYTask, "DisplayTASK", 3072, NULL, 1, NULL);

}