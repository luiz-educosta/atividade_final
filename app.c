#include <stdio.h>
#include <pthread.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h" 
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Local includes. */
#include "console.h"

#include <termios.h>

static void prvTask_getChar(void *pvParameters);
static void prvTask_led(void *pvParameters);
static void prvTask_decode(void *pvParameters);
static void prvTask_process_msg_txt(void *pvParameters);

#define TASK1_PRIORITY 1
#define TASK2_PRIORITY 1
#define TASK3_PRIORITY 1
#define TASK4_PRIORITY 1

#define BLACK "\033[30m" /* Black */
#define RED "\033[31m"   /* Red */
#define YELLOW "\033[33m" /* Yellow */
#define DISABLE_CURSOR() printf("\e[?25l")
#define ENABLE_CURSOR() printf("\e[?25h")

#define clear() printf("\033[H\033[J")
#define gotoxy(x, y) printf("\033[%d;%dH", (y), (x))

static uint32_t pos_x_init = 2;
static uint32_t pos_y_init = 4;

typedef struct
{
    int pos;
    char *color;
    int period_ms;
} st_led_param_t;

st_led_param_t yellow = {
    6,
    YELLOW,
    500};

QueueHandle_t structQueue_morse = NULL;
QueueHandle_t structQueue_key = NULL;
QueueHandle_t structQueue_txt = NULL;

TaskHandle_t msgTask_hdlr, decodeTask_hdlr, keyTask_hdlr;

static int input_val(uint8_t data)
{
    if((data >= '0' &&  data <= '9') || (data >= 'A' && data <= 'Z') || (data >= 'a' && data <= 'z') || data == 32)
    {
        return 1;
    }

    return 0;
}

static char* morse_decoder(char key)
{

    switch (key)
    {
        case 'a':
        case 'A':
            return (uint8_t*)".- ";
        break;

        case 'b':
        case 'B':
            return (uint8_t*)"-... ";
        break;

        case 'c':
        case 'C':
            return (uint8_t*)"-.-. ";
        break;

        case 'd':
        case 'D':
            return (uint8_t*)"-..";
        break;

        case 'e':
        case 'E':
            return (uint8_t*)". ";
        break;

        case 'f':
        case 'F':
            return (uint8_t*)"..-. ";
        break;

        case 'g':
        case 'G':
            return (uint8_t*)"... ";
        break;

        case 'h':
        case 'H':
            return (uint8_t*)".... ";
        break;

        case 'i':
        case 'I':
            return (uint8_t*)".. ";
        break;

        case 'j':
        case 'J':
            return (uint8_t*)".--- ";
        break;

        case 'k':
        case 'K':
            return (uint8_t*)"-.- ";
        break;

        case 'l':
        case 'L':
            return (uint8_t*)".-.. ";
        break;

        case 'm':
        case 'M':
            return (uint8_t*)"-- ";
        break;

        case 'n':
        case 'N':
            return (uint8_t*)"-. ";
        break;

        case 'o':
        case 'O':
            return (uint8_t*)"--- ";
        break;

        case 'p':
        case 'P':
            return (uint8_t*)".--. ";
        break;

        case 'q':
        case 'Q':
            return (uint8_t*)"--.. ";
        break;

        case 'r':
        case 'R':
            return (uint8_t*)".-. ";
        break;

        case 's':
        case 'S':
            return (uint8_t*)"... ";
        break;

        case 't':
        case 'T':
            return (uint8_t*)"- ";
        break;

        case 'u':
        case 'U':
            return (uint8_t*)"..- ";
        break;

        case 'v':
        case 'V':
            return (uint8_t*)"...- ";
        break;

        case 'w':
        case 'W':
            return (uint8_t*)".-- ";
        break;

        case 'x':
        case 'X':
            return (uint8_t*)"-..- ";
        break;

        case 'y':
        case 'Y':
            return (uint8_t*)"-.-- ";
        break;

        case 'z':
        case 'Z':
            return (uint8_t*)"--.. ";
        break;

        case '0':
            return (uint8_t*)"----- ";
        break;

        case '1':
            return (uint8_t*)".---- ";
        break;

        case '2':
            return (uint8_t*)"..--- ";
        break;

        case '3':
            return (uint8_t*)"...-- ";
        break;

        case '4':
            return (uint8_t*)"....- ";
        break;

        case '5':
            return (uint8_t*)"..... ";
        break;

        case '6':
            return (uint8_t*)"-.... ";
        break;

        case '7':
            return (uint8_t*)"--... ";
        break;

        case '8':
            return (uint8_t*)"---.. ";
        break;

        case '9':
            return (uint8_t*)"----. ";
        break;

        case '32':
            return (uint8_t*)"  ";
        break;

    }
}

static void prvTask_getChar(void *pvParameters)
{
    char key;
    uint32_t notificationValue;      
    unsigned long notify = 0;  
                          

    /* I need to change  the keyboard behavior to
    enable nonblock getchar */
    struct termios initial_settings,
        new_settings;

    tcgetattr(0, &initial_settings);

    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 1;
    tcsetattr(0, TCSANOW, &new_settings);
    /* End of keyboard configuration */

    for (;;)
    {
        if (xTaskNotifyWait(
        ULONG_MAX,
        ULONG_MAX,
        &notificationValue,
        notify))
        {
            if(notificationValue == 42)
            {
                break;
            }
            else if(notificationValue == 10)
            {
                notify = portMAX_DELAY;
            }
            else{
                notify = 0;
            }
        }

        key = getchar();
        
        if(key > 0 )

        {
            if (xQueueSend(structQueue_key, &key, 0) != pdTRUE)
            {
                vTaskDelay(500 / portTICK_PERIOD_MS);
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    tcsetattr(0, TCSANOW, &initial_settings);
    ENABLE_CURSOR();
    exit(0);
    vTaskDelete(NULL);
}

static void prvTask_process_msg_txt(void *pvParameters)
{
    uint32_t txt_size = 0;
    char txt;

    for(;;)
    {
        if(xQueueReceive(structQueue_key,&txt,portMAX_DELAY) == pdPASS)
        {
           
            if(txt_size > 0 && txt == 10)
            {   
                DISABLE_CURSOR();

                xTaskCreate(prvTask_led, "Morse_LED", configMINIMAL_STACK_SIZE, &yellow, TASK4_PRIORITY, NULL);

                xTaskNotify(keyTask_hdlr, 10, eSetValueWithOverwrite);

                vTaskResume(decodeTask_hdlr);

                xTaskNotify(decodeTask_hdlr, txt_size, eSetValueWithOverwrite);

                vTaskSuspend(msgTask_hdlr);

                txt_size = 0;
                txt = 0;
            }
            else if (txt == 42)
            {
                xTaskNotify(keyTask_hdlr, 42, eSetValueWithOverwrite);
            }
            else
            {
                if(1 == input_val(txt))
                {
                    if(xQueueSend(structQueue_txt, &txt, 0) == pdTRUE)
                    {
                        txt_size++;
                    }
                }
            }
        }

        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void prvTask_decode(void *pvParameters)
{
    uint32_t decode_size = 0; 
    uint32_t notificationValue; 
    char symbol; 
    unsigned long timer = portMAX_DELAY;   
    

    for(;;)
    {
        if (xTaskNotifyWait(
                ULONG_MAX,
                ULONG_MAX,
                &notificationValue,
                timer))
        {
            decode_size = notificationValue; 
            timer = 0;
        }

        if(decode_size > 0)
        {
            if(xQueueReceive(structQueue_txt, &symbol, 0) == pdPASS)
            {
                char* value = morse_decoder(symbol);
                
                xQueueSend(structQueue_morse, &value, 0);
                
                decode_size--;
            }
        }
        else
        {
            vTaskResume(msgTask_hdlr);

            timer = portMAX_DELAY;
            vTaskSuspend(decodeTask_hdlr);
        }
    }
    vTaskDelete(NULL);
}

static void prvTask_led(void *pvParameters)
{
    char* data;
    char x;
    uint8_t size_data = 0;

    st_led_param_t *led = (st_led_param_t *)pvParameters;
    portTickType xLastWakeTime = xTaskGetTickCount();
    for (;;)
    {
        
        if(xQueueReceive(structQueue_morse, &data, 200) == pdPASS)
        {
            size_data = strlen(data);
            for(int i = 0; i < size_data; i++)
            {
                x = data[i];

                if(x == '.')
                {
                    gotoxy(pos_x_init++, pos_y_init);
                    printf("%c", x);

                    gotoxy(led->pos, 2);
                    printf("%s⬤", led->color);
                    fflush(stdout);
                    vTaskDelay(500 / portTICK_PERIOD_MS);

                    gotoxy(led->pos, 2);
                    printf("%s ", BLACK);
                    fflush(stdout);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }
                else if(x == '-'){

                    gotoxy(pos_x_init++, pos_y_init);
                    printf("%c", x);

                    gotoxy(led->pos, 2);
                    printf("%s⬤", led->color);
                    fflush(stdout);
                    vTaskDelay(1500 / portTICK_PERIOD_MS);

                    gotoxy(led->pos, 2);
                    printf("%s ", BLACK);
                    fflush(stdout);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }
                else{
                    gotoxy(led->pos, 2);
                    printf("%s ", BLACK);
                    fflush(stdout);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }

            }

            vTaskDelay(1000 / portTICK_PERIOD_MS);
            gotoxy(pos_x_init++, pos_y_init);
            printf("  ");
        }
        else
        {
            xTaskNotify(keyTask_hdlr, 0UL, eSetValueWithOverwrite);
            
            pos_x_init = 2;
            pos_y_init++;

            vTaskDelete(NULL);
        }
    }

    vTaskDelete(NULL);
}

void app_run(void)
{
    structQueue_key = xQueueCreate(50,1);
    structQueue_txt = xQueueCreate(50,1);
    structQueue_morse = xQueueCreate(50,8);

    if(structQueue_key == NULL || structQueue_txt == NULL || structQueue_morse == NULL)
    {
        exit(1);
    }

    clear();
    DISABLE_CURSOR();
    printf(
        "╔═════════════════╗\n"
        "║                 ║\n"
        "╚═════════════════╝\n");

    xTaskCreate(prvTask_getChar, "Get_key", configMINIMAL_STACK_SIZE, NULL, TASK1_PRIORITY, &keyTask_hdlr);

    xTaskCreate(prvTask_process_msg_txt, "Msg_process_txt", configMINIMAL_STACK_SIZE, NULL, TASK2_PRIORITY, &msgTask_hdlr);

    xTaskCreate(prvTask_decode, "Decode", configMINIMAL_STACK_SIZE, NULL, TASK3_PRIORITY, &decodeTask_hdlr);

    vTaskSuspend(decodeTask_hdlr);

    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    for (;;)
    {
    }
}