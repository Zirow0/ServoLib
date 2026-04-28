/**
 * @file config.h
 * @brief Конфігурація бібліотеки ServoLib
 *
 * Параметри алгоритмів керування — незалежні від платформи.
 * Апаратна конфігурація знаходиться в Board/<target>/board_config.h
 */

#ifndef SERVOCORE_CONFIG_H
#define SERVOCORE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "core.h"

/* Позиційне керування --------------------------------------------------------*/

/** @brief Поріг досягнення цільової позиції (градуси) */
#define POSITION_ERROR_THRESHOLD   0.5f

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CONFIG_H */
