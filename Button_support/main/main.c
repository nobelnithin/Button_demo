/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <button.h>


#define BTN_GPIO 6
uint8_t button_idx1 = 1;




void button_callback(button_event_t event, void* context) {
    int button_idx = *((uint8_t*) context);

    switch (event) {
        case button_event_single_press:
            printf("button %d single press\n", button_idx);
            break;
        case button_event_double_press:
            printf("button %d double press\n", button_idx);
            break;
        case button_event_tripple_press:
            printf("button %d tripple press\n", button_idx);
            break;
        case button_event_long_press:
            printf("button %d long press\n", button_idx);
            break;
        default:
            printf("unexpected button %d event: %d\n", button_idx, event);
    }
}

void app_main(void)
{
      printf("Button example\n");

    button_config_t button_config = BUTTON_CONFIG(
        button_active_low,
        .long_press_time = 750,
        .max_repeat_presses = 3,
    );

    int r;
    r = button_create(BTN_GPIO, button_config, button_callback, &button_idx1);
    if (r) {
        printf("Failed to initialize button %d (code %d)\n", button_idx1, r);
    }

}
