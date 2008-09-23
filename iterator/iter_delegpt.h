/*
 * iterator/iter_delegpt.h - delegation point with NS and address information.
 *
 * Copyright (c) 2007, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * This file implements the Delegation Point. It contains a list of name servers
 * and their addresses if known.
 */

#ifndef ITERATOR_ITER_DELEGPT_H
#define ITERATOR_ITER_DELEGPT_H
#include "util/log.h"
struct regional;
struct delegpt_ns;
struct delegpt_addr;
struct dns_msg;
struct ub_packed_rrset_key;

/**
 * Delegation Point.
 * For a domain name, the NS rrset, and the A and AAAA records for those.
 */
struct delegpt {
	/** the domain name of the delegation point. */
	uint8_t* name;
	/** length of the delegation point name */
	size_t namelen;
	/** number of labels in delegation point */
	int namelabs;

	/** the nameservers, names from the NS RRset rdata. */
	struct delegpt_ns* nslist;
	/** the target addresses for delegation */
	struct delegpt_addr* target_list;
	/** the list of usable targets; subset of target_list 
	 * the items in this list are not part of the result list.  */
	struct delegpt_addr* usable_list;
	/** the list of returned targets; subset of target_list */
	struct delegpt_addr* result_list;
};

/**
 * Nameservers for a delegation point.
 */
struct delegpt_ns {
	/** next in list */
	struct delegpt_ns* next;
	/** name of nameserver */
	uint8_t* name;
	/** length of name */
	size_t namelen;
	/** 
	 * If the name has been resolved. false if not queried for yet.
	 * true if the address is known, or marked true if failed.
	 */
	int resolved;
};

/**
 * Address of target nameserver in delegation point.
 */
struct delegpt_addr {
	/** next delegation point in results */
	struct delegpt_addr* next_result;
	/** next delegation point in usable list */
	struct delegpt_addr* next_usable;
	/** next delegation point in all targets list */
	struct delegpt_addr* next_target;

	/** delegation point address */
	struct sockaddr_storage addr;
	/** length of addr */
	socklen_t addrlen;
	/** number of attempts for this addr */
	int attempts;
	/** rtt stored here in the selection algorithm */
	int sel_rtt;
};

/**
 * Create new delegation point.
 * @param regional: where to allocate it.
 * @return new delegation point or NULL on error.
 */
struct delegpt* delegpt_create(struct regional* regional);

/**
 * Create a copy of a delegation point.
 * @param dp: delegation point to copy.
 * @param regional: where to allocate it.
 * @return new delegation point or NULL on error.
 */
struct delegpt* delegpt_copy(struct delegpt* dp, struct regional* regional);

/**
 * Set name of delegation point.
 * @param dp: delegation point.
 * @param regional: where to allocate the name copy.
 * @param name: name to use.
 * @return false on error.
 */
int delegpt_set_name(struct delegpt* dp, struct regional* regional, 
	uint8_t* name);

/**
 * Add a name to the delegation point.
 * @param dp: delegation point.
 * @param regional: where to allocate the info.
 * @param name: domain name in wire format.
 * @return false on error.
 */
int delegpt_add_ns(struct delegpt* dp, struct regional* regional, 
	uint8_t* name);

/**
 * Add NS rrset; calls add_ns repeatedly.
 * @param dp: delegation point.
 * @param regional: where to allocate the info.
 * @param ns_rrset: NS rrset.
 * return 0 on alloc error.
 */
int delegpt_rrset_add_ns(struct delegpt* dp, struct regional* regional,
	struct ub_packed_rrset_key* ns_rrset);

/**
 * Add target address to the delegation point.
 * @param dp: delegation point.
 * @param regional: where to allocate the info.
 * @param name: name for which target was found (must be in nslist).
 *	This name is marked resolved.
 * @param namelen: length of name.
 * @param addr: the address.
 * @param addrlen: the length of addr.
 * @return false on error.
 */
int delegpt_add_target(struct delegpt* dp, struct regional* regional, 
	uint8_t* name, size_t namelen, struct sockaddr_storage* addr, 
	socklen_t addrlen);

/**
 * Add A RRset to delegpt.
 * @param dp: delegation point.
 * @param regional: where to allocate the info.
 * @param rrset: RRset A to add.
 * @return 0 on alloc error.
 */
int delegpt_add_rrset_A(struct delegpt* dp, struct regional* regional, 
	struct ub_packed_rrset_key* rrset);

/**
 * Add AAAA RRset to delegpt.
 * @param dp: delegation point.
 * @param regional: where to allocate the info.
 * @param rrset: RRset AAAA to add.
 * @return 0 on alloc error.
 */
int delegpt_add_rrset_AAAA(struct delegpt* dp, struct regional* regional, 
	struct ub_packed_rrset_key* rrset);

/**
 * Add any RRset to delegpt.
 * @param dp: delegation point.
 * @param regional: where to allocate the info.
 * @param rrset: RRset to add, NS, A, AAAA.
 * @return 0 on alloc error.
 */
int delegpt_add_rrset(struct delegpt* dp, struct regional* regional, 
	struct ub_packed_rrset_key* rrset);

/**
 * Add address to the delegation point. No servername is associated or checked.
 * @param dp: delegation point.
 * @param regional: where to allocate the info.
 * @param addr: the address.
 * @param addrlen: the length of addr.
 * @return false on error.
 */
int delegpt_add_addr(struct delegpt* dp, struct regional* regional, 
	struct sockaddr_storage* addr, socklen_t addrlen);

/** 
 * Find NS record in name list of delegation point.
 * @param dp: delegation point.
 * @param name: name of nameserver to look for, uncompressed wireformat.
 * @param namelen: length of name.
 * @return the ns structure or NULL if not found.
 */
struct delegpt_ns* delegpt_find_ns(struct delegpt* dp, uint8_t* name, 
	size_t namelen);

/**
 * Print the delegation point to the log. For debugging.
 * @param v: verbosity value that is needed to emit to log.
 * @param dp: delegation point.
 */
void delegpt_log(enum verbosity_value v, struct delegpt* dp);

/** count NS and number missing for logging */
void delegpt_count_ns(struct delegpt* dp, size_t* numns, size_t* missing);

/** count addresses, and number in result and available lists, for logging */
void delegpt_count_addr(struct delegpt* dp, size_t* numaddr, size_t* numres, 
	size_t* numavail);

/**
 * Add all usable targets to the result list.
 * @param dp: delegation point.
 */
void delegpt_add_unused_targets(struct delegpt* dp);

/**
 * Count number of missing targets. These are ns names with no resolved flag.
 * @param dp: delegation point.
 * @return number of missing targets (or 0).
 */
size_t delegpt_count_missing_targets(struct delegpt* dp);

/**
 * Create new delegation point from a dns message
 *
 * Note that this method does not actually test to see if the message is an
 * actual referral. It really is just checking to see if it can construct a
 * delegation point, so the message could be of some other type (some ANSWER
 * messages, some CNAME messages, generally.) Note that the resulting
 * DelegationPoint will contain targets for all "relevant" glue (i.e.,
 * address records whose ownernames match the target of one of the NS
 * records), so if policy dictates that some glue should be discarded beyond
 * that, discard it before calling this method. Note that this method will
 * find "glue" in either the ADDITIONAL section or the ANSWER section.
 *
 * @param msg: the dns message, referral.
 * @param regional: where to allocate delegation point.
 * @return new delegation point or NULL on alloc error, or if the
 *         message was not appropriate.
 */
struct delegpt* delegpt_from_message(struct dns_msg* msg, 
	struct regional* regional);

#endif /* ITERATOR_ITER_DELEGPT_H */
