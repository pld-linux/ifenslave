/* Mode: C;
 * ifenslave.c: Configure network interfaces for parallel routing.
 *
 *	This program controls the Linux implementation of running multiple
 *	network interfaces in parallel.
 *
 * Usage:	ifenslave [-v] master-interface slave-interface [metric <N>]
 *
 * Author:	Donald Becker <becker@cesdis.gsfc.nasa.gov>
 *		Copyright 1994-1996 Donald Becker
 *
 *		This program is free software; you can redistribute it
 *		and/or modify it under the terms of the GNU General Public
 *		License as published by the Free Software Foundation.
 *
 *	The author may be reached as becker@CESDIS.gsfc.nasa.gov, or C/O
 *	Center of Excellence in Space Data and Information Sciences
 *	   Code 930.5, Goddard Space Flight Center, Greenbelt MD 20771
 */

static char *version =
"ifenslave.c:v0.07 9/9/97  Donald Becker (becker@cesdis.gsfc.nasa.gov)\n";
static const char *usage_msg =
"Usage: ifenslave [-afrvV] <master-interface> <slave-interface> [metric <N>]\n";

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>

struct option longopts[] = {
 /* { name  has_arg  *flag  val } */
    {"all-interfaces", 0, 0, 'a'},	/* Show all interfaces. */
    {"force",       0, 0, 'f'},		/* Force the operation. */
    {"help", 		0, 0, '?'},		/* Give help */
    {"receive-slave", 0, 0, 'r'},	/* Make a receive-only slave.  */
    {"verbose", 	0, 0, 'v'},		/* Report each action taken.  */
    {"version", 	0, 0, 'V'},		/* Emit version information.  */
    { 0, 0, 0, 0 }
};

/* Command-line flags. */
unsigned int
opt_a = 0,					/* Show-all-interfaces flag. */
opt_f = 0,					/* Force the operation. */
opt_r = 0,					/* Set up a Rx-only slave. */
verbose = 0,					/* Verbose flag. */
opt_version;
int skfd = -1;					/* AF_INET socket for ioctl() calls.	*/

static void if_print(char *ifname);

int
main(int argc, char **argv)
{
	struct ifreq  ifr2, if_hwaddr, if_ipaddr, if_metric, if_mtu, if_dstaddr;
	struct ifreq  if_netmask, if_brdaddr, if_flags;
	int goterr = 0;
	int c, errflag = 0;
	char **spp, *master_ifname, *slave_ifname;

	while ((c = getopt_long(argc, argv, "afrvV?", longopts, 0)) != EOF)
		switch (c) {
		case 'a': opt_a++; break;
		case 'f': opt_f++; break;
		case 'r': opt_r++; break;
		case 'v': verbose++;		break;
		case 'V': opt_version++;	break;
		case '?': errflag++;
		}
	if (errflag) {
		fprintf(stderr, usage_msg);
		return 2;
	}

	if (verbose || opt_version)
		printf(version);

	/* Open a basic socket. */
	if ((skfd = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		perror("socket");
		exit(-1);
	}

#ifdef notdef
	/* Find options scattered throughout the command line.
	   I should change this to use getopt() sometime. */
	argc--; argv++;
	while (*argv[0] == '-') {
		char *argp = *argv++;
		argc--;
		while (*++argp) {
			switch (*argp) {
			default:
				fprintf(stderr, "Unrecognized option '%c'.\n%s",
						argp[0], usage_msg);
				return 2;
			}
		}
	}
#endif

	if (verbose)
		fprintf(stderr, "DEBUG: argc=%d, optind=%d and argv[optind] is %s.\n",
				argc, optind, argv[optind]);

	/* No remaining args means show all interfaces. */
	if (optind == argc) {
		if_print((char *)NULL);
		(void) close(skfd);
		exit(0);
	}

	/* Copy the interface name. */
	spp = argv + optind;
	master_ifname = *spp++;
	slave_ifname = *spp++;

	/* A single args means show the configuration for this interface. */
	if (slave_ifname == NULL) {
		if_print(master_ifname);
		(void) close(skfd);
		exit(0);
	}

	/* Get the vitals from the master interface. */
	strncpy(if_hwaddr.ifr_name, master_ifname, IFNAMSIZ);
	if (ioctl(skfd, SIOCGIFHWADDR, &if_hwaddr) < 0) {
		fprintf(stderr, "SIOCGIFHWADDR on %s failed: %s\n", master_ifname,
				strerror(errno));
		return 1;
	} else {		/* Gotta convert from 'char' to unsigned for printf().  */
		unsigned char *hwaddr = (unsigned char *)if_hwaddr.ifr_hwaddr.sa_data;

		/* The family '1' is ARPHRD_ETHER for ethernet. */
		if (if_hwaddr.ifr_hwaddr.sa_family != 1 && !opt_f) {
			fprintf(stderr, "The specified master interface '%s' is not"
					" ethernet-like.\n  This program is designed to work"
					" with ethernet-like network interfaces.\n"
					" Use the '-f' option to force the operation.\n",
					master_ifname);

			return 1;
		}

		if (verbose)
			printf("The hardware address (SIOCGIFHWADDR) of %s is type %d  "
				   "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x.\n", master_ifname,
				   if_hwaddr.ifr_hwaddr.sa_family, hwaddr[0], hwaddr[1],
				   hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
	}

	{
		struct ifreq *ifra[6] = { &if_ipaddr, &if_mtu, &if_dstaddr,
									 &if_brdaddr, &if_netmask, &if_flags };
		const char *req_name[6] = {
			"IP address", "MTU", "destination address",
			"broadcast address", "netmask", "status flags", };
		const int ioctl_req_type[6] = {
			SIOCGIFADDR, SIOCGIFMTU, SIOCGIFDSTADDR,
			SIOCGIFBRDADDR, SIOCGIFNETMASK, SIOCGIFFLAGS, };
		int i;

		for (i = 0; i < 6; i++) {
			strncpy(ifra[i]->ifr_name, master_ifname, IFNAMSIZ);
			if (ioctl(skfd, ioctl_req_type[i], ifra[i]) < 0) {
				fprintf(stderr,
						"Something broke getting the master's %s: %s.\n",
						req_name[i], strerror(errno));
			}
		}
	}

	do {
		strncpy(ifr2.ifr_name, slave_ifname, IFNAMSIZ);
		if (ioctl(skfd, SIOCGIFFLAGS, &ifr2) < 0) {
			int saved_errno = errno;
			fprintf(stderr, "SIOCGIFFLAGS on %s failed: %s\n", slave_ifname,
					strerror(saved_errno));
			return 1;
		}
		if (ifr2.ifr_flags & IFF_UP) {
			printf("The interface %s is up, shutting it down it to enslave it.\n",
				   slave_ifname);
			ifr2.ifr_flags &= ~IFF_UP;
			if (ioctl(skfd, SIOCSIFFLAGS, &ifr2) < 0) {
				int saved_errno = errno;
				fprintf(stderr, "Shutting down interface %s failed: %s\n",
						slave_ifname, strerror(saved_errno));
			}
		}

		strncpy(if_hwaddr.ifr_name, slave_ifname, IFNAMSIZ);
		if (ioctl(skfd, SIOCSIFHWADDR, &if_hwaddr) < 0) {
			int saved_errno = errno;
			fprintf(stderr, "SIOCSIFHWADDR on %s failed: %s\n", if_hwaddr.ifr_name,
					strerror(saved_errno));
			if (saved_errno == EBUSY)
				fprintf(stderr, "  The slave device %s is busy: it must be"
						" idle before running this command.\n", slave_ifname);
			else if (saved_errno == EOPNOTSUPP)
				fprintf(stderr, "  The slave device you specified does not support"
						" setting the MAC address.\n  Your kernel likely does not"
						" support slave devices.\n");
			else if (saved_errno == EINVAL)
				fprintf(stderr, "  The slave device's address type does not match"
						" the master's address type.\n");
			return 1;
		} else {
			if (verbose) {
				unsigned char *hwaddr = if_hwaddr.ifr_hwaddr.sa_data;
				printf("Set the slave's hardware address to "
					   "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x.\n", hwaddr[0],
					   hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
			}
		}
		
		if (*spp  &&  !strcmp(*spp, "metric")) {
			if (*++spp == NULL) {
				fprintf(stderr, usage_msg);
				exit(2);
			}
			if_metric.ifr_metric = atoi(*spp);
			if (ioctl(skfd, SIOCSIFMETRIC, &if_metric) < 0) {
				fprintf(stderr, "SIOCSIFMETRIC: %s\n", strerror(errno));
				goterr = 1;
			}
			spp++;
		}

		if (strncpy(if_ipaddr.ifr_name, slave_ifname, IFNAMSIZ) <= 0
			|| ioctl(skfd, SIOCSIFADDR, &if_ipaddr) < 0) {
			fprintf(stderr,
					"Something broke setting the slave's address: %s.\n",
					strerror(errno));
		} else {
			if (verbose) {
				unsigned char *ipaddr = if_ipaddr.ifr_addr.sa_data;
				printf("Set the slave's IP address to %d.%d.%d.%d.\n",
					   ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
			}
		}

		if (strncpy(if_mtu.ifr_name, slave_ifname, IFNAMSIZ) <= 0
			|| ioctl(skfd, SIOCSIFMTU, &if_mtu) < 0) {
			fprintf(stderr, "Something broke setting the slave MTU: %s.\n",
					strerror(errno));
		} else {
			if (verbose)
				printf("Set the slave's MTU to %d.\n", if_mtu.ifr_mtu);
		}

		if (strncpy(if_dstaddr.ifr_name, slave_ifname, IFNAMSIZ) <= 0
			|| ioctl(skfd, SIOCSIFDSTADDR, &if_dstaddr) < 0) {
			fprintf(stderr, "Error setting the slave with SIOCSIFDSTADDR: %s.\n",
					strerror(errno));
		} else {
			if (verbose) {
				unsigned char *ipaddr = if_dstaddr.ifr_dstaddr.sa_data;
				printf("Set the slave's destination address to %d.%d.%d.%d.\n",
					   ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
			}
		}

		if (strncpy(if_brdaddr.ifr_name, slave_ifname, IFNAMSIZ) <= 0
			|| ioctl(skfd, SIOCSIFBRDADDR, &if_brdaddr) < 0) {
			fprintf(stderr,
					"Something broke setting the slave broadcast address: %s.\n",
					strerror(errno));
		} else {
			if (verbose) {
				unsigned char *ipaddr = if_brdaddr.ifr_broadaddr.sa_data;
				printf("Set the slave's broadcast address to %d.%d.%d.%d.\n",
					   ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
			}
		}
		
		if (strncpy(if_netmask.ifr_name, slave_ifname, IFNAMSIZ) <= 0
			|| ioctl(skfd, SIOCSIFNETMASK, &if_netmask) < 0) {
			fprintf(stderr,
					"Something broke setting the slave netmask: %s.\n",
					strerror(errno));
		} else {
			if (verbose) {
				unsigned char *ipaddr = if_netmask.ifr_netmask.sa_data;
				printf("Set the slave's netmask to %d.%d.%d.%d.\n",
					   ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
			}
		}
		
		if ((if_flags.ifr_flags &= ~(IFF_SLAVE | IFF_MASTER)) == 0
			|| strncpy(if_flags.ifr_name, slave_ifname, IFNAMSIZ) <= 0
			|| ioctl(skfd, SIOCSIFFLAGS, &if_flags) < 0) {
			fprintf(stderr,
					"Something broke setting the slave flags: %s.\n",
					strerror(errno));
		} else {
			if (verbose)
				printf("Set the slave's flags %4.4x.\n", if_flags.ifr_flags);
		}

		/* Do the real thing: set the second interface as a slave. */
		if ( ! opt_r) {
			strncpy(if_flags.ifr_name, master_ifname, IFNAMSIZ);
			strncpy(if_flags.ifr_slave, slave_ifname, IFNAMSIZ);
			if (ioctl(skfd, SIOCSIFSLAVE, &if_flags) < 0) {
				fprintf(stderr,	"SIOCSIFSLAVE: %s.\n", strerror(errno));
			}
		}
	} while ( (slave_ifname = *spp++) != NULL);

	/* Close the socket. */
	(void) close(skfd);

	return(goterr);
}

static short mif_flags;

/* Get the inteface configuration from the kernel. */
static int if_getconfig(char *ifname)
{
	struct ifreq ifr;
	int metric, mtu;			/* Parameters of the master interface. */
	struct sockaddr dstaddr, broadaddr, netmask;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
		return -1;
	mif_flags = ifr.ifr_flags;
	printf("The result of SIOCGIFFLAGS on %s is %x.\n",
		   ifname, ifr.ifr_flags);

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0)
		return -1;
	printf("The result of SIOCGIFADDR is %2.2x.%2.2x.%2.2x.%2.2x.\n",
		   ifr.ifr_addr.sa_data[0], ifr.ifr_addr.sa_data[1],
		   ifr.ifr_addr.sa_data[2], ifr.ifr_addr.sa_data[3]);

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0)
		return -1;

	{		/* Gotta convert from 'char' to unsigned for printf().  */
		unsigned char *hwaddr = (unsigned char *)ifr.ifr_hwaddr.sa_data;
		printf("The result of SIOCGIFHWADDR is type %d  "
			   "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x.\n",
			   ifr.ifr_hwaddr.sa_family, hwaddr[0], hwaddr[1],
			   hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
	}

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFMETRIC, &ifr) < 0) {
		metric = 0;
	} else
		metric = ifr.ifr_metric;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFMTU, &ifr) < 0)
		mtu = 0;
	else
		mtu = ifr.ifr_mtu;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFDSTADDR, &ifr) < 0) {
		memset(&dstaddr, 0, sizeof(struct sockaddr));
	} else
		dstaddr = ifr.ifr_dstaddr;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFBRDADDR, &ifr) < 0) {
		memset(&broadaddr, 0, sizeof(struct sockaddr));
	} else
		broadaddr = ifr.ifr_broadaddr;

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0) {
		memset(&netmask, 0, sizeof(struct sockaddr));
	} else
		netmask = ifr.ifr_netmask;

	return(0);
}

static void if_print(char *ifname)
{
	char buff[1024];
	struct ifconf ifc;
	struct ifreq *ifr;
	int i;

	if (ifname == (char *)NULL) {
		ifc.ifc_len = sizeof(buff);
		ifc.ifc_buf = buff;
		if (ioctl(skfd, SIOCGIFCONF, &ifc) < 0) {
			fprintf(stderr, "SIOCGIFCONF: %s\n", strerror(errno));
			return;
		}

		ifr = ifc.ifc_req;
		for (i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; ifr++) {
			if (if_getconfig(ifr->ifr_name) < 0) {
				fprintf(stderr, "%s: unknown interface.\n",
						ifr->ifr_name);
				continue;
			}

			if (((mif_flags & IFF_UP) == 0) && !opt_a) continue;
			/*ife_print(&ife);*/
		}
	} else {
		if (if_getconfig(ifname) < 0)
			fprintf(stderr, "%s: unknown interface.\n", ifname);
	}
}


/*
 * Local variables:
 *  version-control: t
 *  kept-new-versions: 5
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  tab-width: 4
 *  compile-command: "gcc -Wall -Wstrict-prototypes -O ifenslave.c -o ifenslave"
 * End:
 */
