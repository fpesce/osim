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
#include <stdlib.h>
#include <apr_pools.h>
#include <stdio.h>

TCase *os_conf_tcase(void);
TCase *os_fleet_tcase(void);
TCase *os_parse_tcase(void);

Suite *osim_suite(void)
{
    Suite *s = suite_create("osim_suite");
    suite_add_tcase(s, os_conf_tcase());
    suite_add_tcase(s, os_fleet_tcase());
    suite_add_tcase(s, os_parse_tcase());
    return s;
}

int main(void)
{
    char buf[256];
    int nf;
    apr_status_t status;
    Suite *s;
    SRunner *sr;

    status = apr_initialize();
    if (APR_SUCCESS != status) {
	apr_strerror(status, buf, 200);
	printf("error: %s\n", buf);
    }

    s = osim_suite();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_set_xml(sr, "./checks_log.xml");

    srunner_run_all(sr, CK_NORMAL);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);

    apr_terminate();
    return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


