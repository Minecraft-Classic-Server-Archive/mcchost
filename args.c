#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <limits.h>

#include "args.h"

char ** program_args = 0;

void
process_args(int argc, char **argv)
{
    program_args = calloc(argc+8, sizeof(*program_args));
    int bc = 1, plen = strlen(argv[0]);

    getprogram(argv[0]);

    for(int pass2 = 0; pass2<2; pass2++)
    {
	server_t tmpserver = {0};
	if (!pass2)
	    server = &tmpserver;

	for(int ar = 1; ar<argc; ar++) {
	    if (argv[ar][0] != '-') {
		if (!pass2)
		    fprintf(stderr, "Skipping argument '%s'\n", argv[ar]);
		continue;
	    }

	    int addarg = 1;
	    do {
		if (ar+1 < argc) {
		    if (strcmp(argv[ar], "-name") == 0) {
			strncpy(IGNORE_VOLATILE_CHARP(server->name), argv[ar+1], sizeof(server->name)-1);
			ar++; addarg++;
			break;
		    }

		    if (strcmp(argv[ar], "-motd") == 0) {
			strncpy(IGNORE_VOLATILE_CHARP(server->motd), argv[ar+1], sizeof(server->motd)-1);
			ar++; addarg++;
			break;
		    }

		    if (strcmp(argv[ar], "-salt") == 0 || strcmp(argv[ar], "-secret") == 0) {
			ar++; addarg++;
			strncpy(IGNORE_VOLATILE_CHARP(server->secret), argv[ar], sizeof(server->secret)-1);
			// Try to hide the argument used as salt from ps(1)
			if (pass2)
			    for(char * p = argv[ar]; *p; p++) *p = 'X';
			break;
		    }

		    if (strcmp(argv[ar], "-heartbeat") == 0) {
			strncpy(heartbeat_url, argv[ar+1], sizeof(heartbeat_url)-1);
			ar++; addarg++;
			enable_heartbeat_poll = 1;
			break;
		    }

		    if (strcmp(argv[ar], "-port") == 0) {
			tcp_port_no = atoi(argv[ar+1]);
			ar++; addarg++;
			break;
		    }

		    if (strcmp(argv[ar], "-log") == 0) {
			strncpy(logfile_pattern, argv[ar+1], sizeof(logfile_pattern)-1);
			ar++; addarg++;
			break;
		    }

		    if (strcmp(argv[ar], "-dir") == 0) {
			if (!pass2)
			    if(chdir(argv[ar+1]) < 0) {
				perror(argv[ar+1]);
				exit(1);
			    }
			ar++;
			addarg = 0;
			break;
		    }
		}

		if (strcmp(argv[ar], "-saveconf") == 0) {
		    save_conf = 1;
		    addarg = 0;
		    break;
		}

		if (strcmp(argv[ar], "-inetd") == 0) {
		    inetd_mode = 1;
		    start_tcp_server = 0;
		    enable_heartbeat_poll = 0;
		    break;
		}

		if (strcmp(argv[ar], "-tcp") == 0) {
		    inetd_mode = 0;
		    start_tcp_server = 1;
		    enable_heartbeat_poll = 0;
		    break;
		}

		if (strcmp(argv[ar], "-net") == 0) {
		    inetd_mode = 0;
		    start_tcp_server = 1;
		    enable_heartbeat_poll = 1;
		    break;
		}

		if (strcmp(argv[ar], "-cron") == 0) {
		    start_cron_task = 1;
		    break;
		}

		if (strcmp(argv[ar], "-private") == 0) {
		    server->private = 1;
		    break;
		}

		if (strcmp(argv[ar], "-public") == 0) {
		    server->private = 0;
		    break;
		}

		if (strcmp(argv[ar], "-runonce") == 0) {
		    server_runonce = 1;
		    break;
		}

		if (strcmp(argv[ar], "-detach") == 0) {
		    detach_tcp_server = 1;
		    addarg = 0;
		    break;
		}

		if (strcmp(argv[ar], "-no-detach") == 0) {
		    detach_tcp_server = 0;
		    addarg = 0;
		    break;
		}

		if (strcmp(argv[ar], "-log-stderr") == 0) {
		    log_to_stderr = 1;
		    break;
		}

		if (strcmp(argv[ar], "-nocpe") == 0) {
		    server->cpe_disabled = 1;
		    break;
		}

		fprintf(stderr, "Invalid argument '%s'\n", argv[ar]);
		exit(1);
	    } while(0);

	    if (!pass2 && addarg) {
		if (addarg == 2) {
		    program_args[bc++] = strdup(argv[ar-1]);
		    plen += strlen(argv[ar-1])+1;
		}
		program_args[bc++] = strdup(argv[ar]);
		plen += strlen(argv[ar])+1;
	    }
	}

	if (!pass2) {
	    server = 0;
	    init_dirs();
	    open_system_conf();

	    if (server->magic != MAP_MAGIC || server->magic2 != MAP_MAGIC2)
		*server = (server_t){
		    .magic = MAP_MAGIC, .magic2 = MAP_MAGIC2,
		    .software = "MCCHost",
		    .name = "MCCHost Server",
		    .main_level = "main",
		    .save_interval = 300,
		    .backup_interval = 86400,
		    .max_players = 255,
		};

	    load_ini_file(system_ini_fields, SERVER_CONF_NAME, 1, 0);
	}
    }

    // Pad the program args so we get some space after a restart.
    // Also we must turn off -detach to keep the same pid.
    do {
	program_args[bc++] = strdup("-no-detach");
	plen += 11;
    } while(plen < 32);

    struct timeval now;
    gettimeofday(&now, 0);
#ifdef PCG32_INITIALIZER
    // Somewhat better random seed, the whole time, pid and ASLR
    pcg32_srandom(
	now.tv_sec*(uint64_t)1000000 + now.tv_usec,
	(((uintptr_t)&program_args) >> 12) +
	((int64_t)(getpid()) << sizeof(uintptr_t)*4) );
#else
    // A pretty trivial semi-random code, maybe 24bits of randomness.
    srandom(now.tv_sec ^ (now.tv_usec*4294U));
#endif

    if (enable_heartbeat_poll && server->secret[0] == 0) {
	static char base62[] =
	    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	for(int i=0; i<16; i++) {
#ifdef PCG32_INITIALIZER
	    int ch = pcg32_boundedrand(62);
#else
	    int ch = random() % 62;
#endif
	    server->secret[i] = base62[ch];
	}
	server->secret[16] = 0;
	fprintf(stderr, "Generated server secret %s\n", server->secret);
    }
}

LOCAL void
getprogram(char * argv0)
{
    // Is argv0 absolute or a $PATH lookup?
    if (argv0[0] == '/' || strchr(argv0, '/') == 0) {
	program_args[0] = strdup(argv0);
	return;
    }

    // For relative paths try something different.
    char buf[PATH_MAX*2];
    int l = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;

    // /proc/self/exe gives something runnable.
    if (l > 0 && access(buf, X_OK) == 0) {
	// Save it away because we use it for restart after it's been recreated.
	program_args[0] = strdup(buf);
	return;
    }

    // Last try, assume the PATH will get fixed sometime.
    char * p = strrchr(argv0, '/');
    program_args[0] = p?strdup(p+1):strdup("");
}
