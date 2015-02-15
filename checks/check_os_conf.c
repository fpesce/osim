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

START_TEST(test_os_conf_make)
{
    os_conf_t *conf;

    conf = os_conf_make(pool, CONF_DIR);
    fail_unless(NULL != conf, "Unable to init conf.");
}
/* *INDENT-OFF* */
END_TEST
/* *INDENT-ON* */

START_TEST(test_os_conf_accessor)
{
    os_conf_t *conf;

    conf = os_conf_make(pool, CONF_DIR);
    fail_unless(NULL != conf, "Unable to init conf.");

    fail_unless(EPSILON > ABS(10 - os_conf_get_shield_points(conf, PT)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(5 - os_conf_get_attack_value(conf, PT)), "Error while calling os_conf_get_attack_value");
    fail_unless((4000 == os_conf_get_structure_points(conf, PT)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(25 - os_conf_get_shield_points(conf, GT)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(5 - os_conf_get_attack_value(conf, GT)), "Error while calling os_conf_get_attack_value");
    fail_unless((12000 == os_conf_get_structure_points(conf, GT)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(10 - os_conf_get_shield_points(conf, CLE)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(50 - os_conf_get_attack_value(conf, CLE)), "Error while calling os_conf_get_attack_value");
    fail_unless((4000 == os_conf_get_structure_points(conf, CLE)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(25 - os_conf_get_shield_points(conf, CLO)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(150 - os_conf_get_attack_value(conf, CLO)), "Error while calling os_conf_get_attack_value");
    fail_unless((10000 == os_conf_get_structure_points(conf, CLO)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(50 - os_conf_get_shield_points(conf, CR)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(400 - os_conf_get_attack_value(conf, CR)), "Error while calling os_conf_get_attack_value");
    fail_unless((27000 == os_conf_get_structure_points(conf, CR)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(200 - os_conf_get_shield_points(conf, VB)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(1000 - os_conf_get_attack_value(conf, VB)), "Error while calling os_conf_get_attack_value");
    fail_unless((60000 == os_conf_get_structure_points(conf, VB)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(100 - os_conf_get_shield_points(conf, VC)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(50 - os_conf_get_attack_value(conf, VC)), "Error while calling os_conf_get_attack_value");
    fail_unless((30000 == os_conf_get_structure_points(conf, VC)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(10 - os_conf_get_shield_points(conf, REC)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(1 - os_conf_get_attack_value(conf, REC)), "Error while calling os_conf_get_attack_value");
    fail_unless((16000 == os_conf_get_structure_points(conf, REC)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(0.01 - os_conf_get_shield_points(conf, SE)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(0.01 - os_conf_get_attack_value(conf, SE)), "Error while calling os_conf_get_attack_value");
    fail_unless((4000 == os_conf_get_structure_points(conf, SE)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(500 - os_conf_get_shield_points(conf, BB)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(1000 - os_conf_get_attack_value(conf, BB)), "Error while calling os_conf_get_attack_value");
    fail_unless((75000 == os_conf_get_structure_points(conf, BB)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(1 - os_conf_get_shield_points(conf, SAT)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(1 - os_conf_get_attack_value(conf, SAT)), "Error while calling os_conf_get_attack_value");
    fail_unless((2000 == os_conf_get_structure_points(conf, SAT)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(500 - os_conf_get_shield_points(conf, DEST)), "Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(2000 - os_conf_get_attack_value(conf, DEST)), "Error while calling os_conf_get_attack_value");
    fail_unless((110000 == os_conf_get_structure_points(conf, DEST)), "Error while calling os_conf_get_structure_points");

    fail_unless(EPSILON > ABS(50000.0 - os_conf_get_shield_points(conf, EDLM)),
		"Error while calling os_conf_get_shield_points");
    fail_unless(EPSILON > ABS(200000.0 - os_conf_get_attack_value(conf, EDLM)),
		"Error while calling os_conf_get_attack_value");
    fail_unless((9000000 == os_conf_get_structure_points(conf, EDLM)), "Error while calling os_conf_get_structure_points");
}
/* *INDENT-OFF* */
END_TEST
/* *INDENT-ON* */

TCase *os_conf_tcase(void)
{
    TCase *tc_core = tcase_create("os_conf_cases");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_os_conf_make);
    tcase_add_test(tc_core, test_os_conf_accessor);

    return tc_core;
}


