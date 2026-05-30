# servo_6axis — 6-осьовий серво-контролер з FreeRTOS
# Плата: STM32F411_SERVO_MASTER (BlackPill, TIM3+TIM1, SPI2→EncoderHub)

include(${CMAKE_SOURCE_DIR}/cmake/freertos.cmake)

stm32_add_executable(servo_6axis
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_ALL}
    ${SERVOLIB_COMM}
    ${MCU_ASYNC_SRCS}
    ${FREERTOS_SRCS}
)

target_include_directories(servo_6axis PRIVATE
    ${CMAKE_SOURCE_DIR}/Src
    ${CMAKE_CURRENT_SOURCE_DIR}   # FreeRTOSConfig.h
    ${FREERTOS_INCLUDES}
)

target_link_libraries(servo_6axis PRIVATE frame_codec packet_codec ukf_mcu)

target_compile_definitions(servo_6axis PRIVATE
    USE_COMM_ASYNC
    USE_FREERTOS
)
