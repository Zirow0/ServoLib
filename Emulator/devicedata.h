/*******************************************************************************
 * @file    devicedata.h
 * @brief   OpENer Device Data for ServoLib Emulator
 * @details Device identification constants for EtherNet/IP Identity Object
 *
 * @author  ServoLib Team
 * @date    27.12.2024
 ******************************************************************************/

#ifndef DEVICEDATA_H
#define DEVICEDATA_H

#ifdef __cplusplus
extern "C" {
#endif

/** Vendor ID - Use generic vendor ID for testing (1 = Rockwell Automation) */
#define OPENER_DEVICE_VENDOR_ID         0x0001

/** Device Type - Generic Device (0x2B = Communication Adapter) */
#define OPENER_DEVICE_TYPE              0x002B

/** Product Code - Unique product identifier */
#define OPENER_DEVICE_PRODUCT_CODE      0x1234

/** Device Revision */
#define OPENER_DEVICE_MAJOR_REVISION    1
#define OPENER_DEVICE_MINOR_REVISION    0

/** Product Name - Will be set via CipIdentityInit() */
#define OPENER_DEVICE_NAME              "ServoLib EIP Adapter"

#ifdef __cplusplus
}
#endif

#endif /* DEVICEDATA_H */
