/* Copyright (C) 2014 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#include "PS4Parser.h"

// To enable serial debugging see "settings.h"
//#define PRINTREPORT // Uncomment to print the report send by the PS4 Controller

bool PS4Parser::checkDpad(ButtonEnum b) {
        switch (b) {
                case UP:
                        return ps4Data.btn.dpad == DPAD_LEFT_UP || ps4Data.btn.dpad == DPAD_UP || ps4Data.btn.dpad == DPAD_UP_RIGHT;
                case RIGHT:
                        return ps4Data.btn.dpad == DPAD_UP_RIGHT || ps4Data.btn.dpad == DPAD_RIGHT || ps4Data.btn.dpad == DPAD_RIGHT_DOWN;
                case DOWN:
                        return ps4Data.btn.dpad == DPAD_RIGHT_DOWN || ps4Data.btn.dpad == DPAD_DOWN || ps4Data.btn.dpad == DPAD_DOWN_LEFT;
                case LEFT:
                        return ps4Data.btn.dpad == DPAD_DOWN_LEFT || ps4Data.btn.dpad == DPAD_LEFT || ps4Data.btn.dpad == DPAD_LEFT_UP;
                default:
                        return false;
        }
}

bool PS4Parser::getButtonPress(ButtonEnum b) {
	if (b <= LEFT) // Dpad
		return checkDpad(b);
	else {
		uint8_t button = pgm_read_byte(&PS4_BUTTONS[(uint8_t)b]);
		uint8_t index = button < 8 ? 0 : button < 16 ? 1 : 2;
		uint8_t mask = 1 << (button - 8 * index);
		return ps4Data.btn.val[index] & mask;
	}
}

bool PS4Parser::getButtonClick(ButtonEnum b) {
	uint8_t mask, index = 0;
	if (b <= LEFT) // Dpad
		mask = 1 << b;
	else {
		uint8_t button = pgm_read_byte(&PS4_BUTTONS[(uint8_t)b]);
		index = button < 8 ? 0 : button < 16 ? 1 : 2;
		mask = 1 << (button - 8 * index);
	}

	bool click = buttonClickState.val[index] & mask;
	buttonClickState.val[index] &= ~mask; // Clear "click" event
	return click;
}

uint8_t PS4Parser::getAnalogButton(ButtonEnum b) {
	if (b == L2) // These are the only analog buttons on the controller
		return ps4Data.trigger[0];
	else if (b == R2)
		return ps4Data.trigger[1];
	return 0;
}

uint8_t PS4Parser::getAnalogHat(AnalogHatEnum a) {
	return ps4Data.hatValue[(uint8_t)a];
}

void PS4Parser::Parse(uint8_t len, uint8_t *buf) {
	if (len > 0 && buf)  {
#ifdef PRINTREPORT
		for (uint8_t i = 0; i < len; i++) {
			D_PrintHex<uint8_t > (buf[i], 0x80);
			Notify(PSTR(" "), 0x80);
		}
		Notify(PSTR("\r\n"), 0x80);
#endif

		memcpy(&ps4Data, buf, min(len, sizeof(ps4Data)));
		if (ps4Data.reportId != 0x01) {
#ifdef DEBUG_USB_HOST
                	Notify(PSTR("\r\nUnknown report id"), 0x80);
#endif
                	return;
		}

		for (uint8_t i = 0; i < sizeof(ps4Data.btn); i++) {
			if (ps4Data.btn.val[i] != oldButtonState.val[i]) { // Check if anything has changed
				buttonClickState.val[i] = ps4Data.btn.val[i] & ~oldButtonState.val[i]; // Update click state variable
				oldButtonState.val[i] = ps4Data.btn.val[i];
				if (i == 0) { // The DPAD buttons does not set the different bits, but set a value corresponding to the buttons pressed, we will simply set the bits ourself
					uint8_t newDpad = 0;
					if (checkDpad(UP))
						newDpad |= 1 << UP;
					if (checkDpad(RIGHT))
						newDpad |= 1 << RIGHT;
					if (checkDpad(DOWN))
						newDpad |= 1 << DOWN;
					if (checkDpad(LEFT))
						newDpad |= 1 << LEFT;
					if (newDpad != oldDpad) {
						buttonClickState.dpad = newDpad & ~oldDpad; // Override values
						oldDpad = newDpad;
					}
				}
			}
		}
	}
}