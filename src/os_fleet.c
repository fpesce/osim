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
#include <math.h>
#include <values.h>
#include <time.h>

#include <apr_strings.h>

#include <pcre.h>
#include "debug.h"
#include "napr_galife.h"
#include "os_conf.h"
#include "os_fleet.h"

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

struct os_ship_t
{
    const char *shortname;
    float shield_points;
    float shield_points_percent;	/* opti: 1 / shield_points */
    float attack_value;
    float structure_points;
    float structure_points_percent;	/* opti: 1 / structure_points */
    float price;
    unsigned int speed;
    unsigned int consumption;
    unsigned int capacity;
    unsigned int metl_price;
    unsigned int crst_price;
    unsigned int deut_price;
};

typedef struct os_ship_t os_ship_t;

struct os_battle_ship_t
{
    float shield_points;
    float structure_points;
    unsigned char type;
    int exploded:1;
};

typedef struct os_battle_ship_t os_battle_ship_t;

struct os_fleet_t
{
    os_ship_t os_ship[ITEM_END];	/* Contain data with technologies applied */
    unsigned int initial_repartition[ITEM_END];
    unsigned int current_repartition[ITEM_END];
    apr_pool_t *pool;
    os_battle_ship_t *ships_hit_table;
    char *coord;
    unsigned int ship_initial_count;
    unsigned int mit;		/*number of interception missiles */
    unsigned int ship_count;	/* usefull for rand(), 0 when fleet destroyed */
    unsigned int metal;
    unsigned int cristal;
    unsigned int deut;
    apr_uint64_t metl_recycled;
    apr_uint64_t crst_recycled;
    apr_uint64_t metl_lost;
    apr_uint64_t crst_lost;
    apr_uint64_t deut_lost;
    apr_uint64_t ship_metal;
    apr_uint64_t ship_cristal;
    apr_uint64_t ship_deut;
    apr_uint64_t capacity;
    unsigned char attack;	/* Technos */
    unsigned char shield;
    unsigned char structr;
    unsigned char combustion;
    unsigned char impulsion;
    unsigned char hyperespace;

    unsigned char guess_mode;
    enum Item_enum limit;
    enum Fleet_enum type;
};

extern os_fleet_t *os_fleet_make(apr_pool_t *pool, enum Fleet_enum type)
{
    os_fleet_t *result;

    result = apr_pcalloc(pool, sizeof(struct os_fleet_t));
    result->type = type;
    result->pool = pool;

    return result;
}

extern void os_fleet_to_guess(os_fleet_t *fleet)
{
    fleet->guess_mode = 1;
}

static inline unsigned int os_fleet_csv_parse_get_ulong(const char **conf)
{
    const char *ptr;
    unsigned long tmp;

    if (NULL == (ptr = strchr(*conf, ','))) {
	DEBUG_ERR("can't parse %s", *conf);
	return UINT_MAX;
    }
    *conf = ptr + 1;
    tmp = strtoul(*conf, NULL, 10);
    if (ULONG_MAX == tmp)
	return UINT_MAX;

    return (unsigned int) tmp;
}

static inline unsigned char os_fleet_csv_parse_get_uchar(const char **conf)
{
    const char *ptr;
    unsigned int tmp;

    if (NULL == (ptr = strchr(*conf, ','))) {
	DEBUG_ERR("can't parse %s", *conf);
	return UCHAR_MAX;
    }
    *conf = ptr + 1;
    /* get damage, shield, life */
    tmp = strtoul(*conf, NULL, 10);
    if (255 < tmp)
	tmp = UCHAR_MAX;

    return tmp;
}

/*
 * for ATK_FLT:
 * - damage,shield,life,combustion,impulsion,hyperespace,coord,pt,gt,cle,clo,cr,vb,vc,rec,se,bb,0,dest,edlm,mipldm,mipalle,mipallo,mipcg,mipaai,miplp,mippb,mipgb
 * for DEF_FLT:
 * - damage,shield,life,combustion,impulsion,hyperespace,coord,metal,cristal,deut,pt,gt,cle,clo,cr,vb,vc,rec,se,bb,sat,dest,edlm,ldm,alle,allo,cg,aai,lp,pb,gb
 */
extern apr_status_t os_fleet_set_conf(os_fleet_t *fleet, const char *conf)
{
    const char *ptr, *fleet_conf;
    unsigned int tmp;
    enum Item_enum i;

    /* get damage, shield, life */
    tmp = strtoul(conf, NULL, 10);
    if (255 < tmp) {
	DEBUG_ERR("can't parse %s for attack [%u] conf:%s", conf, tmp, conf);
	return APR_EINVAL;
    }
    fleet->attack = (unsigned char) tmp;

    fleet_conf = conf;

    if (UCHAR_MAX == (fleet->shield = os_fleet_csv_parse_get_uchar(&fleet_conf))) {
	DEBUG_ERR("can't parse %s for shield fleet_conf:%s", conf, fleet_conf);
	return APR_EINVAL;
    }

    if (UCHAR_MAX == (fleet->structr = os_fleet_csv_parse_get_uchar(&fleet_conf))) {
	DEBUG_ERR("can't parse %s for structure/life fleet_conf:%s", conf, fleet_conf);
	return APR_EINVAL;
    }

    if (ATK_FLT == fleet->type) {
	if (UCHAR_MAX == (fleet->combustion = os_fleet_csv_parse_get_uchar(&fleet_conf))) {
	    DEBUG_ERR("can't parse %s for internal combustion fleet_conf:%s", conf, fleet_conf);
	    return APR_EINVAL;
	}

	if (UCHAR_MAX == (fleet->impulsion = os_fleet_csv_parse_get_uchar(&fleet_conf))) {
	    DEBUG_ERR("can't parse %s for impulsion reactor fleet_conf:%s", conf, fleet_conf);
	    return APR_EINVAL;
	}

	if (UCHAR_MAX == (fleet->hyperespace = os_fleet_csv_parse_get_uchar(&fleet_conf))) {
	    DEBUG_ERR("can't parse %s for hyperespace fleet_conf:%s", conf, fleet_conf);
	    return APR_EINVAL;
	}
    }
    if (NULL == (ptr = strchr(fleet_conf, ','))) {
	/* If it stops after technos, we might be in guess mode (without coord :\) */
	fleet->guess_mode = 1;
	return APR_SUCCESS;
    }
    fleet_conf = ptr + 1;
    if (NULL == (ptr = strchr(fleet_conf, ','))) {
	DEBUG_ERR("can't get next parse");
	return APR_EINVAL;
    }
    fleet->coord = apr_pstrndup(fleet->pool, fleet_conf, ptr - fleet_conf);

    switch (fleet->type) {
    case ATK_FLT:
	fleet->limit = LM;
	break;
    case DEF_FLT:
	if (UINT_MAX == (fleet->metal = os_fleet_csv_parse_get_ulong(&fleet_conf))) {
	    DEBUG_ERR("can't parse %s for metal fleet_conf:%s", conf, fleet_conf);
	    return APR_EINVAL;
	}

	if (UINT_MAX == (fleet->cristal = os_fleet_csv_parse_get_ulong(&fleet_conf))) {
	    DEBUG_ERR("can't parse %s for cristal [%u] fleet_conf:%s", conf, tmp, fleet_conf);
	    return APR_EINVAL;
	}

	if (UINT_MAX == (fleet->deut = os_fleet_csv_parse_get_ulong(&fleet_conf))) {
	    DEBUG_ERR("can't parse %s for deut fleet_conf:%s", conf, fleet_conf);
	    return APR_EINVAL;
	}

	fleet->limit = GB + 1;
	break;
    default:
	DEBUG_ERR("Unknown fleet type");
	return APR_EINVAL;
    }

    for (i = PT, ptr = strchr(fleet_conf, ','), fleet_conf = ptr + 1; (i < ITEM_END) && (ptr != NULL);
	 i++, ptr = strchr(fleet_conf, ','), fleet_conf = ptr + 1) {
	if (UINT_MAX == (fleet->initial_repartition[i] = strtoul(fleet_conf, NULL, 10))) {
	    DEBUG_ERR("can't parse %s, i:%i, fleet_conf:%s", conf, i, fleet_conf);
	    return APR_EINVAL;
	}

	if ((ATK_FLT != fleet->type) || (i < LM))
	    fleet->ship_initial_count += fleet->initial_repartition[i];

	if ((ATK_FLT == fleet->type) && (SAT == i) && (0 != fleet->initial_repartition[i])) {
	    DEBUG_ERR("invalid attacker, %u satellites found in %s", fleet->initial_repartition[i], conf);
	    return APR_EINVAL;
	}
    }

    if ((DEF_FLT == fleet->type) && (i == ITEM_END)) {
	/* Read mit number */
	if (NULL != ptr) {
	    if (UINT_MAX == (fleet->mit = strtoul(fleet_conf, NULL, 10))) {
		DEBUG_ERR("can't parse %s, i:%i, fleet_conf:%s", conf, i, fleet_conf);
		return APR_EINVAL;
	    }
	}
	else {
	    DEBUG_DBG("Missing MIT definition.");
	}
    }

    fleet->ships_hit_table = apr_palloc(fleet->pool, fleet->ship_initial_count * sizeof(struct os_battle_ship_t));

    return APR_SUCCESS;
}

extern apr_status_t os_fleet_parse(os_fleet_t *fleet, const os_conf_t *conf)
{
    enum Item_enum i;

    for (i = PT; i < ITEM_END; i++) {
	(fleet->os_ship)[i].shield_points = os_conf_get_shield_points(conf, i) * (1.0f + (0.1f * (float) fleet->shield));
	(fleet->os_ship)[i].shield_points_percent = 100.0f / (fleet->os_ship)[i].shield_points;
	(fleet->os_ship)[i].attack_value = os_conf_get_attack_value(conf, i) * (1.0f + (0.1f * (float) fleet->attack));
	(fleet->os_ship)[i].structure_points =
	    os_conf_get_structure_points(conf, i) * (1.0f + (0.1f * (float) fleet->structr)) / 10.0f;
	(fleet->os_ship)[i].structure_points_percent = 100.0f / (fleet->os_ship)[i].structure_points;
	(fleet->os_ship)[i].price = os_conf_get_ship_price(conf, i);
	(fleet->os_ship)[i].capacity = os_conf_get_ship_capacity(conf, i);
	(fleet->os_ship)[i].metl_price = os_conf_get_ship_metal(conf, i);
	(fleet->os_ship)[i].crst_price = os_conf_get_ship_cristal(conf, i);
	(fleet->os_ship)[i].deut_price = os_conf_get_ship_deut(conf, i);
	switch (i) {
	case GT:
	case CLE:
	case REC:
	case SE:
	    (fleet->os_ship)[i].speed = os_conf_get_speed(conf, i) * (1.0f + (0.1f * (float) fleet->combustion));
	    break;
	case PT:
	case CLO:
	case CR:
	case VC:
	    (fleet->os_ship)[i].speed = os_conf_get_speed(conf, i) * (1.0f + (0.2f * (float) fleet->impulsion));
	    break;
	case BB:
	case VB:
	case DEST:
	case EDLM:
	case TRAC:
	    (fleet->os_ship)[i].speed = os_conf_get_speed(conf, i) * (1.0f + (0.3f * (float) fleet->hyperespace));
	    break;
	case LM:
	case ALLE:
	case ALLO:
	case CG:
	case AAI:
	case LP:
	case PB:
	case GB:
	    if (ATK_FLT == fleet->type) {
		(fleet->os_ship)[i].price = os_conf_get_missile_price(conf);
		(fleet->os_ship)[i].metl_price = 12500;
		(fleet->os_ship)[i].crst_price = 2500;
		(fleet->os_ship)[i].deut_price = 10000;
	    }
	    break;

	default:
	    (fleet->os_ship)[i].speed = 0UL;
	}
	(fleet->os_ship)[i].consumption = os_conf_get_fuel_consumption(conf, i);
	(fleet->os_ship)[i].shortname = os_conf_get_shortname(conf, i);
	/*DEBUG_DBG("Ship [%s] : %u", (fleet->os_ship)[i].shortname, fleet->initial_repartition[i]); */
    }

    return APR_SUCCESS;
}

static inline void os_fleet_battle_init(os_fleet_t *fleet)
{
    unsigned int ships_idx, count;
    enum Item_enum i;

    memset(fleet->ships_hit_table, 0, fleet->ship_initial_count);
    fleet->ship_count = 0;

    for (i = PT; i < fleet->limit; i++) {
	count = fleet->ship_count;
	for (ships_idx = 0; ships_idx < fleet->initial_repartition[i]; ships_idx++) {
	    /* No need to set shields, they will be reset */
	    /*(fleet->ships_hit_table)[count + ships_idx].shield_points = (fleet->os_ship)[i].shield_points; */
	    (fleet->ships_hit_table)[count + ships_idx].structure_points = (fleet->os_ship)[i].structure_points;
	    (fleet->ships_hit_table)[count + ships_idx].type = i;
	    (fleet->ships_hit_table)[count + ships_idx].exploded &= 0x0;
	}
	fleet->current_repartition[i] = ships_idx;
	fleet->ship_count += ships_idx;
    }
}

static inline void os_fleet_battle_maximize_shield(os_fleet_t *fleet)
{
    unsigned int count;
    os_battle_ship_t *__restrict__ ships_hit_table = fleet->ships_hit_table;
    os_battle_ship_t *__restrict__ ships_hit_table_bis = fleet->ships_hit_table;
    os_ship_t *__restrict__ os_ship = fleet->os_ship;

    for (count = 0; count < fleet->ship_count; count++)
	ships_hit_table[count].shield_points = os_ship[ships_hit_table_bis[count].type].shield_points;
    /*(fleet->ships_hit_table)[count].shield_points = (fleet->os_ship)[(fleet->ships_hit_table)[count].type].shield_points; */
}

#define RND_ARRAY_SIZE '\55'
static unsigned int rsl[RND_ARRAY_SIZE];
static unsigned char last_rsl_used;

static inline void my_srand()
{
    unsigned char i;

    srand((unsigned int) time(NULL));
    for (i = 0; i < RND_ARRAY_SIZE; ++i)
	rsl[i] = (unsigned int) UINT_MAX *(rand() / (RAND_MAX + 1.0));

    for (i = 0; i < RND_ARRAY_SIZE; ++i)
	rsl[i] = rsl[i] + rsl[(i + 24) % RND_ARRAY_SIZE];
}

static inline unsigned int my_rand(unsigned int limit)
{
    unsigned char i;

    last_rsl_used++;
    if (last_rsl_used == RND_ARRAY_SIZE) {
	for (i = 0; i < RND_ARRAY_SIZE; ++i)
	    rsl[i] = rsl[i] + rsl[(i + 24) % RND_ARRAY_SIZE];
	last_rsl_used = 0;
    }

    return rsl[last_rsl_used] % limit;
}

static inline float my_randf()
{
    unsigned char i;

    last_rsl_used++;
    if (last_rsl_used == RND_ARRAY_SIZE) {
	for (i = 0; i < RND_ARRAY_SIZE; ++i)
	    rsl[i] = rsl[i] + rsl[(i + 24) % RND_ARRAY_SIZE];
	last_rsl_used = 0;
    }

    return (float) rsl[last_rsl_used] / (float) UINT_MAX;
}

static const short unsigned int rapid_fire_const[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 2000, 0, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 2000, 0, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 2000, 0, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3300, 0, 0, 0, 0, 0, 0, 0, 2000, 0, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 3300, 0, 0, 0, 0, 0, 2000, 0, 2000, 0, 0, 0, 1000, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 2000, 0, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 2000, 0, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 2000, 0, 2000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 2000, 0, 2000, 0, 0, 0, 500, 500, 1000, 0, 1000, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 2000, 0, 2000, 0, 0, 5000, 0, 1000, 0, 0, 0, 0, 0, 0,
    40, 40, 50, 100, 303, 333, 40, 40, 8, 400, 8, 2000, 0, 667, 50, 50, 100, 200, 100, 0, 0, 0,
    3300, 3300, 0, 4, 4, 7, 0, 0, 2000, 0, 2000, 0, 0, 5000, 0, 1000, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static inline int rapid_fired(enum Item_enum atktype, enum Item_enum deftype)
{
    unsigned short int rf;

    rf = rapid_fire_const[deftype + atktype * (GB + 1)];
    if (0 == rf)
	return 0;
    else {
	unsigned short int rsi;
	rsi = (unsigned short int) my_rand(10000UL);
	return rsi >= rf;
    }
}

static void os_fleet_battle_shoot(const os_fleet_t *attacker, unsigned int attack_ship_number, os_fleet_t *defender,
				  const os_conf_t *conf)
{
    enum Item_enum attack_ship, defend_ship;
    unsigned int rnd_nb;
    float damage;
    unsigned char has_rapid_fired;

    /* First, get the type of the ship that will shoot from attacker */
    attack_ship = (attacker->ships_hit_table)[attack_ship_number].type;

    /* Then this ship will attack as long as possible */
    do {
	/* find the target */
	rnd_nb = my_rand(defender->ship_count);
	defend_ship = (defender->ships_hit_table)[rnd_nb].type;
	has_rapid_fired = 0;

	damage = attacker->os_ship[attack_ship].attack_value;
	if (1.0f <= (damage * defender->os_ship[defend_ship].shield_points_percent)) {
	    if (!(0x1 & (defender->ships_hit_table)[rnd_nb].exploded)) {
		float tmp;
		damage = floorf(damage * 10.0f) * 0.1f;

		tmp = damage;
		damage -= (defender->ships_hit_table)[rnd_nb].shield_points;
		(defender->ships_hit_table)[rnd_nb].shield_points -= tmp;

		if (damage > 0.0f) {
		    (defender->ships_hit_table)[rnd_nb].structure_points -= damage;
		    /* If we are here, it means that the shield has been destroyed */
		    (defender->ships_hit_table)[rnd_nb].shield_points = 0.0f;

		    if ((defender->ships_hit_table)[rnd_nb].structure_points < 0.0f) {
			(defender->ships_hit_table)[rnd_nb].exploded |= 0x1;
		    }
		    else if ((defender->ships_hit_table)[rnd_nb].structure_points <=
			     (0.7f * defender->os_ship[defend_ship].structure_points)) {
			/*
			 * ship probably explodes, when hull damage >= 30 %
			 */
			if ((float) my_rand(100UL) >=
			    ((defender->ships_hit_table)[rnd_nb].structure_points *
			     defender->os_ship[defend_ship].structure_points_percent)) {
			    (defender->ships_hit_table)[rnd_nb].exploded |= 0x1;
			}
		    }
		}
	    }

	    has_rapid_fired = rapid_fired(attack_ship, defend_ship);
	}
    } while (0 != has_rapid_fired);
}

static inline void os_fleet_battle_remove_exploded_ships(os_fleet_t *fleet)
{
    unsigned int ship_idx;
#if 1
    for (ship_idx = 0; ship_idx < fleet->ship_count; ship_idx++) {
	/*DEBUG_DBG("Ship_idx:%u Ship_count:%u", ship_idx, fleet->ship_count); */
	if (((fleet->ships_hit_table)[ship_idx].exploded & 0x1) && (fleet->ship_count != 0)) {
	    (fleet->ship_count)--;
	    while (((fleet->ships_hit_table)[fleet->ship_count].exploded & 0x1) && (ship_idx < fleet->ship_count)) {
		((fleet->current_repartition)[(fleet->ships_hit_table)[fleet->ship_count].type])--;
		(fleet->ship_count)--;
		/*DEBUG_DBG("Ship_idx:%u Ship_count:%u", ship_idx, fleet->ship_count); */
	    }
	    ((fleet->current_repartition)[(fleet->ships_hit_table)[ship_idx].type])--;
	    (fleet->ships_hit_table)[ship_idx].structure_points =
		(fleet->ships_hit_table)[fleet->ship_count].structure_points;
	    /* No need to set shields, they will be reset */
	    /*(fleet->ships_hit_table)[ship_idx].shield_points = (fleet->ships_hit_table)[fleet->ship_count].shield_points; */
	    (fleet->ships_hit_table)[ship_idx].type = (fleet->ships_hit_table)[fleet->ship_count].type;
	    (fleet->ships_hit_table)[ship_idx].exploded &= 0x0;
	}
	/*DEBUG_DBG("Ship_idx:%u Ship_count:%u", ship_idx, fleet->ship_count); */
    }
#else
    unsigned int nb_removed = 0UL, last_removed_ship = 0UL;

    for (last_removed_ship = 0; (last_removed_ship < fleet->ship_count)
	 && !((fleet->ships_hit_table)[last_removed_ship].exploded & 0x1); last_removed_ship++);

    if (0UL != last_removed_ship)
	nb_removed++;

    for (ship_idx = last_removed_ship + 1; ship_idx < fleet->ship_count; ship_idx++) {
	if (!((fleet->ships_hit_table)[ship_idx].exploded & 0x1)) {
	    /*DEBUG_DBG("Move:%u To:%u Nb_rem:%u", ship_idx, last_removed_ship, nb_removed); */
	    if ((fleet->ships_hit_table)[last_removed_ship].exploded & 0x1) {
		((fleet->current_repartition)[(fleet->ships_hit_table)[last_removed_ship].type])--;
		/*DEBUG_DBG("Remove ship of type [%s] new size : [%u]", (fleet->os_ship)[(fleet->ships_hit_table)[last_removed_ship].type].shortname, ((fleet->current_repartition)[(fleet->ships_hit_table)[last_removed_ship].type])); */
	    }
	    (fleet->ships_hit_table)[last_removed_ship].structure_points =
		(fleet->ships_hit_table)[ship_idx].structure_points;
	    (fleet->ships_hit_table)[last_removed_ship].type = (fleet->ships_hit_table)[ship_idx].type;
	    (fleet->ships_hit_table)[last_removed_ship].exploded &= 0x0;
	    last_removed_ship++;
	}
	else {
	    nb_removed++;
	}
    }
    fleet->ship_count -= nb_removed;
    /*DEBUG_DBG("ship->count:%u", fleet->ship_count); */
#endif
}

static inline void os_fleet_launch_missile(os_fleet_t *attacker, os_fleet_t *defender)
{
    unsigned int initial_repartition[ITEM_END];
    unsigned int nb_dest;
    unsigned int mit;
    float damage;
    enum Item_enum i;

    mit = defender->mit;
    memcpy(initial_repartition, defender->initial_repartition, ITEM_END * sizeof(unsigned int));
    for (i = LM; i <= GB; i++) {
	if (attacker->initial_repartition[i] > mit) {
	    damage = (attacker->initial_repartition[i] - mit) * 12000.0f * (1.0f + attacker->attack / 10.0f);
	    if (attacker->initial_repartition[i] > mit) {
		mit = 0UL;
	    }
	    else {
		mit -= attacker->initial_repartition[i];
	    }
	    nb_dest = damage / defender->os_ship[i].structure_points;
	    nb_dest = MIN(nb_dest, defender->initial_repartition[i]);
	    /*DEBUG_DBG("%u missiles killed %u %s", attacker->initial_repartition[i], nb_dest,  (defender->os_ship)[i].shortname); */
	    defender->initial_repartition[i] -= nb_dest;
	}
	attacker->current_repartition[i] = 0UL;
    }
    os_fleet_battle_init(defender);
    memcpy(defender->initial_repartition, initial_repartition, ITEM_END * sizeof(unsigned int));
}

static inline unsigned char os_fleet_onebattle(os_fleet_t *attacker, os_fleet_t *defender, const os_conf_t *conf)
{
    unsigned int ship_idx;
    unsigned char i;

    os_fleet_battle_init(attacker);
    os_fleet_launch_missile(attacker, defender);

    for (i = '\0'; (i < MAX_ROUND_NUMBER) && (0 != attacker->ship_count) && (defender->ship_count); i++) {
	os_fleet_battle_maximize_shield(attacker);
	os_fleet_battle_maximize_shield(defender);

	for (ship_idx = 0; ship_idx < attacker->ship_count; ship_idx++) {
	    os_fleet_battle_shoot(attacker, ship_idx, defender, conf);
	}

	for (ship_idx = 0; ship_idx < defender->ship_count; ship_idx++) {
	    os_fleet_battle_shoot(defender, ship_idx, attacker, conf);
	}

	/*DEBUG_DBG("Remove defender ships"); */
	os_fleet_battle_remove_exploded_ships(defender);
	/*DEBUG_DBG("Remove attacker ships"); */
	os_fleet_battle_remove_exploded_ships(attacker);
    }

    return i;
}

extern void os_fleet_battle(os_fleet_t *attacker, os_fleet_t *defender, unsigned int nb_simu, const os_conf_t *conf,
			    unsigned char mode)
{
    unsigned int atk_avg[ITEM_END], def_avg[ITEM_END], atk_wrst[ITEM_END], def_wrst[ITEM_END], atk_bst[ITEM_END],
	def_bst[ITEM_END];
    apr_uint64_t recycl_m, recycl_c, atk_loss_m, atk_loss_c, atk_loss_d, def_loss_m, def_loss_c, def_loss_d;
    char *pct_M_atk_str, *pct_C_atk_str, *pct_M_def_str, *pct_C_def_str;
    unsigned int nb_atk_vict, nb_def_vict, nb_round, distance, flight_time;
    float atk_max_score, def_max_score, atk_min_score, def_min_score, atk_score, def_score;
    int j, k;

    my_srand();
    if (0UL == (distance = os_fleet_distance(attacker->coord, defender->coord))) {
	DEBUG_ERR("Invalid coordinates");
	return;
    }
    atk_max_score = def_max_score = 0UL;
    atk_min_score = def_min_score = UINT_MAX;

    nb_atk_vict = nb_def_vict = nb_round = recycl_m = recycl_c = atk_loss_m = atk_loss_c = atk_loss_d = def_loss_m =
	def_loss_c = def_loss_d = 0UL;
    memset(atk_avg, 0UL, sizeof(unsigned int) * ITEM_END);
    memset(def_avg, 0UL, sizeof(unsigned int) * ITEM_END);
    memset(atk_wrst, 0UL, sizeof(unsigned int) * ITEM_END);
    memset(def_wrst, 0UL, sizeof(unsigned int) * ITEM_END);
    memset(atk_bst, 0UL, sizeof(unsigned int) * ITEM_END);
    memset(def_bst, 0UL, sizeof(unsigned int) * ITEM_END);

    if (attacker->guess_mode || defender->guess_mode) {
	DEBUG_ERR("invalid simulation, one is in guess mode (only technos precised)");
	return;
    }

    for (k = 0; k < nb_simu; k++) {
	nb_round += os_fleet_onebattle(attacker, defender, conf);

	if (0 == attacker->ship_count)
	    nb_def_vict++;
	else if (0 == defender->ship_count)
	    nb_atk_vict++;

	for (j = 0; j < LM; j++) {
	    recycl_m +=
		(((attacker->initial_repartition[j] -
		   attacker->current_repartition[j]) * attacker->os_ship[j].metl_price) * 0.3f);
	    atk_loss_m +=
		((attacker->initial_repartition[j] - attacker->current_repartition[j]) * attacker->os_ship[j].metl_price);
	    recycl_m +=
		(((defender->initial_repartition[j] -
		   defender->current_repartition[j]) * defender->os_ship[j].metl_price) * 0.3f);
	    def_loss_m +=
		((defender->initial_repartition[j] - defender->current_repartition[j]) * defender->os_ship[j].metl_price);
	    recycl_c +=
		(((attacker->initial_repartition[j] -
		   attacker->current_repartition[j]) * attacker->os_ship[j].crst_price) * 0.3f);
	    atk_loss_c +=
		((attacker->initial_repartition[j] - attacker->current_repartition[j]) * attacker->os_ship[j].crst_price);
	    recycl_c +=
		(((defender->initial_repartition[j] -
		   defender->current_repartition[j]) * defender->os_ship[j].crst_price) * 0.3f);
	    def_loss_c +=
		((defender->initial_repartition[j] - defender->current_repartition[j]) * defender->os_ship[j].crst_price);
	    atk_loss_d +=
		((attacker->initial_repartition[j] - attacker->current_repartition[j]) * attacker->os_ship[j].deut_price);
	    def_loss_d +=
		((defender->initial_repartition[j] - defender->current_repartition[j]) * defender->os_ship[j].deut_price);
	}
	for (; j < ITEM_END; j++) {
	    def_loss_m +=
		((defender->initial_repartition[j] - defender->current_repartition[j]) * defender->os_ship[j].metl_price);
	    def_loss_c +=
		((defender->initial_repartition[j] - defender->current_repartition[j]) * defender->os_ship[j].crst_price);
	    def_loss_d +=
		((defender->initial_repartition[j] - defender->current_repartition[j]) * defender->os_ship[j].deut_price);
	}
	/*DEBUG_DBG("recycl_m %llu recycl_c %llu", recycl_m, recycl_c); */
	/*DEBUG_DBG("atk_loss_m %llu atk_loss_c %llu atk_loss_d %llu", atk_loss_m, atk_loss_c, atk_loss_d); */
	/*DEBUG_DBG("def_loss_m %llu def_loss_c %llu def_loss_d %llu", def_loss_m, def_loss_c, def_loss_d); */

	for (j = 0, atk_score = 0UL; j < LM; j++) {
	    atk_avg[j] += attacker->current_repartition[j];
	    atk_score += attacker->current_repartition[j] * attacker->os_ship[j].price;
	}
	if (atk_score >= atk_max_score) {
	    for (j = 0; j < LM; j++)
		atk_bst[j] = attacker->current_repartition[j];
	    atk_max_score = atk_score;
	}
	if (atk_score <= atk_min_score) {
	    for (j = 0; j < LM; j++)
		atk_wrst[j] = attacker->current_repartition[j];
	    atk_min_score = atk_score;
	}
	for (j = 0, def_score = 0UL; j < ITEM_END; j++) {
	    def_avg[j] += defender->current_repartition[j];
	    def_score += defender->current_repartition[j] * defender->os_ship[j].price;
	}
	if (def_score >= def_max_score) {
	    for (j = 0; j < ITEM_END; j++)
		def_bst[j] = defender->current_repartition[j];
	    def_max_score = def_score;
	}
	if (def_score <= def_min_score) {
	    for (j = 0; j < ITEM_END; j++)
		def_wrst[j] = defender->current_repartition[j];
	    def_min_score = def_score;
	}
    }

    if (mode & OS_MODE_HTML) {
	fprintf(stdout, "<p>attacker,%s,", attacker->coord);
	for (j = 0; j < ITEM_END; j++) {
	    fprintf(stdout, "%s (%u), ", os_conf_get_shortname(conf, j), atk_avg[j] / nb_simu);
	}
	fprintf(stdout, "%u victoires, %u nuls, %u round", nb_atk_vict, (nb_simu - (nb_atk_vict + nb_def_vict)),
		nb_round / nb_simu);
	fprintf(stdout,
		", %" APR_UINT64_T_FMT " metal perdu, %" APR_UINT64_T_FMT " cristal perdu, %" APR_UINT64_T_FMT
		" deut perdu, %u deut consommé</p><br />", atk_loss_m / nb_simu, atk_loss_c / nb_simu, atk_loss_d / nb_simu,
		os_fleet_consumption(attacker, distance, &flight_time));

	fprintf(stdout, "<p>defender,%s,%u,%u,%u,", defender->coord, defender->metal, defender->cristal, defender->deut);
	for (j = 0; j < ITEM_END; j++) {
	    fprintf(stdout, "%s (%u), ", os_conf_get_shortname(conf, j), def_avg[j] / nb_simu);
	}
	fprintf(stdout, "%u victoires, %u nuls, %u round", nb_def_vict, (nb_simu - (nb_atk_vict + nb_def_vict)),
		nb_round / nb_simu);
	fprintf(stdout,
		", %" APR_UINT64_T_FMT " metal perdu, %" APR_UINT64_T_FMT " cristal perdu, %" APR_UINT64_T_FMT
		" deut perdu</p><br />", def_loss_m / nb_simu, def_loss_c / nb_simu, def_loss_d / nb_simu);

	fprintf(stdout, "<p>Statistiques de recyclage (moyenne):</p><br />");
	fprintf(stdout, "<p>%s, %" APR_UINT64_T_FMT " metal, %" APR_UINT64_T_FMT " cristal,", defender->coord,
		recycl_m / nb_simu, recycl_c / nb_simu);

	pct_M_atk_str =
	    (0 == atk_loss_m) ? apr_pstrdup(attacker->pool, "no-metal-loss") : apr_psprintf(attacker->pool,
											    "%" APR_UINT64_T_FMT,
											    100UL * recycl_m / atk_loss_m);
	pct_C_atk_str =
	    (0 == atk_loss_c) ? apr_pstrdup(attacker->pool, "no-cristal-loss") : apr_psprintf(attacker->pool,
											      "%" APR_UINT64_T_FMT,
											      100UL * recycl_c / atk_loss_c);
	pct_M_def_str =
	    (0 == def_loss_m) ? apr_pstrdup(defender->pool, "no-metal-loss") : apr_psprintf(defender->pool,
											    "%" APR_UINT64_T_FMT,
											    100UL * recycl_m / def_loss_m);
	pct_C_def_str =
	    (0 == def_loss_c) ? apr_pstrdup(defender->pool, "no-cristal-loss") : apr_psprintf(defender->pool,
											      "%" APR_UINT64_T_FMT,
											      100UL * recycl_c / def_loss_c);
	fprintf(stdout, "%s %%age_M_atk, %s %%age_C_atk, %s %%age_M_def, %s %%age_C_def, ", pct_M_atk_str, pct_C_atk_str,
		pct_M_def_str, pct_C_def_str);
	fprintf(stdout, "%" APR_UINT64_T_FMT " recycleurs</p><br />",
		(recycl_m + recycl_c) / (nb_simu * os_conf_get_ship_capacity(conf, REC)));
    }
    else {
	fprintf(stdout, "\nplayer,coord,metal,cristal,deut,");
	for (j = 0; j < ITEM_END; j++) {
	    fprintf(stdout, "%s,", os_conf_get_shortname(conf, j));
	}
	fprintf(stdout, "victory,draw,round,metal_loss,cristal_loss,deut_loss,deut_consumed");

	fprintf(stdout, "\nattacker,%s,0,0,0,", attacker->coord);
	for (j = 0; j < ITEM_END; j++) {
	    fprintf(stdout, "%u,", atk_avg[j] / nb_simu);
	}
	fprintf(stdout, "%u,%u,%u,", nb_atk_vict, (nb_simu - (nb_atk_vict + nb_def_vict)), nb_round / nb_simu);
	fprintf(stdout, "%" APR_UINT64_T_FMT ",%" APR_UINT64_T_FMT ",%" APR_UINT64_T_FMT ",%u", atk_loss_m / nb_simu,
		atk_loss_c / nb_simu, atk_loss_d / nb_simu, os_fleet_consumption(attacker, distance, &flight_time));

	fprintf(stdout, "\ndefender,%s,%u,%u,%u,", defender->coord, defender->metal, defender->cristal, defender->deut);
	for (j = 0; j < ITEM_END; j++) {
	    fprintf(stdout, "%u,", def_avg[j] / nb_simu);
	}
	fprintf(stdout, "%u,%u,%u,", nb_def_vict, (nb_simu - (nb_atk_vict + nb_def_vict)), nb_round / nb_simu);
	fprintf(stdout, "%" APR_UINT64_T_FMT ",%" APR_UINT64_T_FMT ",%" APR_UINT64_T_FMT ",0", def_loss_m / nb_simu,
		def_loss_c / nb_simu, def_loss_d / nb_simu);

	fprintf(stdout, "\n\nRecycling statistics (average):");
	fprintf(stdout, "\nrecycl,coord,metal,cristal,%%age_M_atk,%%age_C_atk,%%age_M_def,%%age_C_def,nb_recycler");
	fprintf(stdout, "\nrecycl,%s,%" APR_UINT64_T_FMT ",%" APR_UINT64_T_FMT ",", defender->coord, recycl_m / nb_simu,
		recycl_c / nb_simu);

	pct_M_atk_str =
	    (0 == atk_loss_m) ? apr_pstrdup(attacker->pool, "no-metal-loss") : apr_psprintf(attacker->pool,
											    "%" APR_UINT64_T_FMT,
											    100UL * recycl_m / atk_loss_m);
	pct_C_atk_str =
	    (0 == atk_loss_c) ? apr_pstrdup(attacker->pool, "no-cristal-loss") : apr_psprintf(attacker->pool,
											      "%" APR_UINT64_T_FMT,
											      100UL * recycl_c / atk_loss_c);
	pct_M_def_str =
	    (0 == def_loss_m) ? apr_pstrdup(defender->pool, "no-metal-loss") : apr_psprintf(defender->pool,
											    "%" APR_UINT64_T_FMT,
											    100UL * recycl_m / def_loss_m);
	pct_C_def_str =
	    (0 == def_loss_c) ? apr_pstrdup(defender->pool, "no-cristal-loss") : apr_psprintf(defender->pool,
											      "%" APR_UINT64_T_FMT,
											      100 * recycl_c / def_loss_c);
	fprintf(stdout, "%s,%s,%s,%s,", pct_M_atk_str, pct_C_atk_str, pct_M_def_str, pct_C_def_str);
	fprintf(stdout, "%" APR_UINT64_T_FMT, (recycl_m + recycl_c) / (nb_simu * os_conf_get_ship_capacity(conf, REC)));

	fprintf(stdout, "\n\nAdditional statistics Best/Worst:");
	fprintf(stdout, "\nplayer,coord,metal,cristal,deut,");
	for (j = 0; j < ITEM_END; j++) {
	    fprintf(stdout, "%s%s", os_conf_get_shortname(conf, j), ((ITEM_END - 1) == j) ? "" : ",");
	}
	fprintf(stdout, "\nbest_attacker,%s,0,0,0,", attacker->coord);
	for (j = 0; j < ITEM_END; j++) {
	    fprintf(stdout, "%u%s", atk_bst[j], ((ITEM_END - 1) == j) ? "" : ",");
	}
	fprintf(stdout, "\nbest_defender,%s,%u,%u,%u,", defender->coord, defender->metal, defender->cristal, defender->deut);
	for (j = 0; j < ITEM_END; j++) {
	    fprintf(stdout, "%u%s", def_bst[j], ((ITEM_END - 1) == j) ? "" : ",");
	}
	fprintf(stdout, "\nworst_attacker,%s,0,0,0,", attacker->coord);
	for (j = 0; j < ITEM_END; j++) {
	    fprintf(stdout, "%u%s", atk_wrst[j], ((ITEM_END - 1) == j) ? "" : ",");
	}
	fprintf(stdout, "\nworst_defender,%s,%u,%u,%u,", defender->coord, defender->metal, defender->cristal,
		defender->deut);
	for (j = 0; j < ITEM_END; j++) {
	    fprintf(stdout, "%u%s", def_wrst[j], ((ITEM_END - 1) == j) ? "" : ",");
	}
	fprintf(stdout, "\n");
    }
}

static const unsigned int item_bitmask[ITEM_END] = {
    0x00000001,			/* PT */
    0x00000002,			/* GT */
    0x00000004,			/* CLE */
    0x00000008,			/* CLO */
    0x00000010,			/* CR */
    0x00000020,			/* VB */
    0x00000040,			/* VC */
    0x00000080,			/* REC */
    0x00000100,			/* SE */
    0x00000200,			/* BB */
    0x00000400,			/* SAT */
    0x00000800,			/* DEST */
    0x00001000,			/* EDLM */
    0x00002000,			/* TRAC */
    0x00004000,			/* LM */
    0x00008000,			/* ALLE */
    0x00010000,			/* ALLO */
    0x00020000,			/* CG */
    0x00040000,			/* AAI */
    0x00080000,			/* LP */
    0x00100000,			/* PB */
    0x00200000			/* GB */
};

#define	BUFFER_FLEET_DEF_FULL (0x00000000)
#define	BUFFER_FLEET_ATK_FULL (item_bitmask[SAT])
#define BUFFER_FLEET_ATK_NORMAL (item_bitmask[PT]|item_bitmask[GT]|item_bitmask[VC]|item_bitmask[REC]|item_bitmask[SE]|item_bitmask[SAT]|item_bitmask[EDLM])
#define BUFFER_FLEET_DEF_NORMAL (item_bitmask[PT]|item_bitmask[GT]|item_bitmask[VC]|item_bitmask[REC]|item_bitmask[SE]|item_bitmask[SAT])
#define BUFFER_FLEET_ATK_NOMISSILE (item_bitmask[LM]|item_bitmask[ALLE]|item_bitmask[ALLO]|item_bitmask[CG]|item_bitmask[AAI]|item_bitmask[LP]|item_bitmask[PB]|item_bitmask[GB])
#define BUFFER_FLEET_SCRIPT (item_bitmask[VC]|item_bitmask[REC]|item_bitmask[SE]|item_bitmask[SAT]|item_bitmask[EDLM]|BUFFER_FLEET_ATK_NOMISSILE)
#define BUFFER_FLEET_DEF_BIGDEF (item_bitmask[PT]|item_bitmask[GT]|item_bitmask[CLE]|item_bitmask[CLO]|item_bitmask[CR]|item_bitmask[VC]|item_bitmask[REC]|item_bitmask[SE]|item_bitmask[SAT])

struct os_fleet_genetic_ctx_t
{
    unsigned int initial_repartition[ITEM_END];	/* Used to store already buyed fleet , to remove the price from guessed */
    unsigned int max_missiles[ITEM_END];
    unsigned int buffer_fleet;	/* binary mask with forbidden ship set to one */
    apr_uint64_t metl_recycled;
    apr_uint64_t crst_recycled;
    const os_conf_t *conf;
    os_fleet_t *fleet;		/* Ennemy fleet */
    apr_pool_t *pool;
    float max_price;
    unsigned int max_ship;
    unsigned int distance;
    unsigned int wave_time;
    unsigned int max_flight_time;
    unsigned int first_fleet_set;	/* 0 if we have a repartition, 1 after the first random generation that won't be random but copy of it */
    unsigned char attack;
    unsigned char shield;
    unsigned char structr;
    unsigned char combustion;
    unsigned char impulsion;
    unsigned char hyperespace;
    unsigned char mode;		/* binary mask of OS_MODE_HUMAN/OS_MODE_PERL/OS_MODE_HTML/OS_MODE_NO_LOSS/OS_MODE_NO_RECYCLING/... see os_fleet.h */
};

typedef struct os_fleet_genetic_ctx_t os_fleet_genetic_ctx_t;

static void os_fleet_ga_display(void *rec, const void *chromosome)
{
    os_fleet_genetic_ctx_t *ctx = rec;
    const os_fleet_t *fleet = chromosome;
    const os_fleet_t *attacker, *defender;
    unsigned int deut_consumed, free_capacity, flight_time;
    long stealsum, metal_stolen, cristal_stolen, deut_stolen;	/* can be negative */
    int is_first = 1, display_mip = 0;
    enum Item_enum j;

    if (ctx->mode & OS_MODE_PERL) {
	defender = ctx->fleet;
	attacker = chromosome;

	for (j = 0, free_capacity = 0; j < LM; j++)
	    free_capacity += (attacker->current_repartition[j] * attacker->os_ship[j].capacity);

	/* Deut consummed added to total lost */
	deut_consumed = os_fleet_consumption(attacker, ctx->distance, &flight_time);
	free_capacity -= deut_consumed;

	metal_stolen = (long) ((defender->metal >> 1) - attacker->metl_lost);
	cristal_stolen = (long) ((defender->cristal >> 1) - attacker->crst_lost);
	deut_stolen = (long) ((defender->deut >> 1) - attacker->deut_lost);

	stealsum = metal_stolen + cristal_stolen + deut_stolen;
	/*
	 * this < insure that stealsum is positive
	 * Thus metal_stolen, cristal_stolen are always positives.
	 */
	if (free_capacity < stealsum) {
	    float capacity_divider;

	    capacity_divider = (float) free_capacity / (float) stealsum;
	    metal_stolen = ((float) metal_stolen * capacity_divider);
	    cristal_stolen = ((float) cristal_stolen * capacity_divider);
	    deut_stolen = (long) ((float) deut_stolen * capacity_divider) - (long) deut_consumed;
	}
	else {
	    deut_stolen -= deut_consumed;
	}

	fprintf(stdout, "\nmetal,cristal,deut(less_consumed),recyclable_metal,recyclable_cristal,flight_time,deut_consum,");
	for (j = PT, is_first = 1; j < LM; j++) {
	    fprintf(stdout, "%s%s", (1 == is_first) ? "" : ",", (fleet->os_ship)[j].shortname);
	    is_first = 0;
	}
	fprintf(stdout, "\n%li,%li,%li,%" APR_UINT64_T_FMT ",%" APR_UINT64_T_FMT ",%u,%u,", metal_stolen, cristal_stolen,
		deut_stolen, attacker->metl_recycled, attacker->crst_recycled, flight_time, deut_consumed);
	for (j = PT, is_first = 1; j < LM; j++) {
	    fprintf(stdout, "%s%u", (1 == is_first) ? "" : ",", fleet->initial_repartition[j]);
	    is_first = 0;
	}
	fprintf(stdout, "\n\n");
    }
    else {
	if (ctx->mode & OS_MODE_HTML)
	    fprintf(stdout, "Flotte proposée:<br />");
	for (j = PT; j < ITEM_END; j++) {
	    if (0 != fleet->initial_repartition[j]) {
		if ((ATK_FLT == fleet->type) && (j >= LM) && !display_mip) {
		    fprintf(stdout, "%sMISSILE VS. ", (1 == is_first) ? "" : ", ");
		    display_mip = 1;
		    is_first = 1;
		}
		fprintf(stdout, "%s%s:%u", (1 == is_first) ? "" : ", ", (fleet->os_ship)[j].shortname,
			fleet->initial_repartition[j]);
		is_first = 0;
	    }
	}

	if (ctx->mode & OS_MODE_HTML)
	    fprintf(stdout, "<br />");

	if (!(ctx->mode & OS_MODE_NO_INVEST)) {
	    fprintf(stdout, "\n");
	    fprintf(stdout, "\tMetal Invested:%" APR_UINT64_T_FMT, fleet->ship_metal);
	    fprintf(stdout, ", Cristal Invested:%" APR_UINT64_T_FMT, fleet->ship_cristal);
	    fprintf(stdout, ", Deut Invested:%" APR_UINT64_T_FMT, fleet->ship_deut);
	}
	fprintf(stdout, "\n");
	if (ctx->mode & OS_MODE_HTML)
	    fprintf(stdout, "<br /><br />");
    }

    fflush(stdout);
}

static inline int os_fleet_compute_fitness_stats(os_fleet_t *adversary, os_fleet_t *own)
{
    unsigned char j;

    /* Must not lose */
    if (0 == own->ship_count)
	return 0;

    /* Ennemy must lose */
    if (0 != adversary->ship_count)
	return 0;

    own->metl_lost = 0ULL;
    own->crst_lost = 0ULL;
    own->deut_lost = 0ULL;
    for (j = '\0'; j < ITEM_END; j++) {
	own->metl_lost +=
	    (apr_uint64_t) ((apr_uint64_t) own->initial_repartition[j] -
			    (apr_uint64_t) own->current_repartition[j]) * (apr_uint64_t) own->os_ship[j].metl_price;
	own->crst_lost +=
	    (apr_uint64_t) ((apr_uint64_t) own->initial_repartition[j] -
			    (apr_uint64_t) own->current_repartition[j]) * (apr_uint64_t) own->os_ship[j].crst_price;
	own->deut_lost +=
	    (apr_uint64_t) ((apr_uint64_t) own->initial_repartition[j] -
			    (apr_uint64_t) own->current_repartition[j]) * (apr_uint64_t) own->os_ship[j].deut_price;
    }
    own->metl_lost = (0ULL == own->metl_lost) ? 1 : own->metl_lost;
    own->crst_lost = (0ULL == own->crst_lost) ? 1 : own->crst_lost;
    own->deut_lost = (0ULL == own->deut_lost) ? 1 : own->deut_lost;

    return 1;
}

#define FITNESS_NB_SIM 32LLU

static float os_fleet_ga_fitness(void *rec, void *chromosome)
{
    os_fleet_genetic_ctx_t *ctx = rec;
    os_fleet_t *attacker, *defender, *own, *adversary;
    unsigned int deut_consumed = 0, free_capacity, flight_time;
    apr_uint64_t metal_stolen = 0, cristal_stolen = 0, divider, div_acc = 0, stealsum, wave_time_divider =
	1, metl_lost, crst_lost, deut_lost;
    apr_uint64_t current_repartition_avg[ITEM_END];
    apr_int64_t deut_stolen = 0, numerator, num_acc = 0;	/* can be negatives */
    os_fleet_t ctx_fleet;
    float ratio;
    int j, k;

    /* 
     * XXX : this ctx->fleet passed like this may be the multithread violation
     * As a workaround, we will temporarly copy new values.
     */
    memcpy(&ctx_fleet, ctx->fleet, sizeof(os_fleet_t));
    memset(current_repartition_avg, 0, ITEM_END * sizeof(apr_uint64_t));

    /* The ships_hit_table is a pointer, so the memcpy does not alloc a new one */
    ctx_fleet.ships_hit_table = malloc(ctx_fleet.ship_initial_count * sizeof(struct os_battle_ship_t));

    if (ctx->fleet->type == ATK_FLT) {
	adversary = attacker = &ctx_fleet;
	own = defender = chromosome;
    }
    else {
	adversary = defender = &ctx_fleet;
	own = attacker = chromosome;

	/* Deut consummed added to total lost */
	deut_consumed = os_fleet_consumption(attacker, ctx->distance, &flight_time);
	if ((0UL != ctx->max_flight_time) && (flight_time > ctx->max_flight_time)) {
	    free(ctx_fleet.ships_hit_table);
	    return -FLT_MAX;
	}
	if ((0UL != ctx->wave_time) && (flight_time > ctx->wave_time)) {
	    for (wave_time_divider = 1; (ctx->wave_time * wave_time_divider) < flight_time; wave_time_divider++);
	}

    }

    own->metl_recycled = 0;
    own->crst_recycled = 0;
    metl_lost = crst_lost = deut_lost = 0;

    for (j = 0, own->ship_metal = 1, own->ship_cristal = 1, own->ship_deut = 1; j < ITEM_END; j++) {
	if (own->initial_repartition[j] > ctx->initial_repartition[j]) {
	    /* No investment mode */
	    if (ctx->mode & OS_MODE_NO_INVEST) {
		free(ctx_fleet.ships_hit_table);
		return -FLT_MAX;
	    }

	    own->ship_metal +=
		(apr_uint64_t) ((apr_uint64_t) own->initial_repartition[j] -
				(apr_uint64_t) ctx->initial_repartition[j]) * (apr_uint64_t) own->os_ship[j].metl_price;
	    own->ship_cristal +=
		(apr_uint64_t) ((apr_uint64_t) own->initial_repartition[j] -
				(apr_uint64_t) ctx->initial_repartition[j]) * (apr_uint64_t) own->os_ship[j].crst_price;
	    own->ship_deut +=
		(apr_uint64_t) ((apr_uint64_t) own->initial_repartition[j] -
				(apr_uint64_t) ctx->initial_repartition[j]) * (apr_uint64_t) own->os_ship[j].deut_price;
	}
    }

    for (k = 0; k < FITNESS_NB_SIM; k++) {
	os_fleet_onebattle(attacker, defender, ctx->conf);

	if (0 == os_fleet_compute_fitness_stats(adversary, own)) {
	    free(ctx_fleet.ships_hit_table);
	    return -FLT_MAX;
	}

	if (own == defender) {
	    numerator =
		META_RATIO * (ctx->metl_recycled - defender->metl_lost) + CRST_RATIO * (ctx->crst_recycled -
											defender->crst_lost) -
		DEUT_RATIO * (defender->deut_lost);
	}
	else {
	    if ((ctx->mode & OS_MODE_NO_LOSS)
		&& (1 != (attacker->metl_lost * attacker->crst_lost * attacker->deut_lost))) {
		/* Don't tolerate a loss */
		free(ctx_fleet.ships_hit_table);
		return -FLT_MAX;
	    }
	    else {
		metl_lost += attacker->metl_lost;
		crst_lost += attacker->crst_lost;
		deut_lost += attacker->deut_lost;

		/* include our fleet in the recycling */
		if (!(ctx->mode & OS_MODE_NO_RECYCLING)) {
		    attacker->metl_recycled += ctx->metl_recycled + attacker->metl_lost * 0.30f;
		    attacker->crst_recycled += ctx->crst_recycled + attacker->crst_lost * 0.30f;
		    attacker->metl_lost *= 0.70f;
		    attacker->crst_lost *= 0.70f;
		}
	    }

	    /* XXX refactor this, if we are in no-loss mode no need to recalculate */
	    for (j = 0, free_capacity = 0; j < LM; j++)
		free_capacity += (attacker->current_repartition[j] * attacker->os_ship[j].capacity);

	    if (free_capacity >= deut_consumed) {
		free_capacity -= deut_consumed;

		metal_stolen = defender->metal >> 1;
		cristal_stolen = defender->cristal >> 1;
		deut_stolen = defender->deut >> 1;

		stealsum = metal_stolen + cristal_stolen + deut_stolen;
		if (free_capacity < stealsum) {
		    float capacity_divider;

		    capacity_divider = (float) free_capacity / (float) stealsum;
		    metal_stolen = ((float) metal_stolen * capacity_divider);
		    cristal_stolen = ((float) cristal_stolen * capacity_divider);
		    deut_stolen = ((float) deut_stolen * capacity_divider) - deut_consumed;
		}
		else {
		    deut_stolen -= deut_consumed;
		}

	    }
	    else if (ctx->mode & OS_MODE_NO_RECYCLING) {
		/* Fleet can take nothing. Thus, if no recycling, just give up */
		free(ctx_fleet.ships_hit_table);
		return -FLT_MAX;
	    }
	    else {
		metal_stolen = 0;
		cristal_stolen = 0;
		deut_stolen = 0;
	    }
	    if (!(ctx->mode & OS_MODE_NO_RECYCLING)) {
		metal_stolen += ctx->metl_recycled;
		cristal_stolen += ctx->crst_recycled;
	    }

	    /* if we are here with acceptable loss else ... it is important for the following (1) */
	    numerator =
		META_RATIO * (metal_stolen - attacker->metl_lost) +
		CRST_RATIO * (cristal_stolen - attacker->crst_lost) + DEUT_RATIO * (deut_stolen - attacker->deut_lost);

	    /*DEBUG_DBG("numerator: %"APR_INT64_T_FMT", %"APR_UINT64_T_FMT" + %"APR_UINT64_T_FMT" + %lu", numerator, metal_stolen - attacker->metl_lost, cristal_stolen - attacker->crst_lost, 2 * deut_stolen - attacker->deut_lost); */
	    /*
	     * (1) if loss acceptable + perl output i'm pretty sure I was
	     * invoked by a script that don't want to lose many ships.
	     */
	    if ((ctx->mode & OS_MODE_PERL) && (numerator < 0)) {
		free(ctx_fleet.ships_hit_table);
		return -FLT_MAX;
	    }
	}

	if ((own == defender) || ((numerator > 0.0f) && !(ctx->mode & OS_MODE_NO_INVEST))) {
	    /* No need do divide if numerator is negative */
	    divider = META_RATIO * own->ship_metal + CRST_RATIO * own->ship_cristal + DEUT_RATIO * own->ship_deut;
	}
	else {
	    divider = 1LLU;
	}
	divider *= wave_time_divider;
	num_acc += numerator;
	div_acc += divider;
    }

    own->metl_recycled /= FITNESS_NB_SIM;
    own->crst_recycled /= FITNESS_NB_SIM;

    own->metl_lost = metl_lost / FITNESS_NB_SIM;
    own->crst_lost = crst_lost / FITNESS_NB_SIM;
    own->deut_lost = deut_lost / FITNESS_NB_SIM;

    if (!(ctx->mode & OS_MODE_NO_LOSS)) {
	for (j = PT; j < ITEM_END; j++) {
	    own->current_repartition[j] = current_repartition_avg[j] / FITNESS_NB_SIM;
	}
    }

    divider *= FITNESS_NB_SIM;
    ratio = ((float) num_acc / (float) div_acc);
    /*DEBUG_DBG("Returning %.2f = %"APR_INT64_T_FMT" / %"APR_UINT64_T_FMT" pt:%lu\n", ratio, num_acc, div_acc, (apr_uint64_t) own->initial_repartition[PT]); */
    free(ctx_fleet.ships_hit_table);

    return ratio;
}

static void os_fleet_ga_allocat(void *rec, apr_pool_t *pool, void **chromosome)
{
    os_fleet_genetic_ctx_t *ctx = rec;
    os_fleet_t *fleet;

    if (ctx->fleet->type == ATK_FLT) {
	/* Must allocate a defender */
	fleet = os_fleet_make(pool, DEF_FLT);
	fleet->limit = GB + 1;
    }
    else {
	/* Must allocate an attacker */
	fleet = os_fleet_make(pool, ATK_FLT);
	fleet->limit = LM;
    }

    fleet->attack = ctx->attack;
    fleet->shield = ctx->shield;
    fleet->structr = ctx->structr;
    fleet->combustion = ctx->combustion;
    fleet->impulsion = ctx->impulsion;
    fleet->hyperespace = ctx->hyperespace;
    /*for (i = PT; i < fleet->limit; i++) { */
    /*(fleet->os_ship)[i].shield_points = */
    /*os_conf_get_shield_points(ctx->conf, i) * (1.0f + (0.1f * (float) fleet->shield)); */
    /*(fleet->os_ship)[i].shield_points_percent = 100.0f / (fleet->os_ship)[i].shield_points; */
    /*(fleet->os_ship)[i].attack_value = os_conf_get_attack_value(ctx->conf, i) * (1.0f + (0.1f * (float) fleet->attack)); */
    /*(fleet->os_ship)[i].structure_points = */
    /*os_conf_get_structure_points(ctx->conf, i) * (1.0f + (0.1f * (float) fleet->structr)) / 10.0f; */
    /*(fleet->os_ship)[i].structure_points_percent = 100.0f / (fleet->os_ship)[i].structure_points; */
    /*(fleet->os_ship)[i].price = os_conf_get_ship_price(ctx->conf, i); */
    /*(fleet->os_ship)[i].shortname = os_conf_get_shortname(ctx->conf, i); */
    /*(fleet->os_ship)[i].metl_price = os_conf_get_ship_metal(ctx->conf, i); */
    /*(fleet->os_ship)[i].crst_price = os_conf_get_ship_cristal(ctx->conf, i); */
    /*(fleet->os_ship)[i].deut_price = os_conf_get_ship_deut(ctx->conf, i); */
    /*} */
    os_fleet_parse(fleet, ctx->conf);

    fleet->ships_hit_table = apr_palloc(pool, ctx->max_ship * sizeof(struct os_battle_ship_t));
    *chromosome = fleet;
}

static inline void os_fleet_reduce_to_max(os_fleet_genetic_ctx_t *ctx, os_fleet_t *fleet, unsigned int max, float max_price)
{
    unsigned int less;
    float divider_price, divider_nmb, divider;
    int j;
    enum Item_enum i;

    /* Average method */
    divider_nmb = (float) max / (float) fleet->ship_initial_count;
    fleet->ship_initial_count = 0;

    for (i = PT; i < PB; i++) {
	divider_price = max_price / (fleet->os_ship[i].price * (float) fleet->initial_repartition[i]);	/* * (float) (LM - 6)); */
	divider = MIN(divider_price, divider_nmb);
	if (divider < 1.0f) {
	    fleet->initial_repartition[i] = (float) fleet->initial_repartition[i] * divider;
	}
	if ((ATK_FLT != fleet->type) || (i < LM)) {
	    fleet->ship_initial_count += fleet->initial_repartition[i];
	}
	else {
	    if (fleet->initial_repartition[i] > ctx->max_missiles[i]) {
		fleet->initial_repartition[i] = ctx->max_missiles[i];
	    }
	}
    }

    if (fleet->initial_repartition[PB] >= 1) {
	fleet->initial_repartition[PB] = 1UL;
	if (DEF_FLT == fleet->type) {
	    fleet->ship_initial_count += 1;
	}
	else {
	    fleet->initial_repartition[PB] = ctx->max_missiles[PB];
	}
    }
    if (fleet->initial_repartition[GB] >= 1) {
	fleet->initial_repartition[GB] = 1UL;
	if (DEF_FLT == fleet->type) {
	    fleet->ship_initial_count += 1;
	}
	else {
	    fleet->initial_repartition[GB] = ctx->max_missiles[GB];
	}
    }

    for (j = 0; fleet->ship_initial_count > max; j++) {
	if (0 == fleet->initial_repartition[j % fleet->limit])
	    continue;
	less = my_rand(fleet->initial_repartition[j % fleet->limit]);
	fleet->initial_repartition[j % fleet->limit] -= less;
	fleet->ship_initial_count -= less;
    }
}

static void os_fleet_ga_randomz(void *rec, void *chromosome)
{
    os_fleet_genetic_ctx_t *ctx = rec;
    os_fleet_t *fleet = chromosome;
    unsigned int max;
    enum Item_enum i;

    if (ITEM_END > ctx->first_fleet_set) {
	for (i = ctx->first_fleet_set; i < ITEM_END; i++) {
	    fleet->initial_repartition[i] = ctx->initial_repartition[i];
	    /* MIP don't count as ship */
	    if ((ATK_FLT != fleet->type) || (i < LM))
		fleet->ship_initial_count += fleet->initial_repartition[i];
	}
	/*
	 * XXX
	 * an assignement of ctx in a chromosome function may break a
	 * possibility of multithreading, but, we will begin by paralellizing
	 * only fitness, so random is not an issue here
	 */
	ctx->first_fleet_set += 1;
    }
    else {
	/* 1+ because it must not be 0 */
	max = 1 + my_rand(ctx->max_ship);
	for (i = PT; i < ITEM_END; i++) {
	    /* 2nd cond: We want to randomly try some fleet with no ship of one type */
	    if ((item_bitmask[i] & ctx->buffer_fleet) || (my_randf() > 0.85f)) {
		fleet->initial_repartition[i] = 0UL;
		continue;
	    }
	    /* If we are in no invest mode, only purpose sub fleet of ctx->initial_repartition */
	    if (ctx->mode & OS_MODE_NO_INVEST) {
		fleet->initial_repartition[i] = my_randf() * ctx->initial_repartition[i];
	    }
	    else {
		fleet->initial_repartition[i] = my_rand(max);
	    }

	    /* MIP don't count as ship */
	    if ((ATK_FLT != fleet->type) || (i < LM))
		fleet->ship_initial_count += fleet->initial_repartition[i];
	}
	os_fleet_reduce_to_max(ctx, fleet, max, ctx->max_price);
    }
}

static void os_fleet_ga_crossvr(void *rec, float crossover_p, const void *father, void *mother)
{
    const os_fleet_t *fleet1 = father;
    os_fleet_t *fleet2 = mother;
    os_fleet_genetic_ctx_t *ctx = rec;
    enum Item_enum i;

    fleet2->ship_initial_count = 0;
    for (i = PT; i < ITEM_END; i++) {
	if (crossover_p >= my_randf()) {
	    /* methods of crossover: take something between father value and mother value */
	    float average;

	    average = my_randf();
	    fleet2->initial_repartition[i] =
		average * fleet2->initial_repartition[i] + (1.0f - average) * fleet1->initial_repartition[i];
	}
	if ((ATK_FLT != fleet2->type) || (i < LM))
	    fleet2->ship_initial_count += fleet2->initial_repartition[i];
    }
    os_fleet_reduce_to_max(ctx, fleet2, ctx->max_ship, ctx->max_price);
}

static void os_fleet_ga_mutation(void *rec, float mutation_p, void *chromosome)
{
    os_fleet_t *fleet = chromosome;
    os_fleet_genetic_ctx_t *ctx = rec;
    float percentage, randval;
    enum Item_enum i;

    fleet->ship_initial_count = 0;
    for (i = PT; i < ITEM_END; i++) {
	if (item_bitmask[i] & ctx->buffer_fleet)
	    continue;
	randval = (-1.0f + (2.0f * my_randf()));
	percentage = 1.0f + (mutation_p * randval);
	if ((0 == fleet->initial_repartition[i]) && (randval > 1.9f)) {
	    fleet->initial_repartition[i] = 1UL;
	}
	else if (randval < 0.1f) {
	    fleet->initial_repartition[i] = 0UL;
	}
	else {
	    /* no modification possible */
	    if (0 == fleet->initial_repartition[i])
		continue;
	    if (my_randf() > 0.5f) {
		/* 1 time / 2 this mutation affect 2 ship types */
		float price_inc;
		int rand_idx;

		price_inc = fleet->os_ship[i].price * (float) fleet->initial_repartition[i];
		fleet->initial_repartition[i] = (float) fleet->initial_repartition[i] * percentage;
		price_inc = (fleet->os_ship[i].price * (float) fleet->initial_repartition[i]) - price_inc;
		/* Report this mutation to another ship type */
		for (rand_idx = my_rand((unsigned int) ITEM_END); item_bitmask[rand_idx] & ctx->buffer_fleet;
		     rand_idx = my_rand((unsigned int) ITEM_END));
		if (((float) fleet->initial_repartition[rand_idx] - (price_inc / fleet->os_ship[rand_idx].price)) < 0.0f)
		    fleet->initial_repartition[rand_idx] = 0;
		else
		    fleet->initial_repartition[rand_idx] =
			(float) fleet->initial_repartition[rand_idx] - (price_inc / fleet->os_ship[rand_idx].price);
	    }
	    else {
		/* 1 time / 2 this mutation affect only one ship types */
		fleet->initial_repartition[i] = (float) fleet->initial_repartition[i] * percentage;
	    }
	}

	if ((ATK_FLT != fleet->type) || (i < LM))
	    fleet->ship_initial_count += fleet->initial_repartition[i];
    }

    if (DEF_FLT == fleet->type) {
	if (fleet->initial_repartition[PB] >= 1) {
	    fleet->initial_repartition[PB] = 1UL;
	    fleet->ship_initial_count += 1;
	}
	if (fleet->initial_repartition[GB] >= 1) {
	    fleet->initial_repartition[GB] = 1UL;
	    fleet->ship_initial_count += 1;
	}
    }

    os_fleet_reduce_to_max(ctx, fleet, ctx->max_ship, ctx->max_price);
}

static enum Item_enum find_cheapest_ship(unsigned int *initial_repartition, unsigned int buffer_fleet_mask)
{
    enum Item_enum i;
    return i;
}

extern void os_fleet_find_cheapest_winner(os_fleet_t *attacker, os_fleet_t *defender, const os_conf_t *conf,
					  enum genetic_algorithm_mask mask, unsigned int inactivity_timeout,
					  unsigned int fixed_timeout, unsigned int flight_time, unsigned int wave_time,
					  unsigned char mode, unsigned int nb_cpu)
{
    os_fleet_genetic_ctx_t ctx;
    os_fleet_t *toguess, *tofight;
    napr_galife_t *ga;
    apr_pool_t *ga_pool;
    FILE *meminfo;
    unsigned int memfree = 0UL, our_nb_ships, nb_individuals;
    float defender_price, attacker_price, our_price;
    enum Item_enum i;

    apr_pool_create(&ga_pool, attacker->pool);
    my_srand();

    ctx.max_flight_time = flight_time;
    ctx.wave_time = wave_time;
    memset(ctx.max_missiles, 0UL, ITEM_END);
    if (attacker->guess_mode) {
	toguess = attacker;
	tofight = defender;
    }
    else {
	toguess = defender;
	tofight = attacker;
    }
    ctx.mode = mode;
    switch (mask) {
    case FULL:
	if (toguess == defender)
	    ctx.buffer_fleet = BUFFER_FLEET_DEF_FULL;
	else
	    ctx.buffer_fleet = BUFFER_FLEET_ATK_FULL;
	break;
    case NO_MISSILE:
	ctx.buffer_fleet = BUFFER_FLEET_ATK_NORMAL | BUFFER_FLEET_ATK_NOMISSILE;
	break;
    case SCRIPT:
	ctx.buffer_fleet = BUFFER_FLEET_SCRIPT;
	break;
    case NORMAL:
	if (toguess == defender)
	    ctx.buffer_fleet = BUFFER_FLEET_DEF_NORMAL;
	else
	    ctx.buffer_fleet = BUFFER_FLEET_ATK_NORMAL;
	break;
    case DEF:
	if (toguess == defender) {
	    ctx.buffer_fleet = BUFFER_FLEET_DEF_BIGDEF;
	}
	else {
	    DEBUG_DBG("No meaning of the def mask in atk guess.");
	    ctx.buffer_fleet = BUFFER_FLEET_ATK_NORMAL;
	}
	break;
    }

    if (attacker->guess_mode) {
	/* Evaluate price & recycling of the defending fleet */
	ctx.metl_recycled = 0;
	ctx.crst_recycled = 0;
	for (i = PT, defender_price = 0.0f; i < ITEM_END; i++) {
	    defender_price += defender->initial_repartition[i] * defender->os_ship[i].price;
	    if (i < LM) {
		ctx.metl_recycled += (defender->initial_repartition[i] * defender->os_ship[i].metl_price);
		ctx.crst_recycled += (defender->initial_repartition[i] * defender->os_ship[i].crst_price);
	    }
	    else {
		if (0UL != defender->initial_repartition[i]) {
		    ctx.max_missiles[i] = defender->mit + ((defender->os_ship[i].structure_points * defender->initial_repartition[i])	/* Structure */
							   /(12000.0f *
							     (1.0f +
							      attacker->attack / 10.0f)) /* damage */ +0.99999999999999);
		    DEBUG_DBG("%u missiles max to kill %u %s", ctx.max_missiles[i], defender->initial_repartition[i],
			      (defender->os_ship)[i].shortname);
		}
		else {
		    ctx.max_missiles[i] = 0UL;
		}
	    }
	}
	ctx.max_price = 3.5f * defender_price;
	ctx.max_ship = (unsigned int) (ctx.max_price / defender->os_ship[CLE].price);
	ctx.combustion = toguess->combustion;
	ctx.impulsion = toguess->impulsion;
	ctx.hyperespace = toguess->hyperespace;
    }
    else if (defender->guess_mode) {
	ctx.max_ship = attacker->ship_initial_count;
	/* Evaluate price & recycling of the attacking fleet */
	for (i = PT, attacker_price = 0.0f; i < LM; i++) {
	    attacker_price += attacker->initial_repartition[i] * attacker->os_ship[i].price;
	    ctx.metl_recycled += (attacker->initial_repartition[i] * attacker->os_ship[i].metl_price);
	    ctx.crst_recycled += (attacker->initial_repartition[i] * attacker->os_ship[i].crst_price);
	}
	ctx.max_price = 1.5f * attacker_price;
	ctx.max_ship = (unsigned int) (ctx.max_price / attacker->os_ship[LM].price);
    }
    else {
	DEBUG_ERR("Fleet repartition set for both player, mode not supported. RTFM.");
	return;
    }

    for (i = 0, our_nb_ships = 0, our_price = 0.0f; i < ITEM_END; i++) {
	our_nb_ships += toguess->initial_repartition[i];
	our_price += toguess->initial_repartition[i] * toguess->os_ship[i].price;
    }
    ctx.max_ship = MAX(our_nb_ships, ctx.max_ship);
    ctx.max_price = MAX(our_price, ctx.max_price);
    ctx.fleet = tofight;
    ctx.attack = toguess->attack;
    ctx.shield = toguess->shield;
    ctx.structr = toguess->structr;
    if (0UL == (ctx.distance = os_fleet_distance(toguess->coord, tofight->coord))) {
	DEBUG_ERR("Can't parse one of the coordinates: %s or %s", toguess->coord, tofight->coord);
	return;
    }

    ctx.metl_recycled *= 0.30f;
    ctx.crst_recycled *= 0.30f;
    memcpy(ctx.initial_repartition, toguess->initial_repartition, ITEM_END * sizeof(unsigned int));
    ctx.first_fleet_set = 0;

    if (0 == ctx.metl_recycled)
	ctx.metl_recycled = 1;
    if (0 == ctx.crst_recycled)
	ctx.crst_recycled = 1;

    ctx.conf = conf;
    ctx.pool = ga_pool;

    /* check memory usage to tune number of individuals */
    meminfo = fopen("/proc/meminfo", "r");
    if (meminfo) {
	char line[256];
	/* Drop MemTotal */
	while (fgets(line, sizeof(line), meminfo)) {
	    if (strstr(line, "MemFree: ")) {
		memfree = strtoul(line + 9 /* strlen("MemFree: ") */ , NULL, 10);
	    }
	    else if (strstr(line, "Buffers: ")) {
		memfree += strtoul(line + 9 /* strlen("Buffers: ") */ , NULL, 10);
	    }
	    else if (strstr(line, "Cached: ")) {
		memfree += strtoul(line + 8 /* strlen("Cached: ") */ , NULL, 10);
		break;
	    }
	}
	if (fclose(meminfo)) {
	    DEBUG_ERR("Can't close proc meminfo");
	}
    }
    else {
	DEBUG_ERR("Can't read meminfo MemTotal from meminfo");
	return;
    }

    if (!memfree) {
	DEBUG_ERR("Can't read correctly memory infos.");
	return;
    }
    /* Computation of max number of individuals */
    /* Leave 10Mo for stack and other process :> */
    memfree -= (5UL * 1024UL);
    DEBUG_DBG("memfree less 5Mo : %uko, sizeof an individual: %luko", memfree,
	      (sizeof(struct os_fleet_t) + ctx.max_ship * sizeof(struct os_battle_ship_t)) / 1024UL);
    nb_individuals = (1024UL * memfree) / (sizeof(struct os_fleet_t) + ctx.max_ship * sizeof(struct os_battle_ship_t));
    nb_individuals = MIN(nb_individuals, 256UL);
    DEBUG_DBG("Plan to use %u individuals", nb_individuals);
    if (nb_individuals <= 3) {
	DEBUG_ERR("Not enough memory...");
	return;
    }

    if (APR_SUCCESS ==
	napr_galife_init(ga_pool, nb_individuals, 100000UL, inactivity_timeout, fixed_timeout, nb_cpu, &ctx,
			 os_fleet_ga_allocat, os_fleet_ga_randomz, os_fleet_ga_display, os_fleet_ga_fitness, 0.95f,
			 os_fleet_ga_crossvr, 0.5f, os_fleet_ga_mutation, &ga)) {
	if (APR_SUCCESS != ga_run(ga))
	    DEBUG_ERR("error calling ga_run");
    }
    else {
	DEBUG_ERR("error calling napr_galife_init");
    }

    apr_pool_destroy(ga_pool);
}

#define MATCH_VECTOR_SIZE 12
#define BUF_SIZE 256
#define CHK_SUBSTR(reg,str,ovector,size,name,buffer,bufsiz) { \
    int rc; \
    if(0 > (rc = pcre_copy_named_substring(reg, str, ovector, size, name, buffer, bufsiz))) { \
	DEBUG_ERR("An error occured while getting [%s]:[%i]", name, rc); \
	return 0UL; \
    } \
}

extern unsigned int os_fleet_distance(const char *fleet1, const char *fleet2)
{
    int ovector[MATCH_VECTOR_SIZE];
    char buffer[BUF_SIZE];
    const char *regex = "(?P<gal>[0-9]):(?P<sys>[0-9]*):(?P<pla>[0-9]*)";
    pcre *regexp_compiled;
    const char *errptr;
    unsigned int result;
    int rc, erroffset, gal1, sys1, pla1, gal2, sys2, pla2;

    regexp_compiled = pcre_compile(regex, PCRE_DOLLAR_ENDONLY | PCRE_DOTALL, &errptr, &erroffset, NULL);
    if (NULL != regexp_compiled) {
	rc = pcre_exec(regexp_compiled, NULL, fleet1, strlen(fleet1), 0, 0, ovector, MATCH_VECTOR_SIZE);
	if (rc < 0) {
	    DEBUG_ERR("An error occured while matching [%s] : [%i]", regex, rc);
	    return 0UL;
	}
	CHK_SUBSTR(regexp_compiled, fleet1, ovector, MATCH_VECTOR_SIZE / 3, "gal", buffer, BUF_SIZE);
	gal1 = atoi(buffer);
	CHK_SUBSTR(regexp_compiled, fleet1, ovector, MATCH_VECTOR_SIZE / 3, "sys", buffer, BUF_SIZE);
	sys1 = atoi(buffer);
	CHK_SUBSTR(regexp_compiled, fleet1, ovector, MATCH_VECTOR_SIZE / 3, "pla", buffer, BUF_SIZE);
	pla1 = atoi(buffer);
	rc = pcre_exec(regexp_compiled, NULL, fleet2, strlen(fleet2), 0, 0, ovector, MATCH_VECTOR_SIZE);
	if (rc < 0) {
	    DEBUG_ERR("An error occured while matching [%s] : [%i]", regex, rc);
	    return 0UL;
	}
	CHK_SUBSTR(regexp_compiled, fleet2, ovector, MATCH_VECTOR_SIZE / 3, "gal", buffer, BUF_SIZE);
	gal2 = atoi(buffer);
	CHK_SUBSTR(regexp_compiled, fleet2, ovector, MATCH_VECTOR_SIZE / 3, "sys", buffer, BUF_SIZE);
	sys2 = atoi(buffer);
	CHK_SUBSTR(regexp_compiled, fleet2, ovector, MATCH_VECTOR_SIZE / 3, "pla", buffer, BUF_SIZE);
	pla2 = atoi(buffer);
	if (gal2 != gal1) {
	    result = abs(gal2 - gal1) * 20000UL;
	}
	else if (sys2 != sys1) {
	    result = abs(sys2 - sys1) * 5UL * 19UL + 2700UL;
	}
	else if (pla2 - pla1) {
	    result = abs(pla2 - pla1) * 5UL + 1000UL;
	}
	else {
	    result = 5UL;
	}

    }
    else {
	DEBUG_ERR("An error occured while compiling [%s] : [%s] [%i]", regex, errptr, erroffset);
	return 0UL;
    }

    return result;
}

extern unsigned int os_fleet_consumption(const os_fleet_t *attacker, unsigned int distance, unsigned int *flight_time)
{
    unsigned int max_speed = 1000000000;
    float duration, spd, result = 0.0f;
    enum Item_enum i;

    /*DEBUG_DBG("distance: %u", distance); */
    for (i = PT; i < LM; i++) {
	/*DEBUG_DBG("speed: [%s] %u", (attacker->os_ship)[i].shortname, (attacker->os_ship)[i].speed); */
	if (attacker->initial_repartition[i] > 0) {
	    max_speed = MIN(max_speed, (attacker->os_ship)[i].speed);
	    /*DEBUG_DBG("speed: %u", (attacker->os_ship)[i].speed); */
	}
    }

    /*DEBUG_DBG("max_speed: %u", max_speed); */
    duration = ((35000.0f / 10.0f * sqrt(distance * 10.0f / (float) max_speed) + 10.0f));
    *flight_time = (unsigned int) duration;
    /*DEBUG_DBG("duration: %f", duration); */
    for (i = PT; i < LM; i++) {
	if (attacker->initial_repartition[i] > 0) {
	    spd = 35000.0f / (duration - 10.0f) * sqrt((float) distance * 10.0f / (float) (attacker->os_ship)[i].speed);
	    /*DEBUG_DBG("spd %f", spd); */
	    result +=
		((attacker->initial_repartition[i] * (attacker->os_ship)[i].consumption)) * (float) distance / 35000.0f *
		((spd / 10.0f) + 1) * ((spd / 10.0f) + 1);
	}
    }

    return result;
}


