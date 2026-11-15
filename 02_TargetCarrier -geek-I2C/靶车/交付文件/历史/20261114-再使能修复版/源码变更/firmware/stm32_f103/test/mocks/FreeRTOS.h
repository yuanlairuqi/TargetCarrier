#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H
#include <stdint.h>
#include <stddef.h>
typedef uint32_t TickType_t;
typedef int StaticSemaphore_t;
typedef void *SemaphoreHandle_t;
typedef int BaseType_t;
typedef unsigned UBaseType_t;
typedef int StaticTask_t;
typedef uint32_t StackType_t;
typedef void *TaskHandle_t;
#define pdTRUE 1
#define pdFALSE 0
#define pdMS_TO_TICKS(x) (x)
#define portYIELD_FROM_ISR(woken) ((void)(woken))
#define portMAX_DELAY UINT32_MAX
#endif
