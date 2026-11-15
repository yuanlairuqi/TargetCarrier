#include "FreeRTOS.h"
#define taskENTER_CRITICAL() ((void)0)
#define taskEXIT_CRITICAL() ((void)0)
enum { eSetBits };
TickType_t xTaskGetTickCount(void);
BaseType_t xTaskNotifyFromISR(TaskHandle_t task, uint32_t value, int action, BaseType_t *woken);
BaseType_t xTaskNotifyWait(uint32_t clear_in, uint32_t clear_out, uint32_t *value, TickType_t timeout);
void vTaskDelay(TickType_t delay);
void vTaskDelayUntil(TickType_t *wake, TickType_t delay);
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t task);
TaskHandle_t xTaskCreateStatic(void (*run)(void *), const char *name, uint32_t size,
                               void *arg, UBaseType_t prio, StackType_t *stack, StaticTask_t *buf);
void vTaskStartScheduler(void);
