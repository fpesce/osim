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

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <apr_file_io.h>

#include "os_conf.h"
#include "os_fleet.h"

apr_pool_t *pool;

static void setup(void)
{
    apr_status_t rs;

    rs = apr_pool_create(&pool, NULL);
    if (rs != APR_SUCCESS) {
	printf("Error creating pool\n");
	exit(1);
    }
}

static void teardown(void)
{
    apr_pool_destroy(pool);
}

START_TEST(test_os_fleet_make)
{
    os_fleet_t *fleet;

    fleet = os_fleet_make(pool, ATK_FLT);
    fail_unless(NULL != fleet, "Unable to make fleet.");
    fleet = os_fleet_make(pool, DEF_FLT);
    fail_unless(NULL != fleet, "Unable to make fleet.");
}
/* *INDENT-OFF* */
END_TEST
/* *INDENT-ON* */

START_TEST(test_os_fleet_set_conf)
{
    os_fleet_t *fleet;
    apr_status_t status;

    fleet = os_fleet_make(pool, ATK_FLT);
    fail_unless(NULL != fleet, "Unable to make fleet.");
    status = os_fleet_set_conf(fleet, "15,16,15,15,13,10,[3:432:9],1400,700,14000,4900,2200,2400,0,1400,100,1050,0,750,13");
    fail_unless(APR_SUCCESS == status, "Unable to configure fleet.");
    fleet = os_fleet_make(pool, DEF_FLT);
    fail_unless(NULL != fleet, "Unable to make fleet.");
    status =
	os_fleet_set_conf(fleet,
			  "15,16,15,[3:432:9],200000,100000,90000,1100,550,11000,3850,1100,1100,1,1100,10000,2,1969,550,11,66000,33000,16500,2750,2200,1650,1,1,0");
    fail_unless(APR_SUCCESS == status, "Unable to configure fleet.");

    /* Conf with only technos for guess mode */
    fleet = os_fleet_make(pool, ATK_FLT);
    fail_unless(NULL != fleet, "Unable to make fleet.");
    status = os_fleet_set_conf(fleet, "15,15,14,15,13,10");
    fail_unless(APR_SUCCESS == status, "Unable to configure fleet.");
}
/* *INDENT-OFF* */
END_TEST
/* *INDENT-ON* */

START_TEST(test_os_fleet_parse)
{
    os_fleet_t *fleet;
    os_conf_t *conf;
    apr_status_t status;

    fleet = os_fleet_make(pool, DEF_FLT);
    fail_unless(NULL != fleet, "Unable to make fleet.");
    conf = os_conf_make(pool, CONF_DIR);
    fail_unless(NULL != conf, "Unable to load conf.");

    status =
	os_fleet_set_conf(fleet,
			  "15,15,14,[3:432:9],200000,100000,90000,1100,550,11000,3850,1100,1100,1,1100,10000,2,1969,550,11,66000,33000,16500,2750,2200,1650,1,1,0");
    fail_unless(APR_SUCCESS == status, "Unable to configure fleet.");

    status = os_fleet_parse(fleet, conf);
    fail_unless(APR_SUCCESS == status, "Unable to parse fleet configuration.");
}
/* *INDENT-OFF* */
END_TEST
/* *INDENT-ON* */

START_TEST(test_os_fleet_battle)
{
    os_fleet_t *attacker, *defender;
    os_conf_t *conf;
    apr_status_t status;

    attacker = os_fleet_make(pool, ATK_FLT);
    fail_unless(NULL != attacker, "Unable to make fleet.");
    conf = os_conf_make(pool, NULL);
    fail_unless(NULL != conf, "Unable to load conf.");
    status = os_fleet_set_conf(attacker, "15,15,14,15,13,10,[3:432:9],0,0,11000,3850,1100,1100,0,0,0,0,0,550,0");
    fail_unless(APR_SUCCESS == status, "Unable to configure fleet.");
    status = os_fleet_parse(attacker, conf);
    fail_unless(APR_SUCCESS == status, "Unable to parse fleet configuration.");

    defender = os_fleet_make(pool, DEF_FLT);
    fail_unless(NULL != defender, "Unable to make fleet.");
    status =
	os_fleet_set_conf(defender,
			  "11,11,11,[3:412:7],200000,100000,90000,21,44,4,3,7,4,0,0,80,0,119,28,0,8063,20,76,25,95,51,1,1");
    fail_unless(APR_SUCCESS == status, "Unable to configure fleet.");
    status = os_fleet_parse(defender, conf);
    fail_unless(APR_SUCCESS == status, "Unable to parse fleet configuration.");

    os_fleet_battle(attacker, defender, 100UL, conf, 0x01);
}
/* *INDENT-OFF* */
END_TEST
/* *INDENT-ON* */

START_TEST(test_os_fleet_distance)
{
    fail_unless(4695UL == os_fleet_distance("3:432:9", "3:411:12"), "Bad distance to syst.");
    fail_unless(1015UL == os_fleet_distance("3:432:9", "3:432:12"), "Bad distance to planet.");
    fail_unless(20000UL == os_fleet_distance("3:432:9", "2:432:12"), "Bad distance to galaxy.");
}
/* *INDENT-OFF* */
END_TEST
/* *INDENT-ON* */

START_TEST(test_os_fleet_consumption)
{
    os_fleet_t *attacker;
    os_conf_t *conf;
    apr_status_t status;
    unsigned int flight_time;

    attacker = os_fleet_make(pool, ATK_FLT);
    fail_unless(NULL != attacker, "Unable to make fleet.");
    conf = os_conf_make(pool, NULL);
    fail_unless(NULL != conf, "Unable to load conf.");
    status = os_fleet_set_conf(attacker, "15,16,15,15,13,10,[3:432:9],0,0,14000,4900,2200,2400,0,0,0,1050,0,750,0");
    fail_unless(APR_SUCCESS == status, "Unable to configure fleet.");
    status = os_fleet_parse(attacker, conf);
    fail_unless(APR_SUCCESS == status, "Unable to parse fleet configuration.");

    fprintf(stdout, "%u\n", os_fleet_consumption(attacker, 1015UL, &flight_time));
    fprintf(stdout, "\n%u\n", os_fleet_consumption(attacker, 4695UL, &flight_time));
    fprintf(stdout, "%u\n", os_fleet_consumption(attacker, 20000UL, &flight_time));
}
/* *INDENT-OFF* */
END_TEST
/* *INDENT-ON* */

TCase *os_fleet_tcase(void)
{
    TCase *tc_core = tcase_create("os_fleet_cases");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_os_fleet_make);
    tcase_add_test(tc_core, test_os_fleet_set_conf);
    tcase_add_test(tc_core, test_os_fleet_parse);
    tcase_add_test(tc_core, test_os_fleet_battle);
    tcase_add_test(tc_core, test_os_fleet_distance);
    tcase_add_test(tc_core, test_os_fleet_consumption);

    return tc_core;
}


