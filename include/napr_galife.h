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

#ifndef NAPR_GALIFE_H
#define NAPR_GALIFE_H

#include <apr_pools.h>

/* #define DEBUG */

typedef struct napr_galife_t napr_galife_t;

/**
 * function that allocate a gene for your problem.
 * @param rec The data passed as the first argument to napr_galife_init.
 * @param chromosome A pointer to a chromosome that will be created.
 */
typedef void (chrom_allocat_callback_fn_t) (void *rec, apr_pool_t *pool, void **chromosome);

/**
 * function that randomize a gene.
 * @param rec The data passed as the first argument to napr_galife_init.
 * @param chromosome The gene to shuffle.
 */
typedef void (chrom_randomz_callback_fn_t) (void *rec, void *chromosome);

/**
 * The function that will report some useful informations about a chromosome,
 * called for each best.
 * @param rec The data passed as the first argument to napr_galife_init.
 * @param chromosome The gene to print.
 */
typedef void (chrom_display_callback_fn_t) (void *rec, const void *chromosome);

/**
 * Function that will estimate the efficiency of a gene.
 * @param rec The data passed as the first argument to napr_galife_init.
 * @param chromosome The gene to evaluate.
 */
typedef float (chrom_fitness_callback_fn_t) (void *rec, void *chromosome);

/** 
 * Function that will cross two genes into one.
 * @param rec The data passed as the first argument to napr_galife_init.
 * @param crossover_p Probability that a crossover occured.
 * @param father One of the parent genes.
 * @param mother The other parent gene overwritten by child.
 */
typedef void (chrom_crossvr_callback_fn_t) (void *rec, float crossover_p, const void *father, void *mother);

/**
 * Function that mutate one gene.
 * @param rec The data passed as the first argument to napr_galife_init.
 * @param mutation_p Probability that a mutation occured.
 * @param chromosome The gene to mutate.
 */
typedef void (chrom_mutation_callback_fn_t) (void *rec, float mutation_p, void *chromosome);

/** 
 * Allocate and initialize a genetic algorithm worker.
 * @param pop_size Number of beings.
 * @param max_ages Number of generations to run.
 * @param inactivity_timeout Maximum time (in seconds) to wait a best individual, if no best has been found, stop the generation.
 * @param fixed_timeout Maximum time to process.
 * @param nb_cpu The number of threads that will be running, ideally should be equal to number of CPU.
 * @param rec The data to pass as the first argument to the chromosome function.
 * @param chrom_allocat The function to run in order to allocate a chromosome.
 * @param chrom_randomz The function to run in order to randomize a chromosome.
 * @param chrom_display The function that will print on stderr some useful informations about a chromosome.
 * @param fitness Function that will estimate the efficiency of a gene.
 * @param crossover_p Probability that a crossover occured.
 * @param crossover Function that will cross two genes into one.
 * @param mutation_p Probability that a mutation occured.
 * @param mutation Function that mutate one gene.
 * @param ga The genetic algorithm searcher that will be freshly allocated by this function.
 * 
 * @return APR_SUCCESS if no error occured.
 */
apr_status_t napr_galife_init(apr_pool_t *pool, unsigned long pop_size, unsigned long max_ages,
			      unsigned long inactivity_timeout, unsigned long fixed_timeout, unsigned long nb_cpu, void *rec,
			      chrom_allocat_callback_fn_t *chrom_allocat, chrom_randomz_callback_fn_t *chrom_randomz,
			      chrom_display_callback_fn_t *chrom_display, chrom_fitness_callback_fn_t *fitness,
			      float crossover_p, chrom_crossvr_callback_fn_t *crossover, float mutation_p,
			      chrom_mutation_callback_fn_t *mutation, napr_galife_t **ga);

apr_status_t ga_run(napr_galife_t *ga);

/*
 * greetz to Juan who thought about heap for handling GA... and for all !
 */

#endif /* NAPR_GALIFE_H */


