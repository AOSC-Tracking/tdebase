/*****************************************************************

   Copyright (c) 2006 Debajyoti Bera <dbera.web@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

******************************************************************/

#include "beaglesearch.h"

#include <tqdatetime.h>
#include <tqmutex.h>
#include <tqstringlist.h>
#include <tqapplication.h>
#include <time.h>

void beagle_init ()
{
    g_type_init ();
}

// ---------------- Hit ---------------------------

Hit::Hit (BeagleHit *_hit) : processed (false)
{
    hit = beagle_hit_ref (_hit);
}

Hit::~Hit ()
{
    beagle_hit_unref (hit);
    if (! processed)
	return;
    TQDictIterator<TQStringList> it (property_map);
    for( ; it.current(); ++it )
        ((TQStringList *)it.current())->clear ();

}

void Hit::processProperties ()
{
    processed = true;
    GSList *prop_list = beagle_hit_get_all_properties (hit);
    GSList *it;
    property_map.setAutoDelete (true);
    for (it = prop_list; it; it = it->next) {
        BeagleProperty *property = (BeagleProperty *) it->data;
        TQString key = TQString::fromUtf8 (beagle_property_get_key (property));
        if (! property_map [key])
            property_map.insert (key, new TQStringList ());
        property_map [key]->append (TQString::fromUtf8 (beagle_property_get_value (property)));
    }
    g_slist_free (prop_list);
}

const TQString Hit::operator[] (TQString prop_name)
{
    if (! processed)
	processProperties ();

    TQStringList *prop_list = property_map [prop_name];
    if (! prop_list)
        return TQString::null;
    if (prop_list->count () != 1)
        return TQString::null;
    return (TQString)prop_list->first ();
}

// ---------------- BeagleSearch ------------------

BeagleSearchResult::BeagleSearchResult(int client_id)
  : client_id (client_id), total (0)
{
    hitlist = new TQPtrList<Hit>;
    hitlist->setAutoDelete (true);
}


BeagleSearchResult::~BeagleSearchResult()
{
    // everything is set to autodelete
}

void BeagleSearchResult::addHit(BeagleHit *_hit)
{
    Hit *hit = new Hit (_hit);
    hitlist->prepend (hit);
}

const TQPtrList<Hit> *BeagleSearchResult::getHits () const
{
    return hitlist;
}


static int total_hits;

static void print_feed_item_hit (BeagleHit *hit)
{
    const char *text;

    if (beagle_hit_get_one_property (hit, "dc:title", &text))
        g_print ("Blog: %s\n", text);
}

static void print_file_hit (BeagleHit *hit)
{
    g_print ("File: %s, (%s)\n", beagle_hit_get_uri (hit), beagle_hit_get_mime_type (hit));
}

static void print_other_hit (BeagleHit *hit)
{
    const char *text;

    g_print ("%s (%s)", beagle_hit_get_uri (hit),
             beagle_hit_get_source (hit));
    if (beagle_hit_get_one_property (hit, "dc:title", &text))
        g_print ("title = %s\n", text);
}

static void print_hit (BeagleHit *hit) 
{
    if (strcmp (beagle_hit_get_type (hit), "FeedItem") == 0) {
        print_feed_item_hit (hit);
    } 
    else if (strcmp (beagle_hit_get_type (hit), "File") == 0) {
        print_file_hit (hit);
    } else {
        print_other_hit (hit);
    }
}

// ---------------- BeagleSearchClient ------------------

void BeagleSearchClient::run ()
{
    kdDebug () << "Starting query ..." << endl;

    TQTime query_timer;
    query_timer.start ();
    
    g_signal_connect (query, "hits-added",
                      G_CALLBACK (hitsAddedSlot),
                      this);
    g_signal_connect (query, "finished",
                      G_CALLBACK (finishedSlot),
                      this);
    beagle_client_send_request_async (client,
                                      BEAGLE_REQUEST (query),
                                      NULL);
    g_main_loop_run (main_loop);
    kdDebug () << "Finished query ..." << endl;
    
    TQCustomEvent *ev;
    if (collate_results) {
	result->query_msec = query_timer.elapsed ();
    
	ev =  new TQCustomEvent (RESULTFOUND, result);
        TQApplication::postEvent (object, ev);
    }

    ev =  new TQCustomEvent (KILLME, this);
    TQApplication::postEvent (object, ev);

}

void BeagleSearchClient::stopClient ()
{
   if (finished ())
       return; // duh!
   kdDebug () << "Query thread " << id << " not yet finished ..." << endl;
   // get ready for suicide
   client_mutex->lock ();
   kill_me = true;
   g_signal_handlers_disconnect_by_func (
           query, 
           (void *)hitsAddedSlot,
           this);
   g_signal_handlers_disconnect_by_func (
           query,
           (void *)finishedSlot,
           this);
   g_main_loop_quit (main_loop);
   client_mutex->unlock ();
}

void BeagleSearchClient::hitsAddedSlot (BeagleQuery *query,
                                        BeagleHitsAddedResponse *response,
                                        BeagleSearchClient *bsclient)
{
    GSList *hits, *l;
    gint    i;
    gint    nr_hits;

    // check if we are supposed to be killed
    bsclient->client_mutex->lock ();
    if (bsclient->kill_me) {
        kdDebug () << "Suicide time before processing" << endl;
	bsclient->client_mutex->unlock ();
	return;
    }
    bsclient->client_mutex->unlock ();

    hits = beagle_hits_added_response_get_hits (response);

    nr_hits = g_slist_length (hits);
    total_hits += nr_hits;
    g_print ("Found hits (%d) at %ld:\n", nr_hits, time (NULL));

    BeagleSearchResult *search_result;
    if (! bsclient->collate_results)
        search_result = new BeagleSearchResult (bsclient->id);
    else
        search_result = bsclient->result;
    search_result->total += nr_hits;

    for (l = hits, i = 1; l; l = l->next, ++i) {
        //g_print ("[%d] ", i);
        //print_hit (BEAGLE_HIT (l->data));
        //g_print ("\n");

        search_result->addHit(BEAGLE_HIT (l->data));//hit);
    }
    g_print ("[%ld] hits adding finished \n", time (NULL));

    // check if we are supposed to be killed
    bsclient->client_mutex->lock ();
    if (bsclient->kill_me) {
        kdDebug () << "Suicide time before sending ..." << endl;
	bsclient->client_mutex->unlock ();
	if (! bsclient->collate_results)
	    delete search_result;
	return;
    }
    bsclient->client_mutex->unlock ();

    // time to send back results, if user asked so
    if (bsclient->collate_results)
        return;
    TQCustomEvent *ev =  new TQCustomEvent (RESULTFOUND, search_result);
    g_print ("[%ld] event notified \n", time (NULL));
    TQApplication::postEvent (bsclient->object, ev);
}

void BeagleSearchClient::finishedSlot (BeagleQuery *query,
                                       BeagleFinishedResponse *response,
                                       BeagleSearchClient *bsclient)
{
    // check if we are supposed to be killed
    bsclient->client_mutex->lock ();
    bool should_kill = bsclient->kill_me;
    TQObject* receiver = bsclient->object;
    bsclient->client_mutex->unlock ();

    if (should_kill)
	return;

    g_main_loop_quit (bsclient->main_loop);

    if (bsclient->collate_results)
        return; // if we are collating, everything will be send from a central place
    if (receiver) {
        TQCustomEvent *ev =  new TQCustomEvent (SEARCHOVER, bsclient);
        g_print ("[%ld] query finish notified \n", time (NULL));
        TQApplication::postEvent (receiver, ev);
    }
}

// ----------------- BeagleUtil -------------------

BeagleQuery *
BeagleUtil::createQueryFromString (TQString query_str,
                                    TQStringList &sources_menu,
                                    TQStringList &types_menu,
				    int max_hits_per_source)
{
    BeagleQuery *beagle_query = beagle_query_new ();
    beagle_query_set_max_hits (beagle_query, max_hits_per_source); // this is per source!

    kdDebug () << "Creating query from \"" << query_str << "\"" << endl;
    for ( TQStringList::Iterator it = sources_menu.begin(); it != sources_menu.end(); ++it )
        beagle_query_add_source (beagle_query, g_strdup ((*it).utf8 ()));

    for ( TQStringList::Iterator it = types_menu.begin(); it != types_menu.end(); ++it )
        beagle_query_add_hit_type (beagle_query, g_strdup ((*it).utf8 ()));

    TQStringList query_terms;
    TQString start_date, end_date;
    TQStringList words = TQStringList::split (' ', query_str, false);
    for ( TQStringList::Iterator it = words.begin(); it != words.end(); ++it ) {
        TQStringList key_value_pair = TQStringList::split ('=', *it, false);
        if (key_value_pair.count () == 1)
            query_terms += *it;
        else if (key_value_pair.count () == 2) {
            TQString key = key_value_pair [0].lower ();
            TQString value = key_value_pair [1];
            if (key == "mime")
                beagle_query_add_mime_type (beagle_query, g_strdup (value.utf8 ()));
            else if (key == "type")
                beagle_query_add_hit_type (beagle_query, g_strdup (value.utf8 ()));
            else if (key == "source")
                beagle_query_add_source (beagle_query, g_strdup (value.utf8 ()));
            else if (key == "start")
                start_date = value;
            else if (key == "end")
                end_date = value;
            else
                query_terms += *it;
        } else
            query_terms += *it;
    }

    beagle_query_add_text (beagle_query, g_strdup (query_terms.join (" ").utf8 ()));
    kdDebug () << "Adding query text:" << query_terms.join (" ").utf8 () << endl;

    if (start_date.isNull () && end_date.isNull ())
        return beagle_query;

    //kdDebug () << "Handling dates ..." << endl;
    BeagleQueryPartDate * date_part = beagle_query_part_date_new ();
    if (! start_date.isNull ())
        beagle_query_part_date_set_start_date (date_part, timestringToBeagleTimestamp (start_date));
    if (! end_date.isNull ())
        beagle_query_part_date_set_end_date (date_part, timestringToBeagleTimestamp (end_date));
    beagle_query_add_part (beagle_query, BEAGLE_QUERY_PART (date_part));

    return beagle_query;
}

// timestring format allowed YYYYmmDD
BeagleTimestamp *
BeagleUtil::timestringToBeagleTimestamp(TQString timestring)
{
    //kdDebug () << "datetime string:" << timestring << endl;
    // FIXME: error check timestring format
    if (timestring.isNull () || timestring.stripWhiteSpace () == "" || timestring.length() != 8 )
        return beagle_timestamp_new_from_unix_time (TQDateTime::currentDateTime ().toTime_t ());
    //TQDateTime dt = TQDateTime::fromString (timestring, Qt::ISODate);
    struct tm tm_time;
    time_t timet_time;
    time (&timet_time);
    localtime_r (&timet_time, &tm_time);
    strptime (timestring.ascii(), "%Y%m%d", &tm_time);
    tm_time.tm_sec = tm_time.tm_min = tm_time.tm_hour = 0;
    //kdDebug() << asctime (&tm_time) << endl;
    timet_time = mktime (&tm_time);
    return beagle_timestamp_new_from_unix_time (timet_time);
}

