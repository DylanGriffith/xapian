/** @file localsubmatch.cc
 *  @brief SubMatch class for a local database.
 */
/* Copyright (C) 2006,2007,2009,2010,2011,2013,2014,2015 Olly Betts
 * Copyright (C) 2007,2008,2009 Lemur Consulting Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <config.h>

#include "localsubmatch.h"

#include "backends/database.h"
#include "debuglog.h"
#include "api/emptypostlist.h"
#include "extraweightpostlist.h"
#include "api/leafpostlist.h"
#include "omassert.h"
#include "queryoptimiser.h"
#include "synonympostlist.h"
#include "api/termlist.h"
#include "weight/weightinternal.h"

#include "autoptr.h"
#include <map>
#include <string>

using namespace std;

/** Xapian::Weight subclass which adds laziness.
 *
 *  For terms from a wildcard when remote databases are involved, we need to
 *  delay calling init_() on the weight object until the stats for the terms
 *  from the wildcard have been collated.
 */
class LazyWeight : public Xapian::Weight {
    LeafPostList * pl;

    Xapian::Weight * real_wt;

    Xapian::Weight::Internal * stats;

    Xapian::termcount qlen;

    std::string term;

    Xapian::termcount wqf;

    double factor;

    LazyWeight * clone() const;

    void init(double factor_);

  public:
    LazyWeight(LeafPostList * pl_,
	       Xapian::Weight * real_wt_,
	       Xapian::Weight::Internal * stats_,
	       Xapian::termcount qlen_,
	       const std::string & term_,
	       Xapian::termcount wqf__,
	       double factor_)
	: pl(pl_),
	  real_wt(real_wt_),
	  stats(stats_),
	  qlen(qlen_),
	  term(term_),
	  wqf(wqf__),
	  factor(factor_)
    { }

    std::string name() const;

    std::string serialise() const;
    LazyWeight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms) const;
    double get_maxpart() const;

    double get_sumextra(Xapian::termcount doclen,
			Xapian::termcount uniqterms) const;
    double get_maxextra() const;
};

LazyWeight *
LazyWeight::clone() const
{
    throw Xapian::InvalidOperationError("LazyWeight::clone()");
}

void
LazyWeight::init(double factor_)
{
    (void)factor_;
    throw Xapian::InvalidOperationError("LazyWeight::init()");
}

string
LazyWeight::name() const
{
    string desc = "LazyWeight(";
    desc += real_wt->name();
    desc += ")";
    return desc;
}

string
LazyWeight::serialise() const
{
    throw Xapian::InvalidOperationError("LazyWeight::serialise()");
}

LazyWeight *
LazyWeight::unserialise(const string &) const
{
    throw Xapian::InvalidOperationError("LazyWeight::unserialise()");
}

double
LazyWeight::get_sumpart(Xapian::termcount wdf,
			Xapian::termcount doclen,
			Xapian::termcount uniqterms) const
{
    (void)wdf;
    (void)doclen;
    (void)uniqterms;
    throw Xapian::InvalidOperationError("LazyWeight::get_sumpart()");
}

double
LazyWeight::get_maxpart() const
{
    // This gets called first for the case we care about.
    // FIXME: We wouldn't need "term" here if we did this in
    // resolve_lazy_termweight() instead.
    real_wt->init_(*stats, qlen, term, wqf, factor);
    return pl->resolve_lazy_termweight(real_wt, stats);
}

double
LazyWeight::get_sumextra(Xapian::termcount doclen,
			 Xapian::termcount uniqterms) const
{
    (void)doclen;
    (void)uniqterms;
    throw Xapian::InvalidOperationError("LazyWeight::get_sumextra()");
}

double
LazyWeight::get_maxextra() const
{
    throw Xapian::InvalidOperationError("LazyWeight::get_maxextra()");
}

bool
LocalSubMatch::prepare_match(bool nowait,
			     Xapian::Weight::Internal & total_stats)
{
    LOGCALL(MATCH, bool, "LocalSubMatch::prepare_match", nowait | total_stats);
    (void)nowait;
    Assert(db);
    total_stats.accumulate_stats(*db, rset);
    RETURN(true);
}

void
LocalSubMatch::start_match(Xapian::doccount first,
			   Xapian::doccount maxitems,
			   Xapian::doccount check_at_least,
			   Xapian::Weight::Internal & total_stats)
{
    LOGCALL_VOID(MATCH, "LocalSubMatch::start_match", first | maxitems | check_at_least | total_stats);
    (void)first;
    (void)maxitems;
    (void)check_at_least;
    // Store a pointer to the total stats to use when building the Query tree.
    stats = &total_stats;
}

PostList *
LocalSubMatch::get_postlist(MultiMatch * matcher,
			    Xapian::termcount * total_subqs_ptr)
{
    LOGCALL(MATCH, PostList *, "LocalSubMatch::get_postlist", matcher | total_subqs_ptr);

    if (query.empty())
	RETURN(new EmptyPostList); // MatchNothing

    // Build the postlist tree for the query.  This calls
    // LocalSubMatch::open_post_list() for each term in the query.
    PostList * pl;
    {
	QueryOptimiser opt(*db, *this, matcher);
	pl = query.internal->postlist(&opt, 1.0);
	*total_subqs_ptr = opt.get_total_subqs();
    }

    AutoPtr<Xapian::Weight> extra_wt(wt_factory->clone());
    extra_wt->init_(*stats, qlen); // Only uses term-indep. stats.
    if (extra_wt->get_maxextra() != 0.0) {
	// There's a term-independent weight contribution, so we combine the
	// postlist tree with an ExtraWeightPostList which adds in this
	// contribution.
	pl = new ExtraWeightPostList(pl, extra_wt.release(), matcher);
    }

    RETURN(pl);
}

PostList *
LocalSubMatch::make_synonym_postlist(PostList * or_pl, MultiMatch * matcher,
				     double factor)
{
    LOGCALL(MATCH, PostList *, "LocalSubMatch::make_synonym_postlist", or_pl | matcher | factor);
    if (rare(or_pl->get_termfreq_max() == 0)) {
	// or_pl is an EmptyPostList or equivalent.
	return or_pl;
    }
    LOGVALUE(MATCH, or_pl->get_termfreq_est());
    Xapian::termcount len_lb = db->get_doclength_lower_bound();
    AutoPtr<SynonymPostList> res(new SynonymPostList(or_pl, matcher, len_lb));

    // FIXME: do this part below later?
    AutoPtr<Xapian::Weight> wt(wt_factory->clone());

    TermFreqs freqs;
    // Avoid calling get_termfreq_est_using_stats() if the database is empty
    // so we don't need to special case that repeatedly when implementing it.
    // FIXME: it would be nicer to handle an empty database higher up, though
    // we need to catch the case where all the non-empty subdatabases have
    // failed, so we can't just push this right up to the start of get_mset().
    if (usual(stats->collection_size != 0)) {
	// FIXME: needs term-dep. stats.
	freqs = or_pl->get_termfreq_est_using_stats(*stats);
    }
    wt->init_(*stats, qlen, factor,
	      freqs.termfreq, freqs.reltermfreq, freqs.collfreq);

    res->set_weight(wt.release());
    RETURN(res.release());
}

LeafPostList *
LocalSubMatch::open_post_list(const string& term,
			      Xapian::termcount wqf,
			      double factor,
			      bool need_positions,
			      LeafPostList ** hint)
{
    LOGCALL(MATCH, LeafPostList *, "LocalSubMatch::open_post_list", term | wqf | factor | need_positions | hint);

    bool weighted = (factor != 0.0 && !term.empty());
    AutoPtr<Xapian::Weight> wt(weighted ? wt_factory->clone() : NULL);
    if (weighted) {
	wt->init_(*stats, qlen, term, wqf, factor);
	stats->set_max_part(term, wt->get_maxpart());
    }

    LeafPostList * pl = NULL;
    if (!term.empty() && !need_positions) {
	if (!weighted || !wt_factory->get_sumpart_needs_wdf_()) {
	    Xapian::doccount sub_tf;
	    db->get_freqs(term, &sub_tf, NULL);
	    if (sub_tf == db->get_doccount()) {
		// If we're not going to use the wdf or term positions, and the
		// term indexes all documents, we can replace it with the
		// MatchAll postlist, which is especially efficient if there
		// are no gaps in the docids.
		pl = db->open_post_list(string());
	    }
	}
    }

    if (!pl) {
	if (*hint)
	    pl = (*hint)->open_nearby_postlist(term);
	if (!pl)
	    pl = db->open_post_list(term);
	*hint = pl;
    }

    if (weighted)
	pl->set_termweight(wt.release());
    RETURN(pl);
}
