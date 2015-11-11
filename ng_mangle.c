/*
 * Copyright (c) 2004, Dominik Lupinski <dlupinsk@wsb-nlu.edu.pl>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * $Id: ng_mangle.c,v 1.5 2004/02/06 00:08:32 me Exp $
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_var.h>
#include <net/netisr.h>
#include <net/ethernet.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <machine/in_cksum.h>

#include <netgraph/ng_message.h>
#include <netgraph/ng_parse.h>
#include <netgraph/netgraph.h>

#include "ng_mangle.h"



NETGRAPH_INIT(mangle, &mangle_struct);

/*
 * Make node
 */
static int
ng_mangle_constr(node_p node)
{
	mangle_p mp;

	MALLOC(mp, mangle_p, sizeof(*mp), M_NETGRAPH, M_NOWAIT | M_ZERO);
	
	if(mp == NULL) {
		return(ENOMEM);
	}

	NG_NODE_SET_PRIVATE(node, mp);
	mp->node = node;

	return(0);
}

/*
 * Define some hooks
 */
static int
ng_mangle_newhook(node_p node, hook_p hook, const char *name)
{
	const mangle_p mp = NG_NODE_PRIVATE(node);

		
	/* Which hook we need to? */
	if(strcmp(name, NG_MANGLE_HOOK_UPPER) == 0) {
		mp->upper.hook	= hook;
		mp->upper.ttl	= IPDEFTTL;
		mp->upper.tos	= 0;
		NG_HOOK_SET_PRIVATE(hook, &mp->upper);
	}
	else if(strcmp(name, NG_MANGLE_HOOK_LOWER) == 0) {
		mp->lower.hook = hook;
		mp->lower.ttl	= IPDEFTTL;
		mp->lower.tos	= 0;
		NG_HOOK_SET_PRIVATE(hook, &mp->lower);
	}
	else {
		return(EINVAL);
	}
	
	return(0);
}

/*
 * Message handling
 */
static int
ng_mangle_rcvmsg(node_p node, item_p item, hook_p hook)
{
	const mangle_p mp = NG_NODE_PRIVATE(node);
	struct ng_mesg *msg;
	int error = 0;

	NGI_GET_MSG(item, msg);

	/* Take out cookie and command */
	if(msg->header.typecookie == NGM_MANGLE_COOKIE) {
		switch(msg->header.cmd) {
			case NG_MANGLE_SET_TTL_UPPER:

				if(msg->header.arglen != sizeof(u_int8_t)) {
					error = EINVAL;
					break;
				}
				mp->upper.ttl = *((u_int8_t *)&msg->data);
				break;
			
			case NG_MANGLE_SET_TTL_LOWER:

				if(msg->header.arglen != sizeof(u_int8_t)) {
					error = EINVAL;
					break;
				}
				mp->lower.ttl = *((u_int8_t *)&msg->data);
				break;

			case NG_MANGLE_SET_TOS_UPPER:

				if(msg->header.arglen != sizeof(u_int8_t)) {
					error = EINVAL;
					break;
				}
				mp->upper.tos = *((u_int8_t *)&msg->data);
				break;

			case NG_MANGLE_SET_TOS_LOWER:

				if(msg->header.arglen != sizeof(u_int8_t)) {
					error = EINVAL;
					break;
				}
				mp->lower.tos = *((u_int8_t *)&msg->data);
				break;

			default:
				error = EINVAL;
				break;
		}
	}

	NG_FREE_MSG(msg);
	return(error);
};


/*
 * Here is the core. We modify our packets and return pointer 
 * to mbuf later.
 */
static struct mbuf *
ng_mangle_mangle(struct mbuf *mint, struct hookinfo *hi)
{
	struct ether_header *ethdr;
	/* copy center */
	struct ether_header ethdr_saved;
	struct ip *ip;


	/* extra sanity check */
	if((mint->m_len < sizeof(*ethdr)) 
	&& (mint = m_pullup(mint, sizeof(*ethdr))) == NULL) {
		return(NULL);
	}
	ethdr = mtod(mint, struct ether_header *);
	
	if(ntohs(ethdr->ether_type) != ETHERTYPE_IP) {
		/* Oops! We are not invited. */
		return(mint);
	}
	
	/* Yup, since now we have only IPv4 packets to deal with... */

	/*
	 * We need to strip ethernet header off to proceed with ip packets
	 * but later on we will need that part to build the frame back, so 
	 * we copy that stripped part to local structure to have it for
	 * restoring. I've stolen this idea from bridge.c written by 
	 * Luigi Rizzo.
	 */
	bcopy(ethdr, &ethdr_saved, sizeof(struct ether_header));
	m_adj(mint, sizeof(struct ether_header));

	/* extra sanity check */
	if((mint->m_len < sizeof(*ip)) 
	&& (mint = m_pullup(mint, sizeof(*ip))) == NULL) {
		return(NULL);
	}
	ip = mtod(mint, struct ip *);

	/* Here is TTL */
	ip->ip_ttl = hi->ttl;

	/* Here is TOS */
	ip->ip_tos = hi->tos;
	
	/* 
	 * Rebuild checksum. We need to do so as we already modified some.
	 * It's really important to get rid of previous value of ip->ip_sum
	 * before we do so. Otherwise newly computed check sum won't match
	 * anything and packets going out with this value will disapear with
	 * no any icmp from remote side!
	 */
	ip->ip_sum = 0;
	ip->ip_sum = in_cksum_hdr(ip);


	/* RESTORING the frame */
	M_PREPEND(mint, sizeof(struct ether_header), M_DONTWAIT);
	if(mint == NULL) {
		/* Do nothing at this time, we'll handle it outside */
		/* return(mint); */
		return(NULL);
	}
	if(ethdr != mtod(mint, struct ether_header *)) {
		bcopy(&ethdr_saved, mtod(mint, struct ether_header *), sizeof(struct ether_header));
	}
	

	return(mint);
}

/*
 * Receive data
 */
static int
ng_mangle_rcvdata(hook_p hook, item_p item)
{
	
	const mangle_p mp = NG_NODE_PRIVATE(NG_HOOK_NODE(hook));
	struct hookinfo *const hinfo = NG_HOOK_PRIVATE(hook);
	struct hookinfo *dest;
	struct mbuf *m;
	int error = 0;

	
	/* Better use this instead of m = NGI_M(item); if going to modify mbuf */
	NGI_GET_M(item, m);

	if(m == NULL) {
		NG_FREE_ITEM(item);
		NG_FREE_M(m);
		return(ENOBUFS);
	}

	/* We need to find out the direction we are following */
	if(hinfo == &mp->upper) {
		dest = &mp->lower;
		/* FALLTHROUGH */
	}
	else if(hinfo == &mp->lower) {
		dest = &mp->upper;
		goto proceed;
	}
	else {
		panic("%s: no hook!", __func__);
	}

	/*
	 * OK, so if we are here already we are sure to go downstream, outside
	 * our machine, down the wild, etc. ;) Incomming packets skip this
	 * area and goto proceed.
	 */
		
	m = ng_mangle_mangle(m, dest);

	if(m == NULL) {
		NG_FREE_ITEM(item);
		NG_FREE_M(m);
		return(ENOBUFS);
	}

	
proceed:
	/* Set the right hook for a destination */
	if(dest->hook) {

		/* Forward possibly modified data */
		NG_FWD_NEW_DATA(error, item, dest->hook, m);
	}
	else {
		NG_FREE_ITEM(item);
		NG_FREE_M(m);
	}

	return(error);
}

/*
 * Shut the node up
 */
static int
ng_mangle_shutdown(node_p node)
{
	const mangle_p mp = NG_NODE_PRIVATE(node);

	NG_NODE_SET_PRIVATE(node, NULL);
	NG_NODE_UNREF(mp->node);
	FREE(mp, M_NETGRAPH);

	return(0);
}

/*
 * Hook disconnection
 */
static int
ng_mangle_disconnect(hook_p hook)
{
	/* const mangle_p mp = NG_NODE_PRIVATE(NG_HOOK_NODE(hook)); */
	
	if((NG_NODE_NUMHOOKS(NG_HOOK_NODE(hook)) == 0) 
	&& (NG_NODE_IS_VALID(NG_HOOK_NODE(hook))))
		ng_rmnode_self(NG_HOOK_NODE(hook));
	
	return (0);
}

/*****************************************************************************
 *	<quote>                                                              *
 *	                                                                     *
 *		[...]                                                        *
 *		I feel the weight of the world on my shoulder                *
 *		As I'm gettin' older, y'all, people gets colder              *
 *		Most of us only care about money makin'                      *
 *		Selfishness got us followin' our own direction               *
 *		Wrong information always shown by the media                  *
 *		Negative images is the main criteria                         *
 *		Infecting the young minds faster than bacteria               *
 *		Kids act like what they see in the cinema                    *
 *		Yo', whatever happened to the values of humanity             *
 *		Whatever happened to the fairness in equality                *
 *		Instead in spreading love we spreading animosity             *
 *		Lack of understanding, leading lives away from unity         *
 *		That's the reason why sometimes I'm feelin' under            *
 *		That's the reason why sometimes I'm feelin' down             *
 *		There's no wonder why sometimes I'm feelin' under            *
 *		[...]                                                        *
 *                                                                           *
 *			-- Black Eyed Peas - Where Is The Love               *
 *	</quote>                                                             *
 *	                                                                     *
 *	                         -- Good night...                            *
 *	                                                                     *
 ****************************************************************************/

