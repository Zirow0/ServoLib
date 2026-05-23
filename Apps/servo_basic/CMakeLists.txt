# Базовий сервопривід (каскадний PID, без safety та trajectory)
# з асинхронним UART DMA комунікаційним шаром

stm32_add_executable(servo_basic
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_ALL}
    ${SERVOLIB_COMM}
    ${MCU_ASYNC_SRCS}
)

target_include_directories(servo_basic PRIVATE
    ${CMAKE_SOURCE_DIR}/Src
)

target_link_libraries(servo_basic PRIVATE frame_codec packet_codec ukf_mcu)

target_compile_definitions(servo_basic PRIVATE USE_COMM_ASYNC)
