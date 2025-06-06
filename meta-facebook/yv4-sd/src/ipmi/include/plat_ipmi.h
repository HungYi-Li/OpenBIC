/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PLAT_IPMI_H
#define PLAT_IPMI_H

enum GET_HTTP_BOOT_ATTR {
	GET_HTTP_BOOT_SIZE = 0x00,
	GET_HTTP_BOOT_CRC32 = 0x01,
	GET_HTTP_BOOT_MAX = 0x02,
};

enum WDT_TIMER_ACTIONS {
	NO_ACTION = 0x00,
	HARD_RESET = 0x01,
	POWER_DOWN = 0x02,
	POWER_CYCLE = 0x03,
};

void event_resend_work_handler(struct k_work *work);

#endif
