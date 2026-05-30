# ─── Зовнішні залежності ──────────────────────────────────────────────────────
# Пріоритет: Lib/<name>/ (submodule/symlink) → FetchContent (fallback)

include(${CMAKE_CURRENT_LIST_DIR}/deps/cmake_dep.cmake)

fetch_or_local(frame_codec  git@github.com:Zirow0/frame_codec.git  master)
fetch_or_local(packet_codec git@github.com:Zirow0/packet_codec.git master)
fetch_or_local(ukf_mcu      git@github.com:Zirow0/ukf_mcu.git      master)

# ─── ServoLib source groups ───────────────────────────────────────────────────
# Кожна група — мінімальний набір файлів для конкретної підсистеми.
# Застосовуйте у Apps/*/CMakeLists.txt на власний розсуд.

set(SL ${CMAKE_SOURCE_DIR}/Src)

# Базові утиліти (потрібні завжди)
set(SERVOLIB_UTIL
    ${SL}/core.c
    ${SL}/hwd/hwd.c
    ${SL}/util/derivative.c
    ${SL}/ctrl/time.c
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
    ${SL}/drv/position/encoder_ukf.c
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
)

# Async комунікація (frame_codec + packet_codec + servo_comm)
# Використовувати разом з MCU_ASYNC_SRCS (hwd_uart_async.c + hwd_crc32.c)
# frame_codec та packet_codec підключаються через target_link_libraries
set(SERVOLIB_COMM
    ${SL}/comm/servo_comm.c
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
