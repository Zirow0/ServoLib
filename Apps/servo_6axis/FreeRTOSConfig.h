#pragma once

/* ── Основні параметри ─────────────────────────────────────────────────────── */
#define configCPU_CLOCK_HZ                          100000000UL
#define configTICK_RATE_HZ                          1000U        /* 1 ms tick */
#define configMAX_PRIORITIES                        8
#define configMINIMAL_STACK_SIZE                    128          /* слів */
#define configTOTAL_HEAP_SIZE                       (24 * 1024)

/* Нове ядро: явна ширина типу tick (замість configUSE_16_BIT_TICKS) */
#define configTICK_TYPE_WIDTH_IN_BITS               TICK_TYPE_WIDTH_32_BITS

/* Cortex-M4: апаратний CLZ для вибору задачі */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION     1

/* ── Планувальник ──────────────────────────────────────────────────────────── */
#define configUSE_PREEMPTION                        1
#define configUSE_IDLE_HOOK                         0
#define configUSE_TICK_HOOK                         0
#define configUSE_CO_ROUTINES                       0

/* ── Пріоритети переривань (STM32F411: 4 біти = 0..15) ────────────────────── */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* ISR з NVIC-пріоритетом 0..4 — поза контролем FreeRTOS (не викликати API) */
#define configKERNEL_INTERRUPT_PRIORITY     (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << 4)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << 4)

/* ── Використовувані примітиви ─────────────────────────────────────────────── */
#define configUSE_MUTEXES                           1
#define configUSE_COUNTING_SEMAPHORES               0
#define configUSE_TASK_NOTIFICATIONS                1
#define configUSE_TIMERS                            0

/* ── Безпека ───────────────────────────────────────────────────────────────── */
#define configCHECK_FOR_STACK_OVERFLOW              2
#define configUSE_MALLOC_FAILED_HOOK                1

/* ── INCLUDE ───────────────────────────────────────────────────────────────── */
#define INCLUDE_vTaskDelay                          1
#define INCLUDE_vTaskDelayUntil                     1
#define INCLUDE_xTaskGetTickCount                   1
#define INCLUDE_uxTaskGetStackHighWaterMark         1

/* ── Відображення імен переривань libopencm3 → FreeRTOS port ──────────────── */
#define vPortSVCHandler     sv_call_handler
#define xPortPendSVHandler  pend_sv_handler
#define xPortSysTickHandler sys_tick_handler
