/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2005 Novell, Inc.
 *
 * Nemo is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * Nemo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin Street - Suite 500,
 * Boston, MA 02110-1335, USA.
 *
 * Author: Anders Carlsson <andersca@imendio.com>
 *
 */

#include <config.h>
#include <glib/gprintf.h>
#include "nemo-search-engine.h"
#include "nemo-search-engine-advanced.h"

#ifdef ENABLE_TRACKER
#include "nemo-search-engine-tracker.h"
#endif

enum {
	HITS_ADDED,
	HITS_SUBTRACTED,
	FINISHED,
	ERROR,
	LAST_SIGNAL
}; 

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE (NemoSearchEngine, nemo_search_engine,
			G_TYPE_OBJECT);

static void
nemo_search_engine_class_init (NemoSearchEngineClass *class)
{
	signals[HITS_ADDED] =
		g_signal_new ("hits-added",
		              G_TYPE_FROM_CLASS (class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (NemoSearchEngineClass, hits_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1,
			      G_TYPE_POINTER);

	signals[HITS_SUBTRACTED] =
		g_signal_new ("hits-subtracted",
		              G_TYPE_FROM_CLASS (class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (NemoSearchEngineClass, hits_subtracted),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1,
			      G_TYPE_POINTER);

	signals[FINISHED] =
		g_signal_new ("finished",
		              G_TYPE_FROM_CLASS (class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (NemoSearchEngineClass, finished),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	
	signals[ERROR] =
		g_signal_new ("error",
		              G_TYPE_FROM_CLASS (class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (NemoSearchEngineClass, error),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__STRING,
		              G_TYPE_NONE, 1,
			      G_TYPE_STRING);

}

static void
nemo_search_engine_init (NemoSearchEngine *engine)
{
}

NemoSearchEngine *
nemo_search_engine_new (void)
{
	NemoSearchEngine *engine;
	
#ifdef ENABLE_TRACKER	
	engine = nemo_search_engine_tracker_new ();
	if (engine) {
		return engine;
	}
#endif

	engine = nemo_search_engine_advanced_new ();
	return engine;
}

void
nemo_search_engine_set_query (NemoSearchEngine *engine, NemoQuery *query)
{
	g_return_if_fail (NEMO_IS_SEARCH_ENGINE (engine));
	g_return_if_fail (NEMO_SEARCH_ENGINE_GET_CLASS (engine)->set_query != NULL);

	NEMO_SEARCH_ENGINE_GET_CLASS (engine)->set_query (engine, query);
}

void
nemo_search_engine_start (NemoSearchEngine *engine)
{
	g_return_if_fail (NEMO_IS_SEARCH_ENGINE (engine));
	g_return_if_fail (NEMO_SEARCH_ENGINE_GET_CLASS (engine)->start != NULL);

	NEMO_SEARCH_ENGINE_GET_CLASS (engine)->start (engine);
}


void
nemo_search_engine_stop (NemoSearchEngine *engine)
{
	g_return_if_fail (NEMO_IS_SEARCH_ENGINE (engine));
	g_return_if_fail (NEMO_SEARCH_ENGINE_GET_CLASS (engine)->stop != NULL);

	NEMO_SEARCH_ENGINE_GET_CLASS (engine)->stop (engine);
}

void	       
nemo_search_engine_hits_added (NemoSearchEngine *engine, GList *hits)
{
	g_return_if_fail (NEMO_IS_SEARCH_ENGINE (engine));

	g_signal_emit (engine, signals[HITS_ADDED], 0, hits);
}


void	       
nemo_search_engine_hits_subtracted (NemoSearchEngine *engine, GList *hits)
{
	g_return_if_fail (NEMO_IS_SEARCH_ENGINE (engine));

	g_signal_emit (engine, signals[HITS_SUBTRACTED], 0, hits);
}


void	       
nemo_search_engine_finished (NemoSearchEngine *engine)
{
	g_return_if_fail (NEMO_IS_SEARCH_ENGINE (engine));

	g_signal_emit (engine, signals[FINISHED], 0);
}

void
nemo_search_engine_error (NemoSearchEngine *engine, const char *error_message)
{
	g_return_if_fail (NEMO_IS_SEARCH_ENGINE (engine));

	g_signal_emit (engine, signals[ERROR], 0, error_message);
}

#define DEBUG_FSR_ACCOUNTING 0

#if DEBUG_FSR_ACCOUNTING
static gint64 count = 0;
static GHashTable *fsr_accounting_table = NULL;
#endif

FileSearchResult *
file_search_result_new (gchar *uri, gchar *snippet)
{
    FileSearchResult *ret = g_new0 (FileSearchResult, 1);

    ret->uri = uri;
    ret->snippet = snippet;

#if DEBUG_FSR_ACCOUNTING
    count++;
    g_printf ("%s - New - Count: %ld\n", uri, count);
    if (fsr_accounting_table == NULL)
    {
        fsr_accounting_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    }

    if (!g_hash_table_add (fsr_accounting_table, g_strdup (uri)))
    {
        g_printerr ("************************* %s already existing\n", uri);
    }
#endif

    return ret;
}

void
file_search_result_free (FileSearchResult *res)
{
#if DEBUG_FSR_ACCOUNTING
    count--;
    g_printf ("%s - Free - Count: %ld\n", res->uri, count);
    if (!g_hash_table_remove (fsr_accounting_table, res->uri)) {
        g_printf ("*************************** URI not found in table");
    }
#endif

    g_free (res->uri);
    g_free (res->snippet);
    g_free (res);
}

void
file_search_result_add_hit (FileSearchResult *result)
{
    result->hits++;
}

void
nemo_search_engine_report_accounting (void)
{
#if DEBUG_FSR_ACCOUNTING
    GList *keys = NULL;

    for (keys = g_hash_table_get_keys (fsr_accounting_table); keys != NULL; keys = keys->next)
    {
        g_printerr ("LEFT: %s\n", keys->data);
    }

    g_hash_table_unref (fsr_accounting_table);
#endif
}

gboolean
nemo_search_engine_check_filename_pattern (NemoQuery   *query,
                                           GError     **error)
{
    g_return_val_if_fail (NEMO_IS_QUERY (query), FALSE);

#ifdef ENABLE_TRACKER
    return TRUE;
#else
    return nemo_search_engine_advanced_check_filename_pattern (query, error);
#endif
}

gboolean
nemo_search_engine_check_content_pattern (NemoQuery   *query,
                                          GError     **error)
{
    g_return_val_if_fail (NEMO_IS_QUERY (query), FALSE);

#ifdef ENABLE_TRACKER
    return TRUE;
#else
    return nemo_search_engine_advanced_check_content_pattern (query, error);
#endif
}
