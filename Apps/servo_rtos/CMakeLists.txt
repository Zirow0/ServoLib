# servo_rtos — одна вісь з FreeRTOS
# Передумова: git submodule add https://github.com/FreeRTOS/FreeRTOS-Kernel.git Lib/freertos

include(${CMAKE_SOURCE_DIR}/cmake/freertos.cmake)

stm32_add_executable(servo_rtos
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_ALL}
    ${SERVOLIB_COMM}
    ${MCU_ASYNC_SRCS}
    ${FREERTOS_SRCS}
)

target_include_directories(servo_rtos PRIVATE
    ${CMAKE_SOURCE_DIR}/Src
    ${CMAKE_CURRENT_SOURCE_DIR}   # FreeRTOSConfig.h
    ${FREERTOS_INCLUDES}
)

target_link_libraries(servo_rtos PRIVATE frame_codec packet_codec ukf_mcu)

target_compile_definitions(servo_rtos PRIVATE
    USE_COMM_ASYNC
    USE_FREERTOS
)
