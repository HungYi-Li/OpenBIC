#include <stdio.h>
#include <stdlib.h>

#include "ipmi.h"
#include "libutil.h"

#include "plat_class.h"
#include "plat_m2.h"
#include "plat_led.h"

void OEM_1S_GET_CARD_TYPE(ipmi_msg *msg)
{
	if (msg == NULL) {
		printf("%s failed due to parameter *msg is NULL\n", __func__);
		return;
	}

	if (msg->data_len != 0) {
		msg->data_len = 0;
		msg->completion_code = CC_INVALID_LENGTH;
		return;
	}

	msg->data_len = 1;
	msg->data[0] = get_board_id();
	msg->completion_code = CC_SUCCESS;
	return;
}

void OEM_1S_SET_SSD_LED(ipmi_msg *msg)
{
	if (msg == NULL) {
		printf("%s failed due to parameter *msg is NULL\n", __func__);
		return;
	}

	if (msg->data_len != 2) {
		msg->data_len = 0;
		msg->completion_code = CC_INVALID_LENGTH;
		return;
	}

	uint8_t dev = msg->data[0];

	if (!m2_prsnt(dev)) {
		msg->data_len = 0;
		msg->completion_code = 0x80; //ssd not present response complete code
		return;
	}

	if (!SSDLEDCtrl(dev, msg->data[1])) {
		msg->data_len = 0;
		msg->completion_code = CC_INVALID_DATA_FIELD;
		return;
	}

	msg->data_len = 0;
	msg->completion_code = CC_SUCCESS;
}

void OEM_1S_GET_SSD_STATUS(ipmi_msg *msg)
{
	if (msg == NULL) {
		printf("%s failed due to parameter *msg is NULL\n", __func__);
		return;
	}

	if (msg->data_len != 1) {
		msg->data_len = 0;
		msg->completion_code = CC_INVALID_LENGTH;
		return;
	}

	uint8_t dev = msg->data[0];
	if (!m2_prsnt(dev)) {
		msg->data_len = 0;
		msg->completion_code = 0x80; //ssd not present response complete code
		return;
	}

	msg->data_len = 1;
	msg->data[0] = GetAmberLEDStat(dev);
	msg->completion_code = CC_SUCCESS;
}