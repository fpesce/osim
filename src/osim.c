/*
 * Copyright (C) 2007 François Pesce : francois.pesce (at) gmail (dot) com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <time.h>

#include <apr_getopt.h>
#include <apr_file_io.h>
#include <apr_pools.h>
#include <apr_strings.h>

#include "debug.h"
#include "os_conf.h"
#include "os_fleet.h"
#include "os_parse.h"

static void usage(const char *argv0)
{
    fprintf(stderr,
	    "Usage is: %s -a csv_attacker -d [stdin | csv_defender] [-g a|d [-m s|r|d|f [-i] [-l] [-y]] [-o h|p|x] [-t inactivity_timeout] [-f flight_timeout] [-w wave_timeout] [-x fixed_timeout]] [-c confdir] [-n nb_simu] [-p nb_cpu]\n",
	    argv0);
    fprintf(stderr, "\tcsv_attacker is of the form:\n");
    fprintf(stderr,
	    "\t\tdamage,shield,life,combustion,impulsion,hyperespace,coord,pt,gt,cle,clo,cr,vb,vc,rec,se,bb,0,dest,edlm,trac,mip_vs_ldm,mip_vs_alle,mip_vs_allo,mip_vs_cg,mip_vs_aai,mip_vs_lp,mip_vs_pb,mip_vs_gb\n");
    fprintf(stderr, "\tcsv_defender is of the form:\n");
    fprintf(stderr,
	    "\t\tdamage,shield,life,coord,metal,cristal,deut,pt,gt,cle,clo,cr,vb,vc,rec,se,bb,sat,dest,edlm,trac,ldm,alle,allo,cg,aai,lp,pb,gb,mit\n");
    fprintf(stderr,
	    "\t\tif stdin is precised, you can cut/paste a report with the string \"destruction de la flotte d'espionnage\" at the end.\n");
    fprintf(stderr, "\tconfdir is optionnal to redefine ship values:\n");
    fprintf(stderr, "\t\tif not precised, hardcoded value from 0.76 ogame will be used.\n");
    fprintf(stderr, "\tg indicate to turn on guess mode, use it with one fleet defined (attacker or defender).\n");
    fprintf(stderr, "\t or both fleet defined and one option to indicate which one is in guess\n");
    fprintf(stderr, "\tmode, these fleet will have its already buyed price removed from fitness.\n");
    fprintf(stderr, "\tThe guess mode will compute, using genetic algorithm the best fleet to counter the one\n");
    fprintf(stderr, "\tyou have defined. This option is memory and CPU intensive and is EXPERIMENTAL.\n");
    fprintf(stderr, "\t\tdefault is off.\n");
    fprintf(stderr, "\t\tThe fleet you want to create must be technologically defined: damage,shield,life.\n");
    fprintf(stderr, "\tm indicate the mask of ship to apply (default is r):\n");
    fprintf(stderr, "\t\tr: remove PT,GT,VC,REC,SE,SAT,EDLM from attacking fleet (let EDLM for defending fleet).\n");
    fprintf(stderr,
	    "\t\ts: remove PT,GT,VC,REC,SE,SAT,EDLM and missile from attacking fleet (activate mode: no-loss and no-invest. i.e. SCRIPT MODE).\n");
    fprintf(stderr, "\t\td: defense (to simulate best defense), big ships (VB/BB/DEST/EDLM and ALL DEF) allowed\n");
    fprintf(stderr, "\t\tn: no missile to avoid missile from attacker.\n");
    fprintf(stderr, "\t\tf: full, all ship allowed\n");
    fprintf(stderr,
	    "\ti indicate that no investment must be done by simulation : i.e. generated fleet will be a subset of your fleet (default off).\n");
    fprintf(stderr, "\tl indicate that no loss in your fleet must be tolerated by simulation (default off).\n");
    fprintf(stderr, "\ty indicate that no recycling will be taken in account by simulation (default off).\n");
    fprintf(stderr, "\to indicate the type of output of osim (default is h for human):\n");
    fprintf(stderr, "\t\th: csv more human friendly (will be enhanced).\n");
    fprintf(stderr, "\t\tp: csv for perl script.\n");
    fprintf(stderr, "\t\tx: html for php script.\n");
    fprintf(stderr,
	    "\tinactivity_timeout is optionnal to set the maximum time of inactivity (delay between 2 best solution) in genetic algorithm.\n");
    fprintf(stderr, "\t\tdefault is infinite, except for script mask where it is 30 seconds.\n");
    fprintf(stderr, "\tfixed_timeout is optionnal to set the maximum time of processing genetic algorithm.\n");
    fprintf(stderr, "\t\tdefault is infinite, except for script mask where it is 120 seconds.\n");
    fprintf(stderr, "\tflight_timeout is optionnal to set the maximum time of a flight to the dest [VERY EXPERIMENTAL].\n");
    fprintf(stderr, "\twave_timeout is optionnal to set a penalty to slow fleet [VERY EXPERIMENTAL].\n");
    fprintf(stderr, "\t\tdefault is infinite.\n");
    fprintf(stderr, "\tnb_simu is optionnal to set the number of simulations run.\n");
    fprintf(stderr, "\t\tdefault is 100.\n");
    fprintf(stderr, "\tnb_cpu is optionnal to set the number of threads to run in genetic algo default is 1.\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr,
	    "\t%s -a \"17,17,17,15,14,11,[3:432:9],4300,0,45000,15000,15000,10000,0,5500,0,7500,0,6700,0,0\" -d \"15,13,15,3:482:7,20615000,4934510,3363090,20,450,10000,1000,300,892,0,660,13,1000,0,1000,2,55,0,0,0,0,0,0,0,0,0\"\n",
	    argv0);

    fprintf(stderr,
	    "\t%s -a \"17,17,17,15,14,11,[3:432:9],4300,0,45000,15000,15000,10000,0,5500,0,7500,0,6700,0,0\" -d \"11,11,11\" -g d\n",
	    argv0);
    fprintf(stderr,
	    "\t%s -a \"15,16,15,15,13,10\" -d \"11,11,11,[3:412:7],200000,100000,90000,21,44,4,3,7,4,0,0,80,0,119,28,0,0,8063,20,76,25,95,51,1,1,40\" -g a -m r\n",
	    argv0);
    fprintf(stderr,
	    "\t%s -a \"17,17,17,15,14,11,[3:432:9],4300,0,45000,15000,15000,10000,0,5500,0,7500,0,6700,0,0\" -d \"11,10,10,[3:442:6],1598072,679034,656258,0,22,0,0,0,0,0,6,70,0,0,0,1,0,2900,1800,1400,70,200,77,1,1,40\" -g a\n",
	    argv0);
    fprintf(stderr, "Example of MIP launch:\n");
    fprintf(stderr,
	    "\t%s -a \"15,16,15,15,13,10,[3:432:9],0,0,0,0,0,0,0,0,0,0,0,0,0,0,100,100,100,100,100,100,1,1\" -d \"11,11,11,[3:412:7],200000,100000,90000,21,44,4,3,7,4,0,0,80,0,119,28,0,0,8063,20,76,25,95,51,1,1,40\"\n",
	    argv0);
}

/*
 * for attacker:
 * - damage,shield,life,combustion,impulsion,hyperespace,coord,pt,gt,cle,clo,cr,vb,vc,rec,se,bb,0,dest,edlm,trac,mip_vs_ldm,mip_vs_alle,mip_vs_allo,mip_vs_cg,mip_vs_aai,mip_vs_lp,mip_vs_pb,mip_vs_gb
 * for defender:
 * - damage,shield,life,combustion,impulsion,hyperespace,coord,metal,cristal,deut,pt,gt,cle,clo,cr,vb,vc,rec,se,bb,sat,dest,edlm,trac,ldm,alle,allo,cg,aai,lp,pb,gb,mit
 */
int main(int argc, const char **argv)
{
    static const apr_getopt_option_t opt_option[] = {
	/* long-option, short-option, has-arg flag, description */
	{"attacker", 'a', TRUE, "attacker fleet"},
	{"defender", 'd', TRUE, "defender army"},
	{"confdir", 'c', TRUE, "Configuration directory"},
	{"flight-time", 'f', TRUE, "Maximum flight-time for guess-mode"},
	{"guess", 'g', TRUE, "Guess mode (Find the cheapest fleet to counter this"},
	{"help", 'h', FALSE, "Help"},
	{"no-invest", 'i', FALSE, "Don't allow investment in guess mode (guess WILL be a subset of your fleet)"},
	{"no-loss", 'l', FALSE, "Don't allow loss of ship in guess mode"},
	{"mask", 'm', TRUE, "mask of ships to apply [s|r|d|f|n]"},
	{"nbsim", 'n', TRUE, "Number of simulations"},
	{"output", 'o', TRUE, "type of output html/perl/human [h|p|x]"},
	{"processor", 'p', TRUE, "Number of thread (idealy,the number of CPUs of the machine)"},
	{"timeout", 't', TRUE, "Inactivity timeout for guess mode [s|r|d|f|n]"},
	{"r-depracted", 'r', FALSE, "Deprecated use -m r instead"},
	{"wave-time", 'w', TRUE, "A fleet that overflow this time, will be penalized."},
	{"fiXed-timeout", 'x', TRUE, "Fixed time limit for genetic algorithm."},
	{"no-recYcling", 'y', FALSE, "Don't include recycling in rentability"},
	{NULL, 0, 0, NULL},	/* end (a.k.a. sentinel) */
    };
    char errbuf[128];
    char buffer[1024];
    const char *optarg;
    char *conffile = NULL, *defstdin = NULL, *defline;
    unsigned long nbsim = 100UL, nbcpu = 1, flight_time = 0UL, wave_time = 0UL, fixed_timeout = 0UL, timeout = 0UL;
    apr_size_t readbytes, writtenbytes;
    apr_getopt_t *os;
    apr_file_t *f_stdin;
    apr_pool_t *pool;
    os_conf_t *conf;
    os_fleet_t *attacker, *defender;
    int guessmode = 0, defender_from_stdin = 0;
    int optch;
    enum genetic_algorithm_mask mask = NORMAL;
    apr_status_t status;
    unsigned char mode;

    if (APR_SUCCESS != (status = apr_initialize())) {
	DEBUG_ERR("error calling apr_initialize: %s", apr_strerror(status, errbuf, 128));
	return status;
    }

    srand((unsigned int) time(NULL));

    if (1 == argc) {
	usage(argv[0]);
	return -1;
    }

    if (APR_SUCCESS != (status = apr_pool_create(&pool, NULL))) {
	DEBUG_ERR("error calling apr_pool_create: %s", apr_strerror(status, errbuf, 128));
	return status;
    }

    if (APR_SUCCESS != (status = apr_getopt_init(&os, pool, argc, argv))) {
	char errbuf[128];
	DEBUG_ERR("error calling apr_getopt_init: %s", apr_strerror(status, errbuf, 128));
	return status;
    }

    attacker = os_fleet_make(pool, ATK_FLT);
    defender = os_fleet_make(pool, DEF_FLT);
    mode = OS_MODE_HUMAN;

    while (APR_SUCCESS == (status = apr_getopt_long(os, opt_option, &optch, &optarg))) {
	switch (optch) {
	case 'a':
	    if (APR_SUCCESS != (os_fleet_set_conf(attacker, optarg))) {
		DEBUG_ERR("error calling os_fleet_set_conf");
		return -1;
	    }
	    break;
	case 'd':
	    if (!strcmp("stdin", optarg)) {
		defender_from_stdin = 1;
	    }
	    else {
		if (APR_SUCCESS != (os_fleet_set_conf(defender, optarg))) {
		    DEBUG_ERR("error calling os_fleet_set_conf");
		    return -1;
		}
	    }
	    break;
	case 'c':
	    conffile = apr_pstrdup(pool, optarg);
	    break;
	case 'h':
	    usage(argv[0]);
	    return -1;
	    break;
	case 'i':
	    mode |= OS_MODE_NO_INVEST;
	    break;
	case 'l':
	    mode |= OS_MODE_NO_LOSS;
	    break;
	case 'y':
	    mode |= OS_MODE_NO_RECYCLING;
	    break;
	case 'm':
	    switch (*optarg) {
	    case 's':
		mask = SCRIPT;
		break;
	    case 'r':
		mask = NORMAL;
		break;
	    case 'd':
		mask = DEF;
		break;
	    case 'f':
		mask = FULL;
		break;
	    case 'n':
		mask = NO_MISSILE;
		break;
	    default:
		usage(argv[0]);
		return -1;
	    }
	    break;
	case 'p':
	    nbcpu = strtoul(optarg, NULL, 10);
	    if (ULONG_MAX == nbcpu) {
		DEBUG_ERR("can't parse %s for cpu", optarg);
		return -1;
	    }
	    break;
	case 'n':
	    nbsim = strtoul(optarg, NULL, 10);
	    if (ULONG_MAX == nbsim) {
		DEBUG_ERR("can't parse %s for nbsim", optarg);
		return -1;
	    }
	    break;
	case 'o':
	    switch (*optarg) {
	    case 'h':
		mode |= OS_MODE_HUMAN;
		break;
	    case 'p':
		mode &= ~OS_MODE_HUMAN;
		mode |= OS_MODE_PERL;
		break;
	    case 'x':
		mode &= ~OS_MODE_HUMAN;
		mode |= OS_MODE_HTML;
		break;
	    default:
		usage(argv[0]);
		return -1;
	    }
	    break;
	case 'g':
	    if ('a' == *optarg)
		os_fleet_to_guess(attacker);
	    else if ('d' == *optarg)
		os_fleet_to_guess(defender);
	    guessmode = 1;
	    break;
	case 't':
	    timeout = strtoul(optarg, NULL, 10);
	    if (ULONG_MAX == timeout) {
		DEBUG_ERR("can't parse %s for timeout", optarg);
		return -1;
	    }
	    break;
	case 'f':
	    flight_time = strtoul(optarg, NULL, 10);
	    if (ULONG_MAX == flight_time) {
		DEBUG_ERR("can't parse %s for flight_time", optarg);
		return -1;
	    }
	    break;
	case 'w':
	    wave_time = strtoul(optarg, NULL, 10);
	    if (ULONG_MAX == wave_time) {
		DEBUG_ERR("can't parse %s for wave_time", optarg);
		return -1;
	    }
	    break;
	case 'x':
	    fixed_timeout = strtoul(optarg, NULL, 10);
	    if (ULONG_MAX == fixed_timeout) {
		DEBUG_ERR("can't parse %s for fixed_timeout", optarg);
		return -1;
	    }
	    break;
	case 'r':
	    DEBUG_ERR("-r is deprecated, use -m instead\n");
	    usage(argv[0]);
	    return -1;
	}
    }

    if (SCRIPT == mask) {
	if (0UL == timeout)
	    timeout = 30;

	if (0UL == fixed_timeout)
	    fixed_timeout = 120;
    }

    if (defender_from_stdin) {
	if (APR_SUCCESS != apr_file_open_stdin(&f_stdin, pool)) {
	    fprintf(stderr, "Unable to open stdin\n");
	    return -1;
	}

	writtenbytes = 0;
	for (;;) {
	    char *tmp;

	    readbytes = sizeof(buffer);
	    memset(buffer, '\0', sizeof(buffer));
	    if (APR_SUCCESS != apr_file_read(f_stdin, buffer, &readbytes)) {
		DEBUG_ERR("error calling apr_file_read");
		return -1;
	    }
	    tmp = apr_pcalloc(pool, writtenbytes + readbytes + 1);
	    memcpy(tmp, defstdin, writtenbytes);
	    memcpy(tmp + writtenbytes, buffer, readbytes);
	    writtenbytes += readbytes;
	    tmp[writtenbytes] = '\0';
	    defstdin = tmp;

	    if (strstr(defstdin, "destruction de la flotte d'espionnage"))
		break;
	}

	if (NULL == (defline = os_parse(pool, defstdin))) {
	    DEBUG_ERR("Can't parse stdin");
	    return -1;
	}

	DEBUG_DBG("parsed line is [%s]", defline);

	if (APR_SUCCESS != (os_fleet_set_conf(defender, defline))) {
	    DEBUG_ERR("error calling os_fleet_set_conf");
	    return -1;
	}
    }
    if (NULL == (conf = os_conf_make(pool, conffile))) {
	DEBUG_ERR("error calling os_conf_make");
	return -1;
    }

    if (APR_SUCCESS != os_fleet_parse(attacker, conf)) {
	DEBUG_ERR("error calling os_fleet_parse");
	return -1;
    }

    if (APR_SUCCESS != os_fleet_parse(defender, conf)) {
	DEBUG_ERR("error calling os_fleet_parse");
	return -1;
    }

    if (1 == guessmode) {
	os_fleet_find_cheapest_winner(attacker, defender, conf, mask, timeout, fixed_timeout, flight_time, wave_time, mode,
				      nbcpu);
    }
    else {
	os_fleet_battle(attacker, defender, nbsim, conf, mode);
    }

    apr_terminate();

    return 0;
}


