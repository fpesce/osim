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

#ifndef OS_FLEET_H
#define OS_FLEET_H

#include <apr_pools.h>

typedef struct os_fleet_t os_fleet_t;

#define MAX_ROUND_NUMBER '\6'

enum Fleet_enum
{
    ATK_FLT,
    DEF_FLT
};

os_fleet_t *os_fleet_make(apr_pool_t *pool, enum Fleet_enum type);

void os_fleet_to_guess(os_fleet_t *fleet);

apr_status_t os_fleet_set_conf(os_fleet_t *fleet, const char *conf);

apr_status_t os_fleet_parse(os_fleet_t *fleet, const os_conf_t *conf);

apr_status_t os_fleet_precalc_shoot_table(apr_pool_t *pool, os_fleet_t *defender, const os_fleet_t *attacker);

void os_fleet_battle(os_fleet_t *attacker, os_fleet_t *defender, unsigned int nb_simu, const os_conf_t *conf,
		     unsigned char mode);

/* output formated for a human */
#define OS_MODE_HUMAN 0x01
/* output formated for perl script */
#define OS_MODE_PERL 0x02
/* output formated in HTML */
#define OS_MODE_HTML 0x04
/* computation don't include loss of your own ship */
#define OS_MODE_NO_LOSS 0x08
/* computation don't include recycling of CDR */
#define OS_MODE_NO_RECYCLING 0x10
/* computation don't include investment for the fleet (result will be a subset of your own fleet) */
#define OS_MODE_NO_INVEST 0x20

enum genetic_algorithm_mask
{
    FULL = 0x0,			/* All will be tried except satellites */
    DEF,			/* Big ships VB/BB/DEST/EDLM and all DEF allowed. */
    SCRIPT,			/* transporters allowed, no missile, or buffer fleet (VC,REC,SE,SAT,EDLM) */
    NO_MISSILE,			/* explicit transporters allowed, no missile */
    NORMAL			/* no buffer fleet (PT,GT,VC,REC,SE,SAT,EDLM) */
};

void os_fleet_find_cheapest_winner(os_fleet_t *attacker, os_fleet_t *defender, const os_conf_t *conf,
				   enum genetic_algorithm_mask mask, unsigned int inactivity_timeout,
				   unsigned int fixed_timeout, unsigned int flight_time, unsigned int wave_time,
				   unsigned char mode, unsigned int nb_cpu);

unsigned int os_fleet_distance(const char *fleet1, const char *fleet2);

unsigned int os_fleet_consumption(const os_fleet_t *attacker, unsigned int distance, unsigned int *flight_time);
#endif /* OS_FLEET_H */


