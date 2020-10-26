/*
 * === Main data structures and helpers ===
 *
 * Blackmagic Design Videohub control application
 * Copyright (C) 2014 Unix Solutions Ltd.
 * Written by Georgi Chorbadzhiyski
 *
 * Released under MIT license.
 * See LICENSE-MIT.txt for license terms.
 *
 */

#ifndef DATA_H
#define DATA_H

#include <stdbool.h>

#define MAX_PORTS 288
#define MAX_NAME_LEN 48
#define MAX_RUN_CMDS (288 * 5)
#define NO_PORT ((unsigned int) -1)
#define MAX_ALARMS 32
#define MAX_ALARM_STATUS_LEN 256

struct device_desc {
	bool			dev_present;
	bool			needs_fw_update;
	bool			conf_take_mode;
	char			protocol_ver[16];
	char			model_name[MAX_NAME_LEN];
	char			friendly_name[MAX_NAME_LEN];
	char			unique_id[MAX_NAME_LEN];
};

enum port_status {
	S_UNKNOWN,
	S_NONE,
	S_BNC,
	S_OPTICAL,
	S_THUNDERBOLT,
	S_RS422,
};

enum port_lock {
	PORT_UNLOCKED,
	PORT_LOCKED,
	PORT_LOCKED_OTHER,
};

enum serial_dir {
	DIR_CONTROL,
	DIR_SLAVE,
	DIR_AUTO,
};

struct port {
	char			name[MAX_NAME_LEN];
	// Port statuses are supported only by Universal Videohub
	// The statuses (actually they are connection types) are:
	//  BNC, Optical or None /missing port/ - for input/output
	//  RS422, None                         - for serial ports
	enum port_status	status;
	// For serial ports.
	// The values are:
	//    control - In (Workstation)
	//      slave - Out (Deck)
	//       auto - Automatic
	enum serial_dir	direction;
	unsigned int	routed_to;
	enum port_lock	lock;
};

struct port_set {
	unsigned int	num;
	struct port		port[MAX_PORTS];
};

struct alarm {
	char			name[MAX_NAME_LEN];
	char			status[MAX_ALARM_STATUS_LEN];
};

struct alarm_status {
	unsigned int	num;
	struct alarm	alarm[MAX_ALARMS];
};

struct videohub_data {
	char					*dev_host;
	char					*dev_port;
	int						dev_fd;
	struct device_desc		device;
	struct port_set			inputs;
	struct port_set			outputs;
	struct port_set			mon_outputs;
	struct port_set			serial;
	struct port_set			proc_units;
	struct port_set			frames;
	struct alarm_status		alarms;
};

extern int debug;
extern int quiet;

#define d(fmt, arguments...) \
	do { \
		if (debug) \
			printf("debug: " fmt, ## arguments); \
	} while(0)

#define q(fmt, arguments...) \
	do { \
		if (!quiet) \
			fprintf(stderr, fmt, ## arguments); \
	} while(0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))

#endif
