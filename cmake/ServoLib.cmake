# ─── ServoLib source groups ───────────────────────────────────────────────────
# Кожна група — мінімальний набір файлів для конкретної підсистеми.
# Застосовуйте у Apps/*/CMakeLists.txt на власний розсуд.

set(SL ${CMAKE_SOURCE_DIR}/Src)

# Базові утиліти (потрібні завжди)
set(SERVOLIB_UTIL
    ${SL}/core.c
    ${SL}/hwd/hwd.c
    ${SL}/util/derivative.c
)

# Підсистема двигуна
set(SERVOLIB_MOTOR
    ${SL}/drv/motor/motor.c
    ${SL}/drv/motor/pwm.c
)

# Підсистема датчика положення
set(SERVOLIB_POSITION
    ${SL}/drv/position/position.c
    ${SL}/drv/position/incremental_encoder.c
    ${SL}/drv/position/as5600.c
)

# Підсистема датчика струму
set(SERVOLIB_CURRENT
    ${SL}/drv/current/current.c
    ${SL}/drv/current/acs712.c
)

# Підсистема гальма
set(SERVOLIB_BRAKE
    ${SL}/drv/brake/brake.c
    ${SL}/drv/brake/gpio_brake.c
)

# Шар керування (залежить від усіх підсистем)
set(SERVOLIB_CTRL
    ${SL}/ctrl/servo.c
    ${SL}/ctrl/pid.c
    ${SL}/ctrl/cascade.c
    ${SL}/ctrl/safety.c
    ${SL}/ctrl/traj.c
    ${SL}/ctrl/time.c
)

# Async комунікація (frame_codec + packet_codec + servo_comm)
# Використовувати разом з board-specific hwd_uart_async.c та hwd_crc32.c
set(LIB ${CMAKE_SOURCE_DIR}/Lib)
set(SERVOLIB_COMM
    ${SL}/comm/servo_comm.c
    ${LIB}/frame_codec/src/frame_codec.c
    ${LIB}/frame_codec/src/cobs.c
    ${LIB}/frame_codec/src/crc32_soft.c
    ${LIB}/packet_codec/src/packet_codec.c
)
set(SERVOLIB_COMM_INCLUDES
    ${LIB}/frame_codec/include
    ${LIB}/packet_codec/include
)

# Повна бібліотека
set(SERVOLIB_ALL
    ${SERVOLIB_UTIL}
    ${SERVOLIB_MOTOR}
    ${SERVOLIB_POSITION}
    ${SERVOLIB_CURRENT}
    ${SERVOLIB_BRAKE}
    ${SERVOLIB_CTRL}
)
