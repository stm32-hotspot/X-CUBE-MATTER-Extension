/*
 * qr_code.h
 *
 *  Created on: Jul 25, 2023
 *      Author: pasztore
 */

#ifndef APPLICATION_USER_CORE_SRC_QR_CODE_H_
#define APPLICATION_USER_CORE_SRC_QR_CODE_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	#define QR_LTX 0
	#define QR_LTY 0
	#define QR_LENX 64
	#define QR_LENY 64
	#define QR_USE_VERSION_1
//	#define QR_USE_VERSION_3

#ifdef QR_USE_VERSION_1
	#define QR_USE_VERSION 1
	#define QR_SEGMENTS_NUM 21
	#define QR_PIXEL_SIZE 3
	#define QR_USE_ECC qrcodegen_Ecc_LOW
#else
	#define QR_USE_VERSION 3
	#define QR_SEGMENTS_NUM 29
	#define QR_PIXEL_SIZE 2
	#define QR_USE_ECC qrcodegen_Ecc_HIGH
#endif

	#define QR_IMG_SIZE (QR_SEGMENTS_NUM*QR_PIXEL_SIZE)

	//void create_qr_img(const char *payload, uint8_t *img);
	void create_qr_img(char *qr_payload);
	void draw_qr_img(void);

	extern uint8_t qr_img[QR_IMG_SIZE][QR_IMG_SIZE];

#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_USER_CORE_SRC_QR_CODE_H_ */
