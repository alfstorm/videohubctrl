/*
 * Blackmagic Design Videohub control application
 * Copyright (C) 2014 Unix Solutions Ltd.
 * Written by Georgi Chorbadzhiyski
 *
 * Released under MIT license.
 * See LICENSE-MIT.txt for license terms.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include "data.h"
#include "cmd.h"
#include "net.h"
#include "util.h"
#include "version.h"

#include "libfuncs/libfuncs.h"

int verbose;
int quiet;

static struct videohub_data maindata;
static int show_info = 1;
static int show_monitor = 0;

static const char *program_id = PROGRAM_NAME " Version: " VERSION " Git: " GIT_VER;

static const char short_options[] = "s:p:qvhVim";

static const struct option long_options[] = {
	{ "host",				required_argument, NULL, 's' },
	{ "port",				required_argument, NULL, 'p' },
	{ "quiet",				no_argument,       NULL, 'q' },
	{ "verbose",			no_argument,       NULL, 'v' },
	{ "help",				no_argument,       NULL, 'h' },
	{ "version",			no_argument,       NULL, 'V' },
	{ "info",				no_argument,       NULL, 'i' },
	{ "monitor",			no_argument,       NULL, 'm' },
	{ 0, 0, 0, 0 }
};

static void show_help(struct videohub_data *data) {
	printf("\n");
	printf(" Usage: " PROGRAM_NAME " --host <host> [..commands..]\n");
	printf("\n");
	printf("Main options:\n");
	printf(" -s --host <hostname>       | Set device hostname.\n");
	printf(" -p --port <port_number>    | Set device port (default: 9990).\n");
	printf("\n");
	printf("Misc options:\n");
	printf(" -v --verbose               | Increase logging verbosity.\n");
	printf(" -q --quiet                 | Suppress warnings.\n");
	printf(" -h --help                  | Show help screen.\n");
	printf(" -V --version               | Show program version.\n");
	printf("\n");
	printf("Commands:\n");
	printf(" -i --info                  | Show device info (default command).\n");
	printf(" -m --monitor               | Show real time monitor for config changes.\n");
	printf("\n");
}

static void parse_options(struct videohub_data *data, int argc, char **argv) {
	int j, err = 0;
	// Set defaults
	data->dev_port = "9990";
	while ((j = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch (j) {
			case 's': // --host
				data->dev_host = optarg;
				break;
			case 'p': // --port
				data->dev_port = optarg;
				break;
			case 'v': // --verbose
				verbose++;
				if (verbose)
					quiet = 0; // Disable quiet
				break;
			case 'q': // --quiet
				quiet = !quiet;
				break;
			case 'i': // --info
				show_info = 1;
				break;
			case 'm': // --monitor
				show_monitor = 1;
				break;
			case 'h': // --help
				show_help(data);
				exit(EXIT_SUCCESS);
			case 'V': // --version
				// program_id is already printed on startup, just exit.
				exit(EXIT_SUCCESS);
		}
	}

	if (!data->dev_host || !strtoul(data->dev_port, NULL, 10))
		err = 1;

	if (err) {
		show_help(data);
		if (!data->dev_host)
			fprintf(stderr, "ERROR: host is not set. Use --host option.\n");
		exit(EXIT_FAILURE);
	}

	v("Device address: %s:%s\n", data->dev_host, data->dev_port);
}

static void print_device_desc(struct device_desc *d) {
	if (!strlen(d->protocol_ver) || !strlen(d->model_name))
		die("The device does not return protocol version and model name!");
	printf("\n");
	printf("Protocol version: %s\n", d->protocol_ver);
	printf("Model name: %s\n", d->model_name);
	printf("Unique ID: %s\n", d->unique_id);
	printf("Video inputs: %u\n", d->num_video_inputs);
	printf("Video processing units: %u\n", d->num_video_processing_units);
	printf("Video outputs: %u\n", d->num_video_outputs);
	printf("Video monitoring outputs: %u\n", d->num_video_monitoring_outputs);
	printf("Serial ports: %u\n", d->num_serial_ports);
}

static void printf_line(int len) {
	int i;
	for (i = 0; i < len; i++)
		printf("-");
	printf("\n");
}

static void print_device_settings(struct videohub_data *d) {
	unsigned int i;
	printf("\n");
	printf_line(70);
	printf("|  # | x | %-27s | %-27s |\n", "Input name", "Output name");
	printf_line(70);
	for(i = 0; i < d->device.num_video_inputs; i++) {
		printf("| %2d | %c | %-27s | %-27s |\n",
			i + 1,
			d->outputs[i].locked ? 'L' : ' ',
			d->inputs[d->outputs[i].routed_to].name,
			d->outputs[i].name
		);
	}
	printf_line(70);
}

static int read_device_command_stream(struct videohub_data *d) {
	int ret, ncommands = 0;
	char buf[8192 + 1];
	memset(buf, 0, sizeof(buf));
	while ((ret = fdread_ex(d->dev_fd, buf, sizeof(buf) - 1, 5, 0, 0)) >= 0) {
		ncommands += parse_text_buffer(d, buf);
		memset(buf, 0, sizeof(buf));
	}
	return ncommands;
}

int main(int argc, char **argv) {
	struct videohub_data *data = &maindata;

	printf("%s\n", program_id);

	parse_options(data, argc, argv);
	set_log_io_errors(0);

	data->dev_fd = connect_client(SOCK_STREAM, data->dev_host, data->dev_port);
	if (data->dev_fd < 0)
		exit(EXIT_FAILURE);

	read_device_command_stream(data);
	if (show_monitor) {
		while (1) {
			printf("\e[2J\e[H"); // Clear screen
			printf("%s\n", program_id);
			print_device_desc(&data->device);
			print_device_settings(data);
			fflush(stdout);
			do {
				sleep(1);
			} while (read_device_command_stream(data) == 0);
		}
	} else if (show_info) {
		print_device_desc(&data->device);
		print_device_settings(data);
		fflush(stdout);
	}

	shutdown_fd(&data->dev_fd);

	return 0;
}
