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

#include <apr_strings.h>

#include <pcre.h>
#include "debug.h"
#include "os_conf.h"
#include "os_parse.h"

static const char *regex_total = "Matières premières sur (?P<planet>.*) \\[(?P<position>[0-9:]*)\\] le (.*).*\
Métal:\\s*(?P<metal>[-0-9.]*)\\s*Cristal:\\s*(?P<cristal>[-0-9.]*)\\s*Deutérium:\\s*(?P<deuterium>[-0-9.]*)\\s*Energie:\\s*(?P<energie>[0-9.]*)\
\\s*(?:Flotte\\s*(?:Petit transporteur\\s*(?P<pt>[0-9.]*))?\\s*(?:Grand transporteur\\s*(?P<gt>[0-9.]*))?\\s*(?:Chasseur léger\\s*(?P<cle>[0-9.]*))?\
\\s*(?:Chasseur lourd\\s*(?P<clo>[0-9.]*))?\\s*(?:Croiseur\\s*(?P<cr>[0-9.]*))?\\s*(?:Vaisseau de bataille\\s*(?P<vb>[0-9.]*))?\
\\s*(?:Vaisseau de colonisation\\s*(?P<vc>[0-9.]*))?\\s*(?:Recycleur\\s*(?P<rec>[0-9.]*))?\\s*(?:Sonde espionnage\\s*(?P<se>[0-9.]*))?\
\\s*(?:Bombardier\\s*(?P<bb>[0-9.]*))?\\s*(?:Satellite solaire\\s*(?P<sat>[0-9.]*))?\\s*(?:Destructeur\\s*(?P<dest>[0-9.]*))?\
\\s*(?:Étoile de la mort\\s*(?P<edlm>[0-9.]*))?\\s*(?:Traqueur\\s*(?P<trac>[0-9.]*))?)?\
\\s*(?:Défense\\s*(?:Lanceur de missiles\\s*(?P<lm>[0-9.]*))?\\s*(?:Artillerie laser légère\\s*(?P<alle>[0-9.]*))?\
\\s*(?:Artillerie laser lourde\\s*(?P<allo>[0-9.]*))?\\s*(?:Canon de Gauss\\s*(?P<cg>[0-9.]*))?\\s*(?:Artillerie à ions\\s*(?P<aai>[0-9.]*))?\
\\s*(?:Lanceur de plasma\\s*(?P<lp>[0-9.]*))?\\s*(?:Petit bouclier\\s*(?P<pb>[0-9.]*))?\\s*(?:Grand bouclier\\s*(?P<gb>[0-9.]*))?\
\\s*(?:Missile Interception\\s*(?P<mit>[0-9.]*))?\\s*(?:Missile Interplanétaire\\s*(?P<mip>[0-9.]*))?)?\
\\s*(?:Bâtiments\\s*(?:Mine de métal\\s*(?P<minemetal>[0-9.]*))?\\s*(?:Mine de cristal\\s*(?P<minecristal>[0-9.]*))?\
\\s*(?:Synthétiseur de deutérium\\s*(?P<synthdeut>[0-9.]*))?\\s*(?:Centrale électrique solaire\\s*(?P<centsol>[0-9.]*))?\
\\s*(?:Centrale électrique de fusion\\s*(?P<centfus>[0-9.]*))?\\s*(?:Usine de robots\\s*(?P<robots>[0-9.]*))?\
\\s*(?:Usine de nanites\\s*(?P<nanites>[0-9.]*))?\\s*(?:Chantier spatial\\s*(?P<chantier>[0-9.]*))?\
\\s*(?:Hangar de métal\\s*(?P<hangarmet>[0-9.]*))?\\s*(?:Hangar de cristal\\s*(?P<hangarcris>[0-9.]*))?\
\\s*(?:Réservoir de deutérium\\s*(?P<reserdeut>[0-9.]*))?\\s*(?:Laboratoire de recherche\\s*(?P<labo>[0-9.]*))?\
\\s*(?:Terraformeur\\s*(?P<terra>[0-9.]*))?\\s*(?:Silo de missiles\\s*(?P<silo>[0-9.]*))?\\s*(?:Base lunaire\\s*(?P<lunaire>[0-9.]*))?\
\\s*(?:Phalange de capteur\\s*(?P<phal>[0-9.]*))?\\s*(?:Porte de saut spatial\\s*(?P<pss>[0-9.]*))?)?\
\\s*(?:Recherche\\s*(?:Technologie Espionnage\\s*(?P<espio>[0-9.]*))?\\s*(?:Technologie Ordinateur\\s*(?P<ordi>[0-9.]*))?\
\\s*(?:Technologie Armes\\s*(?P<damg>[0-9.]*))?\\s*(?:Technologie Bouclier\\s*(?P<shld>[0-9.]*))?\
\\s*(?:Technologie Protection des vaisseaux spatiaux\\s*(?P<life>[0-9.]*))?\\s*(?:Technologie Energie\\s*(?P<nrjtek>[0-9.]*))?\
\\s*(?:Technologie Hyperespace\\s*(?P<hypertek>[0-9.]*))?\\s*(?:Réacteur à combustion\\s*(?P<comb>[0-9.]*))?\
\\s*(?:Réacteur à impulsion\\s*(?P<imp>[0-9.]*))?\\s*(?:Propulsion hyperespace\\s*(?P<hyperprop>[0-9.]*))?\
\\s*(?:Technologie Laser\\s*(?P<laser>[0-9.]*))?\\s*(?:Technologie Ions\\s*(?P<ions>[0-9.]*))?\
\\s*(?:Technologie Plasma\\s*(?P<plasma>[0-9.]*))?\\s*(?:Réseau de recherche intergalactique\\s*(?P<rres>[0-9.]*))?\
\\s*(?:Technologie Graviton\\s*(?P<grav>[0-9.]*))?)?";
/*
\\s*Probabilité de destruction de la flotte d'espionnage ";
*/
/* 62 names x 3 = 186  < 210 */
#define MATCH_VECTOR_SIZE 210
#define BIG_STR 256

#define CHK_SUBSTR(reg,str,ovector,size,name,buffer,bufsiz) { \
    int rc; \
    apr_size_t i, j; \
    \
    if(0 > (rc = pcre_copy_named_substring(reg, str, ovector, size, name, buffer, bufsiz))) { \
	DEBUG_ERR("An error occured while getting [%s]:[%i]", name, rc); \
	return NULL; \
    } \
    /* remove '.' from bignumber */ \
    \
    for (i = 0, j = 0; j < rc; i++, j++) { \
	if ('.' != buffer[j]) \
	    buffer[i] = buffer[j]; \
	else \
	    --i; \
    } \
    \
    if (i != rc) { \
	buffer[i] = '\0'; \
    } \
}

static char *os_parse_debug(apr_pool_t *pool, const char *report)
{
    int ovector[MATCH_VECTOR_SIZE];
    char buffer[BIG_STR];
    const char *errptr;
    pcre *regexp_compiled;
    apr_size_t report_len;
    int erroffset, rc;

    regexp_compiled = pcre_compile(regex_total, PCRE_DOLLAR_ENDONLY | PCRE_DOTALL, &errptr, &erroffset, NULL);
    if (NULL != regexp_compiled) {
	report_len = strlen(report);
	rc = pcre_exec(regexp_compiled, NULL, report, report_len, 0, 0, ovector, MATCH_VECTOR_SIZE);
	if (rc >= 0) {
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "position", buffer, BIG_STR);
	    DEBUG_DBG("position is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "planet", buffer, BIG_STR);
	    DEBUG_DBG("planet is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "metal", buffer, BIG_STR);
	    DEBUG_DBG("metal is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "cristal", buffer, BIG_STR);
	    DEBUG_DBG("cristal is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "deuterium", buffer, BIG_STR);
	    DEBUG_DBG("deuterium is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "energie", buffer, BIG_STR);
	    DEBUG_DBG("energie is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "pt", buffer, BIG_STR);
	    DEBUG_DBG("pt is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "gt", buffer, BIG_STR);
	    DEBUG_DBG("gt is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "cle", buffer, BIG_STR);
	    DEBUG_DBG("cle is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "cr", buffer, BIG_STR);
	    DEBUG_DBG("cr is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "vb", buffer, BIG_STR);
	    DEBUG_DBG("vb is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "vc", buffer, BIG_STR);
	    DEBUG_DBG("vc is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "rec", buffer, BIG_STR);
	    DEBUG_DBG("rec is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "se", buffer, BIG_STR);
	    DEBUG_DBG("se is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "bb", buffer, BIG_STR);
	    DEBUG_DBG("bb is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "sat", buffer, BIG_STR);
	    DEBUG_DBG("sat is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "dest", buffer, BIG_STR);
	    DEBUG_DBG("dest is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "edlm", buffer, BIG_STR);
	    DEBUG_DBG("edlm is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "trac", buffer, BIG_STR);
	    DEBUG_DBG("trac is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "lm", buffer, BIG_STR);
	    DEBUG_DBG("lm is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "alle", buffer, BIG_STR);
	    DEBUG_DBG("alle is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "allo", buffer, BIG_STR);
	    DEBUG_DBG("allo is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "cg", buffer, BIG_STR);
	    DEBUG_DBG("cg is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "aai", buffer, BIG_STR);
	    DEBUG_DBG("aai is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "lp", buffer, BIG_STR);
	    DEBUG_DBG("lp is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "pb", buffer, BIG_STR);
	    DEBUG_DBG("pb is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "gb", buffer, BIG_STR);
	    DEBUG_DBG("gb is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "mit", buffer, BIG_STR);
	    DEBUG_DBG("mit is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "mip", buffer, BIG_STR);
	    DEBUG_DBG("mip is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "minemetal", buffer, BIG_STR);
	    DEBUG_DBG("minemetal is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "minecristal", buffer, BIG_STR);
	    DEBUG_DBG("minecristal is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "synthdeut", buffer, BIG_STR);
	    DEBUG_DBG("synthdeut is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "centsol", buffer, BIG_STR);
	    DEBUG_DBG("centsol is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "centfus", buffer, BIG_STR);
	    DEBUG_DBG("centfus is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "robots", buffer, BIG_STR);
	    DEBUG_DBG("robots is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "nanites", buffer, BIG_STR);
	    DEBUG_DBG("nanites is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "chantier", buffer, BIG_STR);
	    DEBUG_DBG("chantier is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "hangarmet", buffer, BIG_STR);
	    DEBUG_DBG("hangarmet is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "hangarcris", buffer, BIG_STR);
	    DEBUG_DBG("hangarcris is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "reserdeut", buffer, BIG_STR);
	    DEBUG_DBG("reserdeut is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "labo", buffer, BIG_STR);
	    DEBUG_DBG("labo is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "terra", buffer, BIG_STR);
	    DEBUG_DBG("terra is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "silo", buffer, BIG_STR);
	    DEBUG_DBG("silo is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "espio", buffer, BIG_STR);
	    DEBUG_DBG("espio is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "ordi", buffer, BIG_STR);
	    DEBUG_DBG("ordi is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "damg", buffer, BIG_STR);
	    DEBUG_DBG("damg is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "shld", buffer, BIG_STR);
	    DEBUG_DBG("shld is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "life", buffer, BIG_STR);
	    DEBUG_DBG("life is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "nrjtek", buffer, BIG_STR);
	    DEBUG_DBG("nrjtek is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "hypertek", buffer, BIG_STR);
	    DEBUG_DBG("hypertek is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "comb", buffer, BIG_STR);
	    DEBUG_DBG("comb is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "imp", buffer, BIG_STR);
	    DEBUG_DBG("imp is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "hyperprop", buffer, BIG_STR);
	    DEBUG_DBG("hyperprop is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "laser", buffer, BIG_STR);
	    DEBUG_DBG("laser is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "ions", buffer, BIG_STR);
	    DEBUG_DBG("ions is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "plasma", buffer, BIG_STR);
	    DEBUG_DBG("plasma is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "rres", buffer, BIG_STR);
	    DEBUG_DBG("rres is [%s]", buffer);
	    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "grav", buffer, BIG_STR);
	    DEBUG_DBG("grav is [%s]", buffer);
	}
	else {
	    DEBUG_ERR("An error occured while matching [%s] : [%i]", regex_total, rc);
	}
    }
    else {
	DEBUG_ERR("An error occured while compiling [%s] : [%s] [%i]", regex_total, errptr, erroffset);
    }

    return NULL;
}

extern char *os_parse(apr_pool_t *pool, const char *report)
{
    int ovector[MATCH_VECTOR_SIZE];
    char buffer[BIG_STR];
    char *elements[MISC_END];
    const char *errptr;
    char *result;
    pcre *regexp_compiled;
    apr_size_t report_len;
    int erroffset, rc;

    regexp_compiled = pcre_compile(regex_total, PCRE_DOLLAR_ENDONLY | PCRE_DOTALL, &errptr, &erroffset, NULL);
    if (NULL == regexp_compiled) {
	DEBUG_ERR("An error occured while compiling [%s] : [%s] [%i]", regex_total, errptr, erroffset);
	return NULL;
    }

    report_len = strlen(report);
    rc = pcre_exec(regexp_compiled, NULL, report, report_len, 0, 0, ovector, MATCH_VECTOR_SIZE);
    if (rc < 0) {
	DEBUG_ERR("An error occured while matching [%s] : [%i]", regex_total, rc);
	return NULL;
    }
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "position", buffer, BIG_STR);
    elements[POSITION] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "metal", buffer, BIG_STR);
    elements[METL] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "cristal", buffer, BIG_STR);
    elements[CRIS] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "deuterium", buffer, BIG_STR);
    elements[DEUT] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "pt", buffer, BIG_STR);
    elements[PT] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "gt", buffer, BIG_STR);
    elements[GT] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "cle", buffer, BIG_STR);
    elements[CLE] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "clo", buffer, BIG_STR);
    elements[CLO] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "cr", buffer, BIG_STR);
    elements[CR] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "vb", buffer, BIG_STR);
    elements[VB] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "vc", buffer, BIG_STR);
    elements[VC] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "rec", buffer, BIG_STR);
    elements[REC] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "se", buffer, BIG_STR);
    elements[SE] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "bb", buffer, BIG_STR);
    elements[BB] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "sat", buffer, BIG_STR);
    elements[SAT] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "dest", buffer, BIG_STR);
    elements[DEST] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "edlm", buffer, BIG_STR);
    elements[EDLM] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "trac", buffer, BIG_STR);
    elements[TRAC] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "lm", buffer, BIG_STR);
    elements[LM] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "alle", buffer, BIG_STR);
    elements[ALLE] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "allo", buffer, BIG_STR);
    elements[ALLO] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "cg", buffer, BIG_STR);
    elements[CG] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "aai", buffer, BIG_STR);
    elements[AAI] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "lp", buffer, BIG_STR);
    elements[LP] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "pb", buffer, BIG_STR);
    elements[PB] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "gb", buffer, BIG_STR);
    elements[GB] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "damg", buffer, BIG_STR);
    elements[DAMG] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "shld", buffer, BIG_STR);
    elements[SHLD] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "life", buffer, BIG_STR);
    elements[LIFE] = apr_pstrdup(pool, (*buffer) ? buffer : "0");
    CHK_SUBSTR(regexp_compiled, report, ovector, MATCH_VECTOR_SIZE / 3, "mit", buffer, BIG_STR);
    elements[MIT] = apr_pstrdup(pool, (*buffer) ? buffer : "0");

    result =
	apr_pstrcat(pool, elements[DAMG], ",", elements[SHLD], ",", elements[LIFE], ",", elements[POSITION], ",",
		    elements[METL], ",", elements[CRIS], ",", elements[DEUT], ",", elements[PT], ",", elements[GT], ",",
		    elements[CLE], ",", elements[CLO], ",", elements[CR], ",", elements[VB], ",", elements[VC], ",",
		    elements[REC], ",", elements[SE], ",", elements[BB], ",", elements[SAT], ",", elements[DEST], ",",
		    elements[EDLM], ",", elements[TRAC], ",", elements[LM], ",", elements[ALLE], ",", elements[ALLO],
		    ",", elements[CG], ",", elements[AAI], ",", elements[LP], ",", elements[PB], ",", elements[GB], ",",
		    elements[MIT], NULL);
    /* Checking if parsing failed on a set of data */
    if (elements[POSITION][0] == '0') {
	DEBUG_ERR("Missing position");
	os_parse_debug(pool, report);
	return NULL;
    }
    if (elements[DAMG][0] == '0' && elements[SHLD][0] == '0' && elements[LIFE][0] == '0') {
	DEBUG_ERR("Missing technologies");
	os_parse_debug(pool, report);
	return NULL;
    }
    if (elements[PT][0] == '0' && elements[GT][0] == '0' && elements[CLE][0] == '0' && elements[CLO][0] == '0'
	&& elements[CR][0] == '0' && elements[VB][0] == '0' && elements[VC][0] == '0' && elements[REC][0] == '0'
	&& elements[SE][0] == '0' && elements[BB][0] == '0' && elements[SAT][0] == '0' && elements[DEST][0] == '0'
	&& elements[EDLM][0] == '0' && elements[TRAC][0]) {
	DEBUG_ERR("No fleet");
	os_parse_debug(pool, report);
    }
    if (elements[LM][0] == '0' && elements[ALLE][0] == '0' && elements[ALLO][0] == '0' && elements[CG][0] == '0'
	&& elements[AAI][0] == '0' && elements[LP][0] == '0' && elements[PB][0] == '0' && elements[GB][0] == '0') {
	DEBUG_ERR("No def");
	os_parse_debug(pool, report);
    }
    return result;
}


