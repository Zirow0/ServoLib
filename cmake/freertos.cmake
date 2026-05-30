# FreeRTOS kernel для Cortex-M4F (STM32F411)
# Передумова: git submodule add https://github.com/FreeRTOS/FreeRTOS-Kernel.git Lib/freertos

set(FREERTOS_DIR ${CMAKE_SOURCE_DIR}/Lib/freertos)

set(FREERTOS_SRCS
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/timers.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
)

set(FREERTOS_INCLUDES
    ${FREERTOS_DIR}/include
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F
    # FreeRTOSConfig.h — в директорії застосунку (додати окремо)
)
