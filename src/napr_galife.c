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
#include <stdio.h>
#include <time.h>
#include <values.h>

#include <apr_time.h>

#include "debug.h"
#include "napr_galife.h"
#include "napr_heap.h"
#include "napr_threadpool.h"

struct beeing_t
{
    void *chromosome;		/* This will be a pointer to a structure manipulated by:
				 * chrom_fitness, mutation, crossover....
				 * I choose this solution to make my GA the more flexible than possible.
				 * Problem is that with this, I will have to alloc and realloc a lot. 
				 * (look at the bottom of this file to see whole strategy)
				 */
    float score;		/* score result from chrom_fitness applied on beeing_t */
    unsigned char born;		/* During the running process, in order to know if it must be drop */
};

typedef struct beeing_t beeing_t;

struct napr_galife_t
{
    chrom_allocat_callback_fn_t *chrom_allocat;
    chrom_randomz_callback_fn_t *chrom_randomz;
    chrom_display_callback_fn_t *chrom_display;
    chrom_fitness_callback_fn_t *chrom_fitness;
    chrom_crossvr_callback_fn_t *chrom_crossvr;
    chrom_mutation_callback_fn_t *chrom_mutation;
    napr_heap_t *population_past;
    napr_threadpool_t *threadpool;
    /*
     * avoid more than 1 thread to write a best score at the same time,
     * confusing the genetic algo, the result of one is the score of the other
     */
    apr_thread_mutex_t *best_mutex;
    apr_pool_t *pool;
    void *param;
    unsigned long current_age;
    unsigned long population_size;
    unsigned long max_ages;
    apr_time_t inactivity_timeout;
    apr_time_t last_best_date;
    apr_time_t init_time;
    apr_time_t death_date;
    float best_score;
    float crossover_p;
    float mutation_p;
};

static inline void beeing_init(napr_galife_t *ga, beeing_t *beeing)
{
    char errbuf[128];
    apr_status_t status;

    beeing->born = 0;
    beeing->score = (ga->chrom_fitness) (ga->param, beeing->chromosome);
    if (APR_SUCCESS != (status = apr_thread_mutex_lock(ga->best_mutex))) {
	DEBUG_ERR("error calling apr_thread_mutex_lock: %s", apr_strerror(status, errbuf, 128));
	return;
    }
    if (beeing->score > (ga->best_score)) {
	apr_time_t now;

	now = apr_time_now();
	ga->best_score = beeing->score;
	ga->last_best_date = now;
	fprintf(stdout, "[%" APR_TIME_T_FMT " sec] best: score[%f]: ", apr_time_sec(now - ga->init_time), beeing->score);
	fflush(stdout);
	ga->chrom_display(ga->param, beeing->chromosome);
    }
    if (APR_SUCCESS != (status = apr_thread_mutex_unlock(ga->best_mutex))) {
	DEBUG_ERR("error calling apr_thread_mutex_unlock: %s", apr_strerror(status, errbuf, 128));
	return;
    }
}

static int beeing_compare_score(const void *b1, const void *b2)
{
    const beeing_t *beeing1 = b1;
    const beeing_t *beeing2 = b2;

    if (beeing1->score > beeing2->score) {
	return 1;
    }

    return -1;
}

typedef struct threadpool_data_t
{
    /* The beeing which will be fitnessed */
    beeing_t *beeing;
    /* The heap in which he will be inserted */
    napr_heap_t *heap;
} threadpool_data_t;

static apr_status_t napr_galife_process_threadpool_data(void *ctx, void *data)
{
    napr_galife_t *ga = ctx;
    threadpool_data_t *tp_data = data;

    beeing_init(ga, tp_data->beeing);
    if (0 > napr_heap_insert_r(tp_data->heap, tp_data->beeing)) {
	fprintf(stderr, "%s: error calling napr_heap_insert_r in %d\n", __FUNCTION__, __LINE__);
	return APR_ENOMEM;
    }

    return APR_SUCCESS;
}

apr_status_t napr_galife_init(apr_pool_t *pool, unsigned long pop_size, unsigned long max_ages,
			      unsigned long inactivity_timeout, unsigned long fixed_timeout, unsigned long nb_cpu, void *rec,
			      chrom_allocat_callback_fn_t *chrom_allocat, chrom_randomz_callback_fn_t *chrom_randomz,
			      chrom_display_callback_fn_t *chrom_display, chrom_fitness_callback_fn_t *chrom_fitness,
			      float crossover_p, chrom_crossvr_callback_fn_t *chrom_crossvr, float mutation_p,
			      chrom_mutation_callback_fn_t *chrom_mutation, napr_galife_t **ga)
{
    char errbuf[128];
    beeing_t *beeing;
    apr_pool_t *local_pool, *tp_pool;
    char date[APR_RFC822_DATE_LEN];
    unsigned long l;
    apr_status_t status;

    srand((unsigned int) time(NULL));
    apr_pool_create(&local_pool, pool);
    (*ga) = apr_palloc(local_pool, sizeof(struct napr_galife_t));
    (*ga)->pool = local_pool;
    (*ga)->population_past = napr_heap_make_r((*ga)->pool, &beeing_compare_score);

    (*ga)->current_age = 0UL;
    (*ga)->population_size = pop_size;
    (*ga)->max_ages = max_ages;
    (*ga)->inactivity_timeout = apr_time_from_sec(inactivity_timeout);
    (*ga)->last_best_date = (*ga)->init_time = apr_time_now();
    apr_rfc822_date(date, (*ga)->init_time);
    DEBUG_DBG("Genetic algorithm initialized at %s", date);
    (*ga)->death_date = (0UL == fixed_timeout) ? (apr_time_t) 0UL : (*ga)->init_time + apr_time_from_sec(fixed_timeout);
    if ((apr_time_t) 0UL != (*ga)->death_date) {
	apr_rfc822_date(date, (*ga)->death_date);
	DEBUG_DBG("Genetic algorithm die at %s", date);
    }
    (*ga)->param = rec;
    (*ga)->best_score = -FLT_MAX;
    (*ga)->chrom_allocat = chrom_allocat;
    (*ga)->chrom_randomz = chrom_randomz;
    (*ga)->chrom_display = chrom_display;
    (*ga)->chrom_fitness = chrom_fitness;
    (*ga)->crossover_p = crossover_p;
    (*ga)->chrom_crossvr = chrom_crossvr;
    (*ga)->mutation_p = mutation_p;
    (*ga)->chrom_mutation = chrom_mutation;

    if (APR_SUCCESS != (status = apr_thread_mutex_create(&((*ga)->best_mutex), APR_THREAD_MUTEX_DEFAULT, (*ga)->pool))) {
	DEBUG_ERR("error calling apr_thread_mutex_create: %s", apr_strerror(status, errbuf, 128));
	return status;
    }

    if (APR_SUCCESS !=
	(status =
	 napr_threadpool_init(&((*ga)->threadpool), (*ga), nb_cpu, napr_galife_process_threadpool_data, (*ga)->pool))) {
	DEBUG_ERR("error calling napr_threadpool_init: %s", apr_strerror(status, errbuf, 128));
	return status;
    }
    /* Because we want to collect garbage */
    apr_pool_create(&tp_pool, local_pool);

    /*
     * Now we fill the heap with random generated beeing.
     */
    for (l = 0; l < pop_size; l++) {
	threadpool_data_t *tp_data;

	if (0 != (*ga)->inactivity_timeout) {
	    apr_time_t now;

	    now = apr_time_now();
	    if ((*ga)->inactivity_timeout < (now - (*ga)->last_best_date)) {
		DEBUG_DBG("Genetic algorithm timeouted");
		break;
	    }
	}
	if ((apr_time_t) 0UL != (*ga)->death_date) {
	    apr_time_t now;

	    now = apr_time_now();
	    if ((*ga)->death_date < now) {
		DEBUG_DBG("Genetic algorithm fixed-timeouted");
		break;
	    }
	}

	beeing = apr_palloc((*ga)->pool, sizeof(struct beeing_t));
	chrom_allocat(rec, (*ga)->pool, &(beeing->chromosome));
	chrom_randomz(rec, beeing->chromosome);

	tp_data = apr_palloc(tp_pool, sizeof(struct threadpool_data_t));
	tp_data->beeing = beeing;
	tp_data->heap = (*ga)->population_past;
	if (APR_SUCCESS != (status = napr_threadpool_add((*ga)->threadpool, tp_data))) {
	    DEBUG_ERR("error calling napr_threadpool_add: %s", apr_strerror(status, errbuf, 128));
	    return status;
	}

    }

    if (APR_SUCCESS != (status = napr_threadpool_wait((*ga)->threadpool))) {
	DEBUG_ERR("error calling napr_threadpool_wait: %s", apr_strerror(status, errbuf, 128));
	return status;
    }
    apr_pool_destroy(tp_pool);

    return APR_SUCCESS;
}

apr_status_t ga_run(napr_galife_t *ga)
{
    char errbuf[128];
    unsigned long era, born;
    unsigned int mother_idx, i, pop_nmb;
    apr_pool_t *tp_pool;	/* garbage collector */
    napr_heap_t *population_future, *swap;
    beeing_t *father, *mother, *buf;
    apr_status_t status;

    /*
     * era(s) are generations.
     */
    population_future = napr_heap_make_r(ga->pool, &beeing_compare_score);

    for (era = 0; era < ga->max_ages; era++) {
	/*DEBUG_DBG("Era [%lu]", era); */
	/* future population is the place where already processed and new born beeings go */
	born = 1;

	apr_pool_create(&tp_pool, ga->pool);

	/* we go through the whole past population to see which survives and which fuck */
	while (napr_heap_size(ga->population_past) > 1) {
	    if (0 != ga->inactivity_timeout) {
		apr_time_t now;

		now = apr_time_now();
		if (ga->inactivity_timeout < (now - ga->last_best_date)) {
		    DEBUG_DBG("Genetic algorithm timeouted");
		    era = ga->max_ages;
		    break;
		}
	    }
	    if ((apr_time_t) 0UL != ga->death_date) {
		apr_time_t now;

		now = apr_time_now();
		if (ga->death_date < now) {
		    DEBUG_DBG("Genetic algorithm fixed-timeouted");
		    era = ga->max_ages;
		    break;
		}
	    }

	    /* extraction and check if the parents will die */
	    /*DEBUG_DBG("Father extract / %lu", napr_heap_size(ga->population_past)); */
	    if (NULL == (father = napr_heap_extract_r(ga->population_past)))
		break;

	    /* This is a new beeing, don't mate him this time */
	    if (0 != father->born) {
		threadpool_data_t *tp_data;

		/*DEBUG_DBG("new born"); */
		tp_data = apr_palloc(tp_pool, sizeof(struct threadpool_data_t));
		tp_data->beeing = father;
		tp_data->heap = population_future;
		if (APR_SUCCESS != (status = napr_threadpool_add(ga->threadpool, tp_data))) {
		    DEBUG_ERR("error calling napr_threadpool_add: %s", apr_strerror(status, errbuf, 128));
		    return status;
		}

		continue;
	    }

	    /*DEBUG_DBG("Mother get"); */
	    pop_nmb = napr_heap_size(ga->population_past);
	    mother_idx = (pop_nmb * rand() / (RAND_MAX + 1.0f));
	    if (NULL == (mother = napr_heap_get_nth(ga->population_past, mother_idx))) {
		/*DEBUG_DBG("Last.. insert father in pop."); */
		if (0 > napr_heap_insert_r(population_future, father)) {
		    fprintf(stderr, "%s: error calling napr_heap_insert_r in %d\n", __FUNCTION__, __LINE__);
		    return APR_EGENERAL;
		}
		break;
	    }
	    if (1 == mother->born) {
		/*DEBUG_DBG("Mother [%i]/[%i] already born", mother_idx, pop_nmb); */
		for (i = 1; (i < pop_nmb) && (NULL != mother) && (1 == mother->born); i++) {
		    mother = napr_heap_get_nth(ga->population_past, (mother_idx + i) % pop_nmb);
		}
		if ((NULL == mother) || (1 == mother->born)) {
		    /*DEBUG_DBG("No more unborn amongst [%i]", pop_nmb); */
		    if (0 > napr_heap_insert_r(population_future, father)) {
			fprintf(stderr, "%s: error calling napr_heap_insert_r in %d\n", __FUNCTION__, __LINE__);
			return APR_EGENERAL;
		    }

		    break;
		}
	    }
	    mother->born = 1;

	    /*
	     * Babies herits from the ancients, here we must check the probability of mutation
	     * Prob of Crossover must be check in the function for each unit copied into the chromosome there's a prob. 
	     * crossover_p that this unit came from the other parent.
	     */
	    /*DEBUG_DBG("Cross"); */
	    ga->chrom_crossvr(ga->param, ga->crossover_p, father->chromosome, mother->chromosome);
	    /*DEBUG_DBG("Mutate"); */
	    if (ga->mutation_p > ((float) rand() / (RAND_MAX + 1.0f)))
		ga->chrom_mutation(ga->param, ga->mutation_p, mother->chromosome);

	    /*DEBUG_DBG("Insert Father"); */
	    if (0 > napr_heap_insert_r(population_future, father)) {
		fprintf(stderr, "%s: error calling napr_heap_insert_r in %d\n", __FUNCTION__, __LINE__);
		return APR_EGENERAL;
	    }
	}

	if (NULL != (buf = napr_heap_extract_r(ga->population_past))) {
	    if (0 > napr_heap_insert_r(population_future, buf)) {
		fprintf(stderr, "%s: error calling napr_heap_insert_r in %d\n", __FUNCTION__, __LINE__);
		return APR_EGENERAL;
	    }
	}

	if (APR_SUCCESS != (status = napr_threadpool_wait(ga->threadpool))) {
	    DEBUG_ERR("error calling napr_threadpool_wait: %s", apr_strerror(status, errbuf, 128));
	    return status;
	}
	apr_pool_destroy(tp_pool);

	swap = ga->population_past;
	ga->population_past = population_future;
	population_future = swap;
    }

    napr_heap_destroy(population_future);

    return APR_SUCCESS;
}


