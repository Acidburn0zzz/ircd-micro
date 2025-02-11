/* ircd-micro, main.c -- entry point
   Copyright (C) 2013 Alex Iadicicco

   This file is protected under the terms contained
   in the COPYING file in the project root */

#include "ircd.h"

struct timeval NOW;

void sync_time(void)
{
	gettimeofday(&NOW, NULL);
}

mowgli_eventloop_t *base_ev;
mowgli_dns_t *base_dns;
u_ts_t started;

short opt_port = -1;

int usage(char *argv0, int code)
{
	printf("Usage: %s [OPTIONS]\n", argv0);
	printf("Options:\n");
	printf("  -v         Be verbose. Supply multiple times for more verbosity.\n");
	printf("  -h         Print this help and exit\n");
	exit(code);
}

#define INIT(fn) if ((err = (fn)()) < 0) return err
#define COMMAND_DEF(cmds) extern u_cmd cmds[]
#define COMMAND(cmds) if ((err = u_cmds_reg(cmds)) < 0) return err
COMMAND_DEF(c_generic);
COMMAND_DEF(c_message);
COMMAND_DEF(c_reg);
COMMAND_DEF(c_server);
COMMAND_DEF(c_user);
int init(void)
{
	int err;
	FILE *f;

	sync_time();

	u_log(LG_DEBUG, "Seeding random number generator...");
	srand(NOW.tv_usec);

	signal(SIGPIPE, SIG_IGN);

	base_ev = mowgli_eventloop_create();
	base_dns = mowgli_dns_create(base_ev, MOWGLI_DNS_TYPE_ASYNC);

	INIT(init_util);
	INIT(init_module);
	INIT(init_hook);
	INIT(init_conf);
	INIT(init_conn);
	INIT(init_upgrade);
	INIT(init_auth);
	INIT(init_user);
	INIT(init_cmd);
	INIT(init_server);
	INIT(init_chan);
	INIT(init_sendto);
	COMMAND(c_generic);
	COMMAND(c_message);
	COMMAND(c_reg);
	COMMAND(c_server);
	COMMAND(c_user);

	mowgli_timer_add(base_ev, "ping", u_conn_check_ping_all, base_ev, 10);

	if (opt_port != -1 && !u_conn_origin_create(base_ev, INADDR_ANY, opt_port))
		return -1;

	f = fopen("etc/micro.conf", "r");
	if (f == NULL) {
		u_log(LG_SEVERE, "Could not find etc/micro.conf!");
		return -1;
	}
	u_conf_read(f);
	fclose(f);

	return 0;
}

extern char *optarg;

int main(int argc, char **argv)
{
	int c;

	gettimeofday(&NOW, NULL);
	started = NOW.tv_sec;

	u_log(LG_INFO, "%s starting...", PACKAGE_FULLNAME);

	while ((c = getopt(argc, argv, "vhp:")) != -1) {
		switch(c) {
		case 'v':
			if (u_log_level < LG_FINE)
				u_log_level++;
			break;
		case 'h':
			usage(argv[0], 0);
			break;
		case 'p':
			u_log(LG_WARN, "Use of -p is deprecated. Please use the"
			      " config file to specify listeners");
			opt_port = atoi(optarg);
			break;
		case '?':
			usage(argv[0], 1);
			break;
		}
	}

	if (init() < 0) {
		u_log(LG_ERROR, "Initialization failed");
		return 1;
	}

	u_log(LG_INFO, "Entering IO loop");

	mowgli_eventloop_run(base_ev);

	u_log(LG_VERBOSE, "IO loop died. Bye bye!");

	return 0;
}
