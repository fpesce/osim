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

#define _ISOC99_SOURCE
#include <math.h>
#include <stdlib.h>

#include <apr_file_io.h>
#include <apr_hash.h>
#include <apr_strings.h>

#include "os_conf.h"
#include "debug.h"

struct os_item_t
{
    char *name;
    const char *shortname;
    float shield_points;
    float attack_value;
    unsigned int structure_points;
    unsigned int capacity;
    unsigned int speed;
    unsigned int metal;
    unsigned int cristal;
    unsigned int deut;
    unsigned short fuel_consom;
};

struct os_conf_t
{
    struct os_item_t item[ITEM_END];
    short unsigned int rapid_fire[ITEM_END][ITEM_END];
};

struct item_word_t
{
    const char *str;
    enum Item_enum item;
};

typedef struct item_word_t item_word_t;

static const struct os_item_t item_const_def[] = {
    /* name, shortname, shield_points, attack_value, structure_points, capacity, speed, metal, cristal, deut, fuel_consom. */
    {"Petit transporteur", "pt", 10.0f, 5.0f, 4000UL, 5000UL, 10000UL, 2000UL, 2000UL, 0UL, 20},
    {"Grand transporteur", "gt", 25.0f, 5.0f, 12000UL, 25000UL, 7500UL, 6000UL, 6000UL, 0UL, 50},
    {"Chasseur léger", "cle", 10.0f, 50.0f, 4000UL, 50UL, 12500UL, 3000UL, 1000UL, 0UL, 20},
    {"Chasseur lourd", "clo", 25.0f, 150.0f, 10000UL, 100UL, 10000UL, 6000UL, 4000UL, 0UL, 75},
    {"Croiseur", "cr", 50.0f, 400.0f, 27000UL, 800UL, 15000UL, 20000UL, 7000UL, 2000UL, 300},
    {"Vaisseau de bataille", "vb", 200.0f, 1000.0f, 60000UL, 1500UL, 10000UL, 45000UL, 15000UL, 0UL, 500},
    {"Vaisseau de colonisation", "vc", 100.0f, 50.0f, 30000UL, 7500UL, 2500UL, 10000UL, 20000UL, 10000UL, 1000},
    {"Recycleur", "rec", 10.0f, 1.0f, 16000UL, 20000UL, 2000UL, 10000UL, 6000UL, 2000UL, 300},
    {"Sonde espionnage", "se", 0.0f, 0.0f, 4000UL, 5UL, 100000000UL, 0UL, 1000UL, 0UL, 1},
    {"Bombardier", "bb", 500.0f, 1000.0f, 75000UL, 500UL, 5000UL, 50000UL, 25000UL, 15000UL, 1000},
    {"Satellite solaire", "sat", 1.0f, 1.0f, 2000UL, 0UL, 0UL, 0UL, 2000UL, 500UL, 0},
    {"Destructeur", "dest", 500.0f, 2000.0f, 110000UL, 2000UL, 5000UL, 60000UL, 50000UL, 15000UL, 1000},
    {"Étoile de la mort", "edlm", 50000.0f, 200000.0f, 9000000UL, 1000000UL, 100UL, 5000000UL, 4000000UL, 1000000UL, 1},
    {"Traqueur", "trac", 400.0f, 700.0f, 70000UL, 750UL, 10000UL, 30000UL, 40000UL, 15000UL, 250},
    {"Lanceur de missiles", "lm", 20.0f, 80.0f, 2000UL, 0UL, 0UL, 2000UL, 0UL, 0UL, 0},
    {"Artillerie laser légère", "alle", 25.0f, 100.0f, 2000UL, 0UL, 0UL, 1500UL, 500UL, 0UL, 0},
    {"Artillerie laser lourde", "allo", 100.0f, 250.0f, 8000UL, 0UL, 0UL, 6000UL, 2000UL, 0UL, 0},
    {"Canon de Gauss", "cg", 200.0f, 1100.0f, 35000UL, 0UL, 0UL, 20000UL, 15000UL, 2000UL, 0},
    {"Artillerie à ions", "aai", 500.0f, 150.0f, 8000UL, 0UL, 0UL, 2000UL, 6000UL, 0UL, 0},
    {"Lanceur de plasma", "lp", 300.0f, 3000.0f, 100000UL, 0UL, 0UL, 50000UL, 50000UL, 30000UL, 0},
    {"Petit bouclier", "pb", 2000.0f, 1.0f, 20000UL, 0UL, 0UL, 10000UL, 10000UL, 0UL, 0},
    {"Grand bouclier", "gb", 10000.0f, 1.0f, 100000UL, 0UL, 0UL, 50000UL, 50000UL, 0UL, 0},
    {NULL, NULL, 0.0f, 0.0f, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0}
};

static const short unsigned int rapid_fire_const[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 0, 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 20, 20, 10, 0, 10, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 2, 0, 10, 0, 0, 0, 0, 0, 0,
    250, 250, 200, 100, 33, 30, 250, 250, 1250, 25, 1250, 5, 0, 15, 200, 200, 100, 50, 100, 0, 0, 0,
    3, 3, 0, 4, 4, 7, 0, 0, 5, 0, 5, 0, 0, 2, 0, 10, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const item_word_t item_thesaurus[] = {
    {"PT", PT},
    {"GT", GT},
    {"CLE", CLE},
    {"CLO", CLO},
    {"CR", CR},
    {"VB", VB},
    {"VC", VC},
    {"REC", REC},
    {"SE", SE},
    {"BB", BB},
    {"SAT", SAT},
    {"DEST", DEST},
    {"EDLM", EDLM},
    {"TRAC", TRAC},
    {"LM", LM},
    {"ALLE", ALLE},
    {"ALLO", ALLO},
    {"CG", CG},
    {"AAI", AAI},
    {"LP", LP},
    {"PB", PB},
    {"GB", GB},
    {NULL, ITEM_END}
};

static unsigned int os_conf_get_ulong_from_line(const char *line)
{
    const char *ptr;
    unsigned long result;

    if (NULL == (ptr = strchr(line, '='))) {
	DEBUG_ERR("Invalid line without '=': %s", line);
	return UINT_MAX;
    }

    result = strtoul(&ptr[1], NULL, 10);

    if (ULONG_MAX == result)
	result = UINT_MAX;

    if ((0 == result) && '0' != ptr[1]) {
	DEBUG_ERR("can't get number from: %s", line);
	return UINT_MAX;
    }

    return result;
}

static char *os_conf_get_unquoted_str_from_line(apr_pool_t *pool, const char *line)
{
    const char *ptr;
    char *result;
    apr_size_t len;

    if ((NULL == (ptr = strchr(line, '='))) || '"' != ptr[1]) {
	DEBUG_ERR("Invalid line without '=' or '\"': %s", line);
	return NULL;
    }

    result = apr_pstrdup(pool, &ptr[2]);
    len = strlen(result);

    if ('"' != result[len - 2]) {
	DEBUG_ERR("missing final '\"': %s", result);
	return NULL;
    }

    result[len - 2] = '\0';

    return result;
}

static float os_conf_get_float_from_line(const char *line)
{
    const char *ptr;

    if (NULL == (ptr = strchr(line, '='))) {
	DEBUG_ERR("Invalid line without '=': %s", line);
	return HUGE_VALF;
    }

    return strtof(&ptr[1], NULL);
}

static apr_status_t os_conf_parse_item_file(apr_pool_t *pool, const char *filename, os_conf_t *conf, int *i)
{
    char line[1024];
    char errbuf[128];
    apr_file_t *f;
    apr_status_t status;

    if (APR_SUCCESS != (status = apr_file_open(&f, filename, APR_READ | APR_BUFFERED, APR_OS_DEFAULT, pool))) {
	DEBUG_ERR("error calling apr_file_open: %s", apr_strerror(status, errbuf, 128));
	return status;
    }

    while (APR_SUCCESS == (status = apr_file_gets(line, sizeof(line), f))) {
	if (line[0] == '[') {
	    while ((APR_SUCCESS == apr_file_gets(line, sizeof(line), f))) {
		if ('p' == line[0] && 's' == line[1]) {
		    if (UINT_MAX == ((conf->item)[*i].structure_points = os_conf_get_ulong_from_line(line))) {
			DEBUG_ERR("error calling os_conf_get_ulong_from_line");
			apr_file_close(f);
			return APR_EINVAL;
		    }
		}
		else if ('p' == line[0] && 'b' == line[1]) {
		    if (HUGE_VALF == ((conf->item)[*i].shield_points = os_conf_get_float_from_line(line))) {
			DEBUG_ERR("error calling os_conf_get_float_from_line");
			apr_file_close(f);
			return APR_EINVAL;
		    }
		}
		else if ('v' == line[0] && 'a' == line[1]) {
		    if (HUGE_VALF == ((conf->item)[*i].attack_value = os_conf_get_float_from_line(line))) {
			DEBUG_ERR("error calling os_conf_get_float_from_line");
			apr_file_close(f);
			return APR_EINVAL;
		    }
		}
		else if ('c' == line[0] && 'f' == line[1]) {
		    if (UINT_MAX == ((conf->item)[*i].capacity = os_conf_get_ulong_from_line(line))) {
			DEBUG_ERR("error calling os_conf_get_ulong_from_line");
			apr_file_close(f);
			return APR_EINVAL;
		    }
		}
		else if ('v' == line[0] && 'b' == line[1]) {
		    if (UINT_MAX == ((conf->item)[*i].speed = os_conf_get_ulong_from_line(line))) {
			DEBUG_ERR("error calling os_conf_get_ulong_from_line");
			apr_file_close(f);
			return APR_EINVAL;
		    }
		}
		else if ('k' == line[0] && 'm' == line[1]) {
		    if (UINT_MAX == ((conf->item)[*i].metal = os_conf_get_ulong_from_line(line))) {
			DEBUG_ERR("error calling os_conf_get_ulong_from_line");
			apr_file_close(f);
			return APR_EINVAL;
		    }
		}
		else if ('k' == line[0] && 'c' == line[1]) {
		    if (UINT_MAX == ((conf->item)[*i].cristal = os_conf_get_ulong_from_line(line))) {
			DEBUG_ERR("error calling os_conf_get_ulong_from_line");
			apr_file_close(f);
			return APR_EINVAL;
		    }
		}
		else if ('k' == line[0] && 'd' == line[1]) {
		    if (UINT_MAX == ((conf->item)[*i].deut = os_conf_get_ulong_from_line(line))) {
			DEBUG_ERR("error calling os_conf_get_ulong_from_line");
			apr_file_close(f);
			return APR_EINVAL;
		    }
		}
		else if ('c' == line[0] && 'c' == line[1]) {
		    unsigned int temp;

		    if (UINT_MAX == (temp = os_conf_get_ulong_from_line(line))) {
			DEBUG_ERR("error calling os_conf_get_ulong_from_line");
			apr_file_close(f);
			return APR_EINVAL;
		    }

		    (conf->item)[*i].fuel_consom = (unsigned short) temp;
		}
		else if (!strncmp("name", line, 4)) {
		    if (NULL == ((conf->item)[*i].name = os_conf_get_unquoted_str_from_line(pool, line))) {
			DEBUG_ERR("error calling os_conf_get_unquoted_str_from_line");
			apr_file_close(f);
			return APR_EINVAL;
		    }
		    (conf->item)[*i].shortname = item_thesaurus[*i].str;
		}
		else if ('\n' == line[0]) {
		    break;
		}
		else if ('[' == line[0]) {
		    DEBUG_DBG("Missing \\n before next element between ship [%s] and line %s", (conf->item)[*i].name, line);
		    break;
		}
		else {
		    DEBUG_DBG("unknown configuration element in [%s]: %s", (conf->item)[*i].name, line);
		}
	    }
	    (*i)++;
	}
    }

    if (APR_EOF != status) {
	DEBUG_ERR("error calling apr_file_gets: %s", apr_strerror(status, errbuf, 128));
    }
    else {
	status = APR_SUCCESS;
    }

    if (APR_SUCCESS != (status = apr_file_close(f))) {
	DEBUG_ERR("error calling apr_file_close: %s", apr_strerror(status, errbuf, 128));
    }

    return status;
}

static apr_status_t os_conf_parse_rapid_fire_file(apr_pool_t *pool, const char *filename, os_conf_t *conf)
{
    char line[1024];
    char errbuf[128];
    apr_hash_t *hash;
    apr_file_t *f;
    struct item_word_t *item1, *item2;
    char *name1, *name2;
    unsigned int temp;
    apr_size_t name1_len, name2_len;
    apr_status_t status;
    int i;

    hash = apr_hash_make(pool);
    for (i = 0; i < ITEM_END; i++) {
	apr_hash_set(hash, item_thesaurus[i].str, APR_HASH_KEY_STRING, &(item_thesaurus[i]));
    }

    if (APR_SUCCESS != (status = apr_file_open(&f, filename, APR_READ | APR_BUFFERED, APR_OS_DEFAULT, pool))) {
	DEBUG_ERR("error calling apr_file_open: %s", apr_strerror(status, errbuf, 128));
	return status;
    }

    while (APR_SUCCESS == (status = apr_file_gets(line, sizeof(line), f))) {
	/* fr[BB][AAI]=10; */
	name1 = &line[3];
	name1_len = strcspn(name1, "]");
	name2 = name1 + name1_len + 2;
	name2_len = strcspn(name2, "]");
	if (NULL == (item1 = apr_hash_get(hash, name1, name1_len))) {
	    DEBUG_ERR("error calling apr_hash_get, no item %.*s", (int) name1_len, name1);
	    status = APR_EINVAL;
	    break;
	}
	if (NULL == (item2 = apr_hash_get(hash, name2, name2_len))) {
	    DEBUG_ERR("error calling apr_hash_get, no item %.*s", (int) name2_len, name2);
	    status = APR_EINVAL;
	    break;
	}

	if (UINT_MAX == (temp = os_conf_get_ulong_from_line(line))) {
	    DEBUG_ERR("error calling os_conf_get_ulong_from_line");
	    apr_file_close(f);
	    return APR_EINVAL;
	}

	(conf->rapid_fire)[item1->item][item2->item] = (unsigned short) temp;
    }

    if (APR_EOF != status) {
	DEBUG_ERR("error calling apr_file_gets: %s", apr_strerror(status, errbuf, 128));
    }
    else {
	status = APR_SUCCESS;
    }

    if (APR_SUCCESS != (status = apr_file_close(f))) {
	DEBUG_ERR("error calling apr_file_close: %s", apr_strerror(status, errbuf, 128));
    }

    return status;
}

extern os_conf_t *os_conf_make(apr_pool_t *pool, const char *dirname)
{
    char errbuf[128];
    os_conf_t *conf;
    apr_status_t status;
    int i = 0, j;

    conf = apr_pcalloc(pool, sizeof(struct os_conf_t));

    if (NULL != dirname) {
	if (APR_SUCCESS != (status = os_conf_parse_item_file(pool, apr_pstrcat(pool, dirname, "fleet.cnf", NULL), conf, &i))) {
	    DEBUG_ERR("error calling os_conf_parse_item_file: %s dir %s", apr_strerror(status, errbuf, 128), dirname);
	    return NULL;
	}

	if (APR_SUCCESS !=
	    (status = os_conf_parse_item_file(pool, apr_pstrcat(pool, dirname, "defense.cnf", NULL), conf, &i))) {
	    DEBUG_ERR("error calling os_conf_parse_item_file: %s dir %s", apr_strerror(status, errbuf, 128), dirname);
	    return NULL;
	}

	if (APR_SUCCESS !=
	    (status = os_conf_parse_rapid_fire_file(pool, apr_pstrcat(pool, dirname, "rapidfire.cnf", NULL), conf))) {
	    DEBUG_ERR("error calling os_conf_parse_item_file: %s dir %s", apr_strerror(status, errbuf, 128), dirname);
	    return NULL;
	}
    }
    else {
	/* Use hardcoded values */
	for (i = 0; i < ITEM_END; i++) {
	    (conf->item)[i].name = item_const_def[i].name;
	    (conf->item)[i].shortname = item_thesaurus[i].str;
	    (conf->item)[i].shield_points = item_const_def[i].shield_points;
	    (conf->item)[i].attack_value = item_const_def[i].attack_value;
	    (conf->item)[i].structure_points = item_const_def[i].structure_points;
	    (conf->item)[i].capacity = item_const_def[i].capacity;
	    (conf->item)[i].speed = item_const_def[i].speed;
	    (conf->item)[i].metal = item_const_def[i].metal;
	    (conf->item)[i].cristal = item_const_def[i].cristal;
	    (conf->item)[i].deut = item_const_def[i].deut;
	    (conf->item)[i].fuel_consom = item_const_def[i].fuel_consom;
	}
	for (i = 0; i < ITEM_END; i++) {
	    for (j = 0; j < ITEM_END; j++) {
		conf->rapid_fire[i][j] = rapid_fire_const[j + i * (ITEM_END)];
	    }
	}
    }

    return conf;
}

extern float os_conf_get_shield_points(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].shield_points;
}

extern float os_conf_get_attack_value(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].attack_value;
}

extern unsigned int os_conf_get_structure_points(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].structure_points;
}

extern short unsigned int os_conf_get_rapid_fire(const os_conf_t *conf, enum Item_enum atk, enum Item_enum def)
{
    return (conf->rapid_fire)[atk][def];
}

extern const char *os_conf_get_shortname(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].shortname;
}

extern const char *os_conf_get_name(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].name;
}

extern float os_conf_get_ship_price(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].metal * META_RATIO + (conf->item)[item].cristal * CRST_RATIO +
	(conf->item)[item].deut * DEUT_RATIO;
}

extern float os_conf_get_missile_price(const os_conf_t *conf)
{
    return 12500.0f * META_RATIO + 2500.0f * CRST_RATIO + 10000.0f * DEUT_RATIO;
}

extern unsigned int os_conf_get_ship_metal(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].metal;
}

extern unsigned int os_conf_get_ship_cristal(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].cristal;
}

extern unsigned int os_conf_get_ship_deut(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].deut;
}

extern unsigned int os_conf_get_ship_capacity(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].capacity;
}

extern unsigned short os_conf_get_fuel_consumption(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].fuel_consom;
}

extern unsigned int os_conf_get_speed(const os_conf_t *conf, enum Item_enum item)
{
    return (conf->item)[item].speed;
}


