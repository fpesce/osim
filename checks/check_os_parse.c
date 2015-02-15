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
#include <apr_pools.h>
#include <apr_strings.h>

#include <pcre.h>
#include "debug.h"
#include "os_parse.h"
/* #include "os_parse.h" */

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

static const char *spy_report1 = "Matières premières sur geonosis [3:414:6] le 09-14 13:32:21 \
Métal:  82709   Cristal:        409079 \
Deutérium:      246397  Energie:        55727 \
Flotte \
Grand transporteur      50      Vaisseau de bataille    30 \
Recycleur       50      Sonde espionnage        100 \
Satellite solaire       1500    Destructeur     20 \
Défense \
Lanceur de missiles     15000   Artillerie laser légère 18374 \
Artillerie laser lourde 1000    Canon de Gauss  200 \
Artillerie à ions       600     Lanceur de plasma       110 \
Petit bouclier  1       Grand bouclier  1 \
Missile Interception    30      Missile Interplanétaire 10 \
Bâtiments \
Mine de métal   27      Mine de cristal 23 \
Synthétiseur de deutérium       22      Centrale électrique solaire     24 \
Centrale électrique de fusion   11      Usine de robots 11 \
Usine de nanites        2       Chantier spatial        12 \
Hangar de métal 8       Hangar de cristal       7 \
Réservoir de deutérium  5       Laboratoire de recherche        12 \
Terraformeur    3       Silo de missiles        5 \
Recherche \
Technologie Espionnage  10      Technologie Ordinateur  10 \
Technologie Armes       12      Technologie Bouclier    12 \
Technologie Protection des vaisseaux spatiaux   11      Technologie Energie     12 \
Technologie Hyperespace 8       Réacteur à combustion   10 \
Réacteur à impulsion    7       Propulsion hyperespace  7 \
Technologie Laser       12      Technologie Ions        10 \
Technologie Plasma      7       Réseau de recherche intergalactique     2 \
Probabilité de destruction de la flotte d'espionnage :0% \
";
static const char *spy_report2 = "Matières premières sur Nabou2 [3:429:7] le 09-14 17:41:11 \
Métal:  998231  Cristal:        407610 \
Deutérium:      166424  Energie:        8130 \
Flotte \
Petit transporteur      100     Grand transporteur      6 \
Chasseur léger  5       Recycleur       1 \
Satellite solaire       116 \
Défense \
Lanceur de missiles     2283    Artillerie laser légère 2414 \
Artillerie laser lourde 1       Canon de Gauss  118 \
Artillerie à ions       1       Lanceur de plasma       30 \
Petit bouclier  1       Grand bouclier  1 \
Bâtiments \
Mine de métal   28      Mine de cristal 24 \
Synthétiseur de deutérium       17      Centrale électrique solaire     23 \
Centrale électrique de fusion   6       Usine de robots 10 \
Usine de nanites        1       Chantier spatial        8 \
Hangar de métal 3       Terraformeur    3 \
Recherche \
Technologie Espionnage  12      Technologie Ordinateur  10 \
Technologie Armes       13      Technologie Bouclier    13 \
Technologie Protection des vaisseaux spatiaux   13      Technologie Energie     12 \
Technologie Hyperespace 8       Réacteur à combustion   12 \
Réacteur à impulsion    9       Propulsion hyperespace  8 \
Technologie Laser       10      Technologie Ions        6 \
Technologie Plasma      7       Réseau de recherche intergalactique     1 \
Technologie Graviton    1 \
Probabilité de destruction de la flotte d'espionnage :0%";

static const char *spy_report3 = "Matières premières sur sangoku [3:442:6] le 09-23 00:27:06 \
Métal:	715179 	Cristal:	560261 \
Deutérium:	686397 	Energie:	4230 \
Flotte \
Chasseur léger	100 	Chasseur lourd	210 \
Croiseur	40 	Vaisseau de bataille	14 \
Recycleur	10 	Sonde espionnage	95 \
Destructeur	15 \
Défense \
Lanceur de missiles	2200	Artillerie laser légère	1500 \
Artillerie laser lourde	950	Canon de Gauss	70 \
Artillerie à ions	130	Lanceur de plasma	60 \
Petit bouclier	1	Grand bouclier	1 \
Missile Interplanétaire	10 \
Bâtiments \
Mine de métal	20	Mine de cristal	19 \
Synthétiseur de deutérium 	18	Centrale électrique solaire	22 \
Centrale électrique de fusion	10	Usine de robots	10 \
Usine de nanites	1	Chantier spatial	12 \
Hangar de métal	9	Hangar de cristal	9 \
Réservoir de deutérium	6	Laboratoire de recherche	12 \
Terraformeur	2	Silo de missiles	4 \
Recherche \
Technologie Espionnage	10	Technologie Ordinateur	10 \
Technologie Armes	11	Technologie Bouclier	10 \
Technologie Protection des vaisseaux spatiaux	10	Technologie Energie	12 \
Technologie Hyperespace	8	Réacteur à combustion	10 \
Réacteur à impulsion	8	Propulsion hyperespace	7 \
Technologie Laser	10	Technologie Ions	9 \
Technologie Plasma	7	Réseau de recherche intergalactique	1 \
Technologie Graviton	1 \
Probabilité de destruction de la flotte d'espionnage :0%";
START_TEST(test_os_parse_make)
{
    char *parsed;

    parsed = os_parse(pool, spy_report2);
    fail_unless(NULL != parsed, "Unable to parse spy_report2.");
    fail_unless(0 ==
		strcmp(parsed,
		       "13,13,13,3:429:7,998231,407610,166424,100,6,5,0,0,0,0,1,0,0,116,0,0,0,2283,2414,1,118,1,30,1,1,0"),
		"Unable to parse spy_report2 \n[%s] different from \n[13,13,13,3:429:7,998231,407610,166424,100,6,5,0,0,0,0,1,0,0,116,0,0,0,2283,2414,1,118,1,30,1,1,0]",
		parsed);
    parsed = os_parse(pool, spy_report1);
    fail_unless(NULL != parsed, "Unable to parse spy_report1.");
    fail_unless(0 ==
		strcmp(parsed,
		       "12,12,11,3:414:6,82709,409079,246397,0,50,0,0,0,30,0,50,100,0,1500,20,0,0,15000,18374,1000,200,600,110,1,1,30"),
		"Unable to parse spy_report1 \n[%s] different from \n[12,12,11,3:414:6,82709,409079,246397,0,50,0,0,0,30,0,50,100,0,1500,20,0,0,15000,18374,1000,200,600,110,1,1,30]",
		parsed);
    printf("%s\n", os_parse(pool, spy_report3));
}
/* *INDENT-OFF* */
END_TEST
/* *INDENT-ON* */

TCase *os_parse_tcase(void)
{
    TCase *tc_core = tcase_create("os_parse_cases");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_os_parse_make);

    return tc_core;
}


