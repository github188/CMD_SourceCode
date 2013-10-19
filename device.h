/*
 * device_init.h
 * 
 * Copyright 2013 qinbh <buaaqbh@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
 * 02110-1301  USA.
 * 
 */

#ifndef DEVICE_INIT_H
#define DEVICE_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

#define SHELL_MAX_BUF_LEN 512
#define MAX_SHELL_OUTPU_LEN	2048

#define CMA_CONFIG_FILE   "cma_config.ini"

#define WLAN_MODULE "/lib/modules/ath.ko"
#define WLAN_DEVICE	"wlan0"

extern struct list_head s_head;

typedef enum {
	 DEVICE_3G,
	 DEVICE_WIFI,
} cma_device_t;

extern int rtc_timer(int interval);

extern int Device_Env_init(void);
extern int Device_power_ctl(cma_device_t dev, int powerOn);
extern int Device_wifi_init(void);
extern int Device_W3G_init(void);
extern int Device_eth_init(void);
extern int Device_can_init(void);
extern int Device_get_basic_info(status_basic_info_t *dev);
extern int Device_get_working_status(status_working_t *status);
extern int Device_getNet_info(Ctl_net_adap_t  *adap);
extern int Device_setNet_info(Ctl_net_adap_t  *adap);
extern int Device_getId(byte *id, byte *org_id);
extern int Device_setId(byte *id, byte *org_id);
extern int Device_reset(byte type);
extern int Device_setWakeup_time(int revival_time, usint revival_cycle, usint duration_time);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* DEVICE_INIT_H */