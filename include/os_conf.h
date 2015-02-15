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

#ifndef OS_CONF_H
#define OS_CONF_H

#include <apr_pools.h>

typedef struct os_conf_t os_conf_t;

enum Item_enum
{
    PT = 0x0,			/* Fleet items */
    GT,
    CLE,
    CLO,
    CR,
    VB,
    VC,
    REC,
    SE,
    BB,
    SAT,
    DEST,
    EDLM,
    TRAC,
    LM,				/* Beginning of defense items */
    ALLE,
    ALLO,
    CG,
    AAI,
    LP,
    PB,
    GB,
    ITEM_END,
};

enum Misc_enum
{
    ENERGY = ITEM_END,
    METL,
    CRIS,
    DEUT,
    DAMG,
    SHLD,
    LIFE,
    POSITION,
    MIT,
    MISC_END
};

#define EPSILON 0.001f
#define META_RATIO 1LLU
#define CRST_RATIO 1LLU
#define DEUT_RATIO 2LLU
#define ABS(x) (((x) > 0.0) ? (x) : (-(x)))

os_conf_t *os_conf_make(apr_pool_t *pool, const char *dirname);

float os_conf_get_shield_points(const os_conf_t *conf, enum Item_enum item);

float os_conf_get_attack_value(const os_conf_t *conf, enum Item_enum item);

unsigned int os_conf_get_structure_points(const os_conf_t *conf, enum Item_enum item);

short unsigned int os_conf_get_rapid_fire(const os_conf_t *conf, enum Item_enum atk, enum Item_enum def);

const char *os_conf_get_name(const os_conf_t *conf, enum Item_enum item);

const char *os_conf_get_shortname(const os_conf_t *conf, enum Item_enum item);

float os_conf_get_ship_price(const os_conf_t *conf, enum Item_enum item);

float os_conf_get_missile_price(const os_conf_t *conf);

unsigned int os_conf_get_ship_metal(const os_conf_t *conf, enum Item_enum item);

unsigned int os_conf_get_ship_cristal(const os_conf_t *conf, enum Item_enum item);

unsigned int os_conf_get_ship_deut(const os_conf_t *conf, enum Item_enum item);

unsigned int os_conf_get_ship_capacity(const os_conf_t *conf, enum Item_enum item);

unsigned short os_conf_get_fuel_consumption(const os_conf_t *conf, enum Item_enum item);

unsigned int os_conf_get_speed(const os_conf_t *conf, enum Item_enum item);
#endif /* OS_CONF_H */


