cmake_minimum_required(VERSION 3.18)  # OpENer requires 3.18+

# Назва проекту
project(ServoLib_Emulator C)

# Стандарт C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

#######################################
# OpENer Configuration                #
#######################################

# OpENer options
option(USE_HWD_EIP "Enable EtherNet/IP support via OpENer" ON)

if(USE_HWD_EIP)
    message(STATUS "EtherNet/IP support: ENABLED")

    # OpENer platform selection
    set(OpENer_PLATFORM "MINGW" CACHE STRING "OpENer platform")

    # OpENer device configuration
    set(OpENer_Device_Config_Vendor_Id 65535 CACHE STRING "Vendor ID (65535 = experimental)")
    set(OpENer_Device_Config_Device_Type 2 CACHE STRING "Device Type (2 = AC Drive)")
    set(OpENer_Device_Config_Product_Code 1 CACHE STRING "Product Code")
    set(OpENer_Device_Config_Device_Name "ServoLib EtherNet/IP" CACHE STRING "Device Name")
    set(OpENer_Device_Major_Version 1 CACHE STRING "Major version")
    set(OpENer_Device_Minor_Version 0 CACHE STRING "Minor version")

    # OpENer build options
    set(OPENER_INSTALL_AS_LIB ON CACHE BOOL "Build OpENer as library")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static library")
    set(OpENer_TRACES ON CACHE BOOL "Enable OpENer traces")
    set(OPENER_TRACE_LEVEL "INFO" CACHE STRING "Trace level")
    set(OPENER_ETHERNET_BUFFER_SIZE 512 CACHE STRING "Ethernet buffer size")

    # Add OpENer as subdirectory
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../External/OpENer/source
                     ${CMAKE_BINARY_DIR}/OpENer_build)

    # Add OpENer include directories
    set(OPENER_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/../External/OpENer/source/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../External/OpENer/source/src/cip
        ${CMAKE_CURRENT_SOURCE_DIR}/../External/OpENer/source/src/enet_encap
        ${CMAKE_CURRENT_SOURCE_DIR}/../External/OpENer/source/src/ports
        ${CMAKE_CURRENT_SOURCE_DIR}/../External/OpENer/source/src/utils
        ${CMAKE_BINARY_DIR}/OpENer_build/src/ports
    )
else()
    message(STATUS "EtherNet/IP support: DISABLED (using legacy UDP)")
endif()

# Опції компіляції для MinGW
if(MINGW)
    # Додаткові флаги для MinGW
    add_compile_options(-Wall -Wextra -Wpedantic)

    # Бібліотека winsock2 для UDP на Windows
    set(PLATFORM_LIBS ws2_32)

    # Визначення для Windows
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
endif()

# Додаткові опції компіляції
add_compile_options(
    -O2                    # Оптимізація
    -g                     # Символи для налагодження
    -DUSE_REAL_HARDWARE=0  # Емуляція, не реальне залізо
    -DPC_EMULATION=1       # PC емуляція
)

# Conditional compilation для EtherNet/IP або legacy UDP
if(USE_HWD_EIP)
    add_compile_options(-DUSE_HWD_EIP=1)
    message(STATUS "Using EtherNet/IP communication (OpENer)")
else()
    add_compile_options(
        -DUSE_BRAKE_UDP=1      # Використовувати UDP гальма для емуляції
        -DUSE_MOTOR_PWM_UDP=1  # Використовувати UDP PWM двигун для емуляції
    )
    message(STATUS "Using legacy UDP communication")
endif()

# Визначення шляхів до заголовочних файлів
set(SERVOLIB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

include_directories(
    ${SERVOLIB_ROOT}/Inc
    ${SERVOLIB_ROOT}/Inc/ctrl
    ${SERVOLIB_ROOT}/Inc/drv
    ${SERVOLIB_ROOT}/Inc/hwd
    ${SERVOLIB_ROOT}/Inc/iface
    ${SERVOLIB_ROOT}/Inc/util
    ${SERVOLIB_ROOT}/Board/PC_Emulation
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add OpENer includes if enabled
if(USE_HWD_EIP)
    include_directories(${OPENER_INCLUDES})
endif()

# Збір всіх .c файлів з бібліотеки
file(GLOB_RECURSE SERVOLIB_SOURCES
    ${SERVOLIB_ROOT}/Src/*.c
    ${SERVOLIB_ROOT}/Src/ctrl/*.c
    ${SERVOLIB_ROOT}/Src/drv/*.c
    ${SERVOLIB_ROOT}/Src/iface/*.c
    ${SERVOLIB_ROOT}/Src/util/*.c
    ${SERVOLIB_ROOT}/Src/hwd/*.c
    ${SERVOLIB_ROOT}/Src/drv/brake/brake.c  # Додати brake.c для можливості виключення
)

# Виключення файлів, що мають замінники в Board/PC_Emulation
list(FILTER SERVOLIB_SOURCES EXCLUDE REGEX ".*hwd_gpio\\.c$")
list(FILTER SERVOLIB_SOURCES EXCLUDE REGEX ".*hwd_pwm\\.c$")
list(FILTER SERVOLIB_SOURCES EXCLUDE REGEX ".*hwd_timer\\.c$")
list(FILTER SERVOLIB_SOURCES EXCLUDE REGEX ".*hwd_udp\\.c$")
# Виключення brake.c оскільки використовується brake_udp.c для емуляції
list(FILTER SERVOLIB_SOURCES EXCLUDE REGEX ".*drv/brake/brake\\.c$")

# Збір всіх .c файлів з платформи PC_Emulation
file(GLOB BOARD_SOURCES
    ${SERVOLIB_ROOT}/Board/PC_Emulation/*.c
)

# Збір всіх .c файлів емулятора
set(EMULATOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/udp_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/brake_mock.c
)

# Виключення файлів, специфічних для STM32 (якщо такі є)
list(FILTER SERVOLIB_SOURCES EXCLUDE REGEX ".*stm32.*")

# Виведення інформації про знайдені файли
message(STATUS "ServoLib sources: ${SERVOLIB_SOURCES}")
message(STATUS "Board sources: ${BOARD_SOURCES}")
message(STATUS "Emulator sources: ${EMULATOR_SOURCES}")

# Створення виконуваного файлу
add_executable(${PROJECT_NAME}
    ${SERVOLIB_SOURCES}
    ${BOARD_SOURCES}
    ${EMULATOR_SOURCES}
)

# Підключення бібліотек
if(USE_HWD_EIP)
    # Link OpENer library
    target_link_libraries(${PROJECT_NAME} OpENer)
    message(STATUS "Linking OpENer library")
endif()

# Підключення бібліотек для Windows (winsock2)
if(MINGW)
    target_link_libraries(${PROJECT_NAME} ${PLATFORM_LIBS})
endif()

# Підключення pthread для Linux/Unix
if(UNIX)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

# Додаткові налаштування для MinGW
if(MINGW)
    # Статичне лінкування runtime бібліотек MinGW
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")

    # Створення консольного застосунку
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif()

# Опції для налагодження
option(ENABLE_DEBUG "Enable debug output" ON)
if(ENABLE_DEBUG)
    add_definitions(-DENABLE_DEBUG=1)
endif()

# Виведення конфігурації збірки
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Platform libs: ${PLATFORM_LIBS}")

# Інсталяція (опціонально)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Додаткова ціль для очищення
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning all build files..."
)

# Ціль для запуску емулятора
add_custom_target(run
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ServoLib Emulator..."
)

# Генерація документації по структурі проекту
add_custom_target(info
    COMMAND ${CMAKE_COMMAND} -E echo "=== ServoLib Emulator Build Info ==="
    COMMAND ${CMAKE_COMMAND} -E echo "Project: ${PROJECT_NAME}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build type: ${CMAKE_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_C_COMPILER}"
    COMMAND ${CMAKE_COMMAND} -E echo "Source files: ${SERVOLIB_SOURCES}"
    COMMENT "Displaying build information"
)
