/**
 * @file hwd_udp.c
 * @brief Реалізація HWD UDP для емуляції на ПК
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить реалізацію HWD для UDP комунікації при емуляції на ПК.
 * (Цей файл вже був створений раніше, але тепер ми оновимо його для емуляційної платформи)
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/hwd/hwd_udp.h"
#include "board_config.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    typedef int socket_t;
    #define SOCKET_ERROR -1
    #define INVALID_SOCKET -1
#endif

/* Private defines -----------------------------------------------------------*/
#define MAX_UDP_BUFFER_SIZE 512

/* Private variables ---------------------------------------------------------*/
static socket_t udp_socket = INVALID_SOCKET;
static struct sockaddr_in server_addr;
static struct sockaddr_in client_addr;
static bool udp_initialized = false;

/* Private function prototypes -----------------------------------------------*/
static Servo_Status_t InitWinSock(void);
static Servo_Status_t CloseWinSock(void);

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_UDP_Init(const char* server_ip, uint16_t server_port, uint16_t client_port)
{
    // Використовуємо значення за замовчуванням, якщо не вказані
    if (server_ip == NULL) {
        server_ip = UDP_SERVER_IP;
    }
    if (server_port == 0) {
        server_port = UDP_SERVER_PORT;
    }
    if (client_port == 0) {
        client_port = UDP_CLIENT_PORT;
    }
    
    // Ініціалізація Windows Sockets (на Windows)
    Servo_Status_t status = InitWinSock();
    if (status != SERVO_OK) {
        return status;
    }
    
    // Створення UDP сокета
    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket == INVALID_SOCKET) {
        printf("ERROR: Failed to create UDP socket\n");
        CloseWinSock();
        return SERVO_ERROR;
    }
    
    // Налаштування адреси сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    // Перетворення IP адреси
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("ERROR: Invalid server IP address: %s\n", server_ip);
        #ifdef _WIN32
        closesocket(udp_socket);
        #else
        close(udp_socket);
        #endif
        CloseWinSock();
        return SERVO_ERROR;
    }
    
    // Налаштування адреси клієнта (локально)
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(client_port);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Прив'язка сокета до адреси клієнта
    if (bind(udp_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)) == SOCKET_ERROR) {
        printf("ERROR: Failed to bind UDP socket\n");
        #ifdef _WIN32
        closesocket(udp_socket);
        #else
        close(udp_socket);
        #endif
        CloseWinSock();
        return SERVO_ERROR;
    }
    
    // Налаштування таймауту для отримання
    struct timeval timeout;
    timeout.tv_sec = UDP_TIMEOUT_MS / 1000;
    timeout.tv_usec = (UDP_TIMEOUT_MS % 1000) * 1000;
    
    if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, 
                   (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        printf("ERROR: Failed to set socket timeout\n");
    }
    
    udp_initialized = true;
    printf("UDP connection established: %s:%d -> Local:%d\n", 
           server_ip, server_port, client_port);
    
    return SERVO_OK;
}

Servo_Status_t HWD_UDP_Send(const UDP_Message_t* msg)
{
    if (!udp_initialized || !msg) {
        return SERVO_NOT_INIT;
    }
    
    if (!msg->payload && msg->payload_size > 0) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // Відправка повідомлення
    int bytes_sent = sendto(udp_socket, 
                           (const char*)msg, 
                           sizeof(UDP_MsgType_t) + sizeof(uint32_t) + sizeof(uint16_t) + msg->payload_size,
                           0,
                           (struct sockaddr*)&server_addr, 
                           sizeof(server_addr));
    
    if (bytes_sent == SOCKET_ERROR) {
        printf("ERROR: Failed to send UDP message (error: %d)\n", 
               #ifdef _WIN32
               WSAGetLastError()
               #else
               errno
               #endif
               );
        return SERVO_ERROR;
    }
    
    return SERVO_OK;
}

Servo_Status_t HWD_UDP_Receive(UDP_Message_t* msg, uint32_t timeout_ms)
{
    if (!udp_initialized || !msg) {
        return SERVO_NOT_INIT;
    }
    
    // Налаштування таймауту
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, 
                   (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        printf("ERROR: Failed to set receive timeout\n");
        return SERVO_ERROR;
    }
    
    // Отримання повідомлення
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int bytes_received = recvfrom(udp_socket, 
                                 (char*)msg, 
                                 sizeof(UDP_Message_t), 
                                 0,
                                 (struct sockaddr*)&server_addr, 
                                 &addr_len);
    
    if (bytes_received == SOCKET_ERROR) {
        printf("ERROR: Failed to receive UDP message (timeout or error)\n");
        return SERVO_TIMEOUT;
    }
    
    return SERVO_OK;
}

Servo_Status_t HWD_UDP_DeInit(void)
{
    if (udp_initialized) {
        #ifdef _WIN32
        closesocket(udp_socket);
        #else
        close(udp_socket);
        #endif
        udp_socket = INVALID_SOCKET;
        CloseWinSock();
        udp_initialized = false;
        printf("UDP connection closed\n");
    }
    
    return SERVO_OK;
}

Servo_Status_t HWD_UDP_Ping(void)
{
    if (!udp_initialized) {
        return SERVO_NOT_INIT;
    }
    
    UDP_Message_t ping_msg = {0};
    ping_msg.msg_type = UDP_MSG_TYPE_PING;
    ping_msg.sequence_number = 0;
    ping_msg.payload_size = 0;
    
    Servo_Status_t status = HWD_UDP_Send(&ping_msg);
    if (status != SERVO_OK) {
        return status;
    }
    
    // Спробуємо отримати відповідь
    UDP_Message_t response_msg = {0};
    status = HWD_UDP_Receive(&response_msg, UDP_TIMEOUT_MS);
    
    return status;
}

/* Private functions ---------------------------------------------------------*/

static Servo_Status_t InitWinSock(void)
{
    #ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("ERROR: WSAStartup failed with error: %d\n", result);
        return SERVO_ERROR;
    }
    #endif
    
    return SERVO_OK;
}

static Servo_Status_t CloseWinSock(void)
{
    #ifdef _WIN32
    WSACleanup();
    #endif
    
    return SERVO_OK;
}