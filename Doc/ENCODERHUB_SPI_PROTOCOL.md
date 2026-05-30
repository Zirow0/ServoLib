# EncoderHub SPI Protocol

## Огляд

Протокол обміну даними між **OCM3** (master) та **EncoderHub** (slave) по шині SPI.
EncoderHub обробляє 6 інкрементальних енкодерів, запускає UKF-фільтр на кожному каналі
та передає відфільтровані дані (θ, ω, α) до master'а.

**Стек протоколу:**
```
OCM3 (master)              EncoderHub (slave)
─────────────────          ──────────────────
ehub_comm                  ehub_comm
     ↓                           ↓
packet_codec               packet_codec
(msg_id | payload)         (msg_id | payload)
     ↓                           ↓
frame_codec                frame_codec
(COBS + CRC32)             (COBS + CRC32)
     ↓                           ↓
hwd_spi_master             hwd_spi_slave
(SPI2 DMA master)          (SPI2 DMA slave)
```

---

## Фізичний рівень

### З'єднання

| Сигнал | Напрям | Призначення |
|--------|--------|-------------|
| `MOSI` | Master → Slave | Дані від master (команди) |
| `MISO` | Slave → Master | Дані від slave (телеметрія, відповіді) |
| `SCK`  | Master → Slave | Тактовий сигнал |
| `CS`   | Master → Slave | Вибір slave (активний LOW) |
| `GND`  | —              | Спільна земля |

**Без DRDY.** Master читає з фіксованим інтервалом (1 кГц). Поле `seq` в телеметрії
дозволяє виявити стале значення (якщо `seq` не змінився — дані не оновлювались).

### Параметри SPI

| Параметр | Значення |
|----------|----------|
| Режим | Mode 0 (CPOL=0, CPHA=0) |
| Тактова частота | 10 МГц |
| Порядок біт | MSB first |
| Розмір кадру | 8 біт |
| Розмір транзакції | **64 байти** (фіксований) |

---

## Транзакційна модель

Кожен SPI-обмін — 64 байти, **full-duplex**:

```
Master MOSI: [ команда або 0xFF·64 ]
Master MISO: [ відповідь на попередню команду ]
```

Slave завжди має готовий кадр відповіді у double-buffer:
- **Buffer A** — передається по DMA (поточна транзакція)
- **Buffer B** — заповнюється UKF (наступне оновлення)
- Swap відбувається після CS↑

**Затримка відповіді:** відповідь на команду надходить у **наступній** транзакції.
Між командою та читанням відповіді — один цикл (1 мс при 1 кГц).

```
Транзакція N:   Master → READ_ALL     Slave → (попередня відповідь)
Транзакція N+1: Master → 0xFF·64      Slave → TELEM_ALL
```

### Фреймування

Кожен кадр кодується через `frame_codec` (COBS + CRC32/MPEG-2):

```
frame_encode([msg_id | payload]) → COBS-кадр + 0x00
```

Кадр доповнюється нулями до 64 байт. Нулі після кінця COBS-кадру є валідними
delimiter'ами і ігноруються при декодуванні.

---

## Таблиця повідомлень

| `msg_id` | Назва | Напрям | Payload | Фрейм |
|----------|-------|--------|---------|-------|
| `0x01` | `READ_ALL` | M→S | — | 7B → 64B |
| `0x02` | `TELEM_ALL` | S→M | `ehub_telem_t` (50B) | 57B → 64B |
| `0x10` | `CALIBRATE_ZERO` | M→S | `ehub_calibrate_t` (1B) | 8B → 64B |
| `0x11` | `ACK` | S→M | `ehub_ack_t` (2B) | 9B → 64B |
| `0x20` | `CONFIG_CH_WRITE` | M→S | `ehub_ch_config_payload_t` (19B) | 26B → 64B |
| `0x21` | `CONFIG_CH_READ` | M→S | `uint8_t mask` (1B) | 8B → 64B |
| `0x22` | `CONFIG_CH_DATA` | S→M | `ehub_ch_config_payload_t` (19B) | 26B → 64B |
| `0x23` | `CONFIG_GLOB_WRITE` | M→S | `ehub_global_config_t` (3B) | 10B → 64B |
| `0x24` | `CONFIG_GLOB_READ` | M→S | — | 7B → 64B |
| `0x25` | `CONFIG_GLOB_DATA` | S→M | `ehub_global_config_t` (3B) | 10B → 64B |
| `0x26` | `CONFIG_UKF_WRITE` | M→S | `ehub_ukf_write_t` (33B) | 40B → 64B |
| `0x27` | `CONFIG_UKF_READ` | M→S | `ehub_ukf_read_t` (1B) | 8B → 64B |
| `0x28` | `CONFIG_UKF_DATA` | S→M | `ehub_ukf_data_t` (33B) | 40B → 64B |

> Розмір фрейму: `FRAME_ENCODED_SIZE(1 + payload) = (payload+5) + (payload+5)/254 + 2`

---

## Структури даних

### Телеметрія

```c
/* Per-channel: 8B */
typedef struct __attribute__((packed)) {
    float   angle_rad;      /* θ — абсолютна позиція, рад          */
    int16_t omega_crad_s;   /* ω × 100 — рад/с, діапазон ±327.67  */
    int16_t alpha_drad_s2;  /* α × 10  — рад/с², діапазон ±3276.7 */
} ehub_ch_telem_t;

/* Всі 6 каналів: 50B */
typedef struct __attribute__((packed)) {
    ehub_ch_telem_t ch[EHUB_CH_COUNT];  /* 48B */
    uint8_t status;  /* код помилки (див. нижче) */
    uint8_t seq;     /* лічильник оновлень UKF, roll-over OK */
} ehub_telem_t;
```

**Поле `status`:**

| Значення | Значення |
|----------|---------|
| `0x00` | Все OK |
| `0x01..0x06` | Втрата сигналу каналу 0..5 |
| `0x07` | CRC помилка останньої команди |
| `bit7 = 1` | Є ще помилки (це перша/критична) |

Приклад: `0x83` = помилка каналу 3, є додаткові помилки.

### Команди

```c
/* CALIBRATE_ZERO: 1B */
typedef struct __attribute__((packed)) {
    uint8_t mask;  /* bitmask каналів, 0x3F = всі */
} ehub_calibrate_t;

/* ACK: 2B */
typedef struct __attribute__((packed)) {
    uint8_t ack_for;  /* msg_id команди що підтверджується */
    uint8_t status;   /* 0x00=OK  0x01=ERROR  0x02=BUSY    */
} ehub_ack_t;
```

### Конфігурація каналів

```c
/* Per-channel config: 3B */
typedef struct __attribute__((packed)) {
    uint16_t cpr;    /* counts per revolution після X4 */
    uint8_t  flags;  /* bit0: 1 = напрям інвертовано   */
} ehub_ch_config_t;

/* CONFIG_CH_WRITE / CONFIG_CH_DATA payload: 19B */
typedef struct __attribute__((packed)) {
    uint8_t          mask;               /* bitmask каналів; CONFIG_CH_DATA → 0x3F */
    ehub_ch_config_t ch[EHUB_CH_COUNT];  /* 18B */
} ehub_ch_config_payload_t;
```

### Глобальна конфігурація

```c
/* CONFIG_GLOB_WRITE / CONFIG_GLOB_DATA payload: 3B */
typedef struct __attribute__((packed)) {
    uint16_t update_period_us;  /* період оновлення UKF, 0 = 100 мкс (10 кГц) */
    uint8_t  flags;             /* зарезервовано                                */
} ehub_global_config_t;
```

### Конфігурація UKF

```c
/* UKF parameters: 32B */
typedef struct __attribute__((packed)) {
    float q_theta;   /* шум процесу: позиція      (рад²)       */
    float q_omega;   /* шум процесу: швидкість    (рад²/с²)    */
    float q_alpha;   /* шум процесу: прискорення  (рад²/с⁴)    */
    float r_theta;   /* шум вимірювання θ         (рад²)        */
    float r_omega;   /* шум вимірювання ω з IC    ((рад/с)²)    */
    float p0_theta;  /* початкова дисперсія θ                   */
    float p0_omega;  /* початкова дисперсія ω                   */
    float p0_alpha;  /* початкова дисперсія α                   */
} ehub_ukf_config_t;

/* CONFIG_UKF_WRITE payload: 33B
 * mask = 0x3F → однакові параметри для всіх каналів           */
typedef struct __attribute__((packed)) {
    uint8_t           mask;  /* bitmask каналів */
    ehub_ukf_config_t ukf;   /* 32B             */
} ehub_ukf_write_t;

/* CONFIG_UKF_READ payload: 1B */
typedef struct __attribute__((packed)) {
    uint8_t channel;  /* 0..5 */
} ehub_ukf_read_t;

/* CONFIG_UKF_DATA payload: 33B */
typedef struct __attribute__((packed)) {
    uint8_t           channel;  /* який канал */
    ehub_ukf_config_t ukf;      /* 32B        */
} ehub_ukf_data_t;
```

---

## Helpers: кодування float → int16

```c
static inline int16_t ehub_omega_encode(float w)    { return (int16_t)(w * 100.0f); }
static inline float   ehub_omega_decode(int16_t v)  { return v * 0.01f; }
static inline int16_t ehub_alpha_encode(float a)    { return (int16_t)(a * 10.0f); }
static inline float   ehub_alpha_decode(int16_t v)  { return v * 0.1f; }
```

---

## Константи

```c
#define EHUB_CH_COUNT    6U
#define EHUB_FRAME_SIZE  64U

/* msg_id */
#define EHUB_MSG_READ_ALL           0x01U
#define EHUB_MSG_TELEM_ALL          0x02U
#define EHUB_MSG_CALIBRATE_ZERO     0x10U
#define EHUB_MSG_ACK                0x11U
#define EHUB_MSG_CONFIG_CH_WRITE    0x20U
#define EHUB_MSG_CONFIG_CH_READ     0x21U
#define EHUB_MSG_CONFIG_CH_DATA     0x22U
#define EHUB_MSG_CONFIG_GLOB_WRITE  0x23U
#define EHUB_MSG_CONFIG_GLOB_READ   0x24U
#define EHUB_MSG_CONFIG_GLOB_DATA   0x25U
#define EHUB_MSG_CONFIG_UKF_WRITE   0x26U
#define EHUB_MSG_CONFIG_UKF_READ    0x27U
#define EHUB_MSG_CONFIG_UKF_DATA    0x28U

/* ACK status */
#define EHUB_ACK_OK    0x00U
#define EHUB_ACK_ERROR 0x01U
#define EHUB_ACK_BUSY  0x02U

/* status byte */
#define EHUB_ERR_NONE      0x00U
#define EHUB_ERR_CH0       0x01U
#define EHUB_ERR_CH1       0x02U
#define EHUB_ERR_CH2       0x03U
#define EHUB_ERR_CH3       0x04U
#define EHUB_ERR_CH4       0x05U
#define EHUB_ERR_CH5       0x06U
#define EHUB_ERR_CRC       0x07U
#define EHUB_STATUS_MULTI  0x80U
```

---

## Приклад ініціалізації (master side)

```c
/* 1. Записати UKF параметри для всіх каналів */
ehub_ukf_write_t ukf_cfg = {
    .mask = 0x3F,
    .ukf  = {
        .q_theta  = 1e-6f, .q_omega = 1e-4f, .q_alpha = 1e-2f,
        .r_theta  = 1e-4f, .r_omega = 0.1f,
        .p0_theta = 0.1f,  .p0_omega = 1.0f, .p0_alpha = 10.0f,
    },
};
ehub_send(EHUB_MSG_CONFIG_UKF_WRITE, &ukf_cfg, sizeof(ukf_cfg));

/* 2. Записати CPR для всіх каналів */
ehub_ch_config_payload_t ch_cfg = { .mask = 0x3F };
for (int i = 0; i < EHUB_CH_COUNT; i++)
    ch_cfg.ch[i] = (ehub_ch_config_t){ .cpr = 4000, .flags = 0 };
ehub_send(EHUB_MSG_CONFIG_CH_WRITE, &ch_cfg, sizeof(ch_cfg));

/* 3. Скалібрувати нуль всіх каналів */
ehub_calibrate_t cal = { .mask = 0x3F };
ehub_send(EHUB_MSG_CALIBRATE_ZERO, &cal, sizeof(cal));

/* 4. Control loop @ 1 кГц */
while (1) {
    ehub_telem_t telem;
    ehub_read_all(&telem);              /* SPI 64B транзакція   */

    if (telem.status != EHUB_ERR_NONE)
        handle_error(telem.status);

    float omega = ehub_omega_decode(telem.ch[0].omega_crad_s);
    /* ... */
}
```
