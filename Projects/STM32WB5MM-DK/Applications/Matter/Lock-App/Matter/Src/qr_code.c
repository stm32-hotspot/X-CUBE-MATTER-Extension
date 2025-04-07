/*
 * qr_code.c
 *
 *  Created on: Jul 25, 2023
 *      Author: pasztore
 */


#include "qr_code.h"
#include "qrcodegen.h"
#include "stm32_lcd.h"

//char qr_payload[] = "MT:4CT9142C00C60648G00";
uint8_t qr_img[QR_IMG_SIZE][QR_IMG_SIZE];


void create_qr_img(char *qr_payload)
{
	uint8_t qr_buff[qrcodegen_BUFFER_LEN_FOR_VERSION(QR_USE_VERSION)];
	uint8_t qr_temp_buff[qrcodegen_BUFFER_LEN_FOR_VERSION(QR_USE_VERSION)];

	bool ok = qrcodegen_encodeText(qr_payload, qr_temp_buff, qr_buff, QR_USE_ECC,
			QR_USE_VERSION, QR_USE_VERSION, qrcodegen_Mask_AUTO, true);

	uint8_t size = qrcodegen_getSize(qr_buff);
	bool dark_pixel = false;
	uint8_t qr_x, qr_y, disp_x = 0, disp_y = 0;

#ifdef QR_USE_VERSION_1
	for (qr_y = 0, disp_y = 0; qr_y < size; qr_y++, disp_y+=3) {
		for (qr_x = 0, disp_x = 0; qr_x < size; qr_x++, disp_x+=3) {

			dark_pixel = qrcodegen_getModule(qr_buff, qr_x, qr_y);
			if (dark_pixel)
			{
				qr_img[disp_y][disp_x] = 255;
				qr_img[disp_y][disp_x+1] = 255;
				qr_img[disp_y][disp_x+2] = 255;
				qr_img[disp_y+1][disp_x] = 255;
				qr_img[disp_y+1][disp_x+1] = 255;
				qr_img[disp_y+1][disp_x+2] = 255;
				qr_img[disp_y+2][disp_x] = 255;
				qr_img[disp_y+2][disp_x+1] = 255;
				qr_img[disp_y+2][disp_x+2] = 255;
			} else
			{
				qr_img[disp_y][disp_x] = 0;
				qr_img[disp_y][disp_x+1] = 0;
				qr_img[disp_y][disp_x+2] = 0;
				qr_img[disp_y+1][disp_x] = 0;
				qr_img[disp_y+1][disp_x+1] = 0;
				qr_img[disp_y+1][disp_x+2] = 0;
				qr_img[disp_y+2][disp_x] = 0;
				qr_img[disp_y+2][disp_x+1] = 0;
				qr_img[disp_y+2][disp_x+2] = 0;
			}
		}
	}
#else
	for (qr_y = 0, disp_y = 0; qr_y < size; qr_y++, disp_y+=2) {
		for (qr_x = 0, disp_x = 0; qr_x < size; qr_x++, disp_x+=2) {

			dark_pixel = qrcodegen_getModule(qr_buff, qr_x, qr_y);
			if (dark_pixel)
			{
				qr_img[disp_y][disp_x] = 255;
				qr_img[disp_y][disp_x+1] = 255;
				qr_img[disp_y+1][disp_x] = 255;
				qr_img[disp_y+1][disp_x+1] = 255;
			} else
			{
				qr_img[disp_y][disp_x] = 0;
				qr_img[disp_y][disp_x+1] = 0;
				qr_img[disp_y+1][disp_x] = 0;
				qr_img[disp_y+1][disp_x+1] = 0;
			}
		}
	}
#endif
}

void draw_qr_img()
{
	for (uint8_t disp_y = 0; disp_y < QR_IMG_SIZE; disp_y++) {
		for (uint8_t disp_x = 0; disp_x < QR_IMG_SIZE; disp_x++) {
			UTIL_LCD_SetPixel(disp_x, disp_y, qr_img[disp_y][disp_x]);
		}
	}
}

