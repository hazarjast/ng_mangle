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
 * $Id: ng_mangle.h,v 1.3 2004/01/29 04:28:38 me Exp $
 *
 */

#ifndef _NETGRAPH_NG_MANGLE_H_
#define _NETGRAPH_NG_MANGLE_H_



/* Node type and cookie for ng_mangle */
#define NG_MANGLE_NODE_TYPE	"mangle"
#define NGM_MANGLE_COOKIE	200401025

/* Whookcares? */
#define NG_MANGLE_HOOK_UPPER	"upper"
#define NG_MANGLE_HOOK_LOWER	"lower"


/*
 * Netgraph commands
 */
enum {
	NG_MANGLE_SET_TTL_UPPER = 1,
	NG_MANGLE_SET_TTL_LOWER,
	NG_MANGLE_SET_TOS_UPPER,
	NG_MANGLE_SET_TOS_LOWER
};


static const struct ng_cmdlist ng_mangle_cmdlist[] = {
	{
		NGM_MANGLE_COOKIE,
		NG_MANGLE_SET_TTL_UPPER,
		"set_ttl_upper",
		&ng_parse_uint8_type,
		NULL
	},
	{
		NGM_MANGLE_COOKIE,
		NG_MANGLE_SET_TTL_LOWER,
		"set_ttl_lower",
		&ng_parse_uint8_type,
		NULL
	}, 
	{
		NGM_MANGLE_COOKIE,
		NG_MANGLE_SET_TOS_UPPER,
		"set_tos_upper",
		&ng_parse_hint8_type,
		NULL
	}, 
	{
		NGM_MANGLE_COOKIE,
		NG_MANGLE_SET_TOS_LOWER,
		"set_tos_lower",
		&ng_parse_hint8_type,
		NULL
	}, 
	{ 0 }
};


/*
 * Info about hooks
 */
struct hookinfo {
	hook_p	hook;
	u_char	ttl;
	u_char	tos;
};

/*
 * Private data for hooks
 */
struct mangle_priv {
	node_p			node;
	struct hookinfo		upper;
	struct hookinfo		lower;
};
typedef struct mangle_priv *mangle_p;

/* 
 * Method declarations 
 */
static ng_constructor_t	ng_mangle_constr;
static ng_rcvmsg_t	ng_mangle_rcvmsg;
static ng_newhook_t	ng_mangle_newhook;
static ng_rcvdata_t	ng_mangle_rcvdata;
static ng_shutdown_t	ng_mangle_shutdown;
static ng_disconnect_t	ng_mangle_disconnect;

/*
 * Mangle node type
 */
static struct ng_type mangle_struct = {
	NG_ABI_VERSION,
	NG_MANGLE_NODE_TYPE,
	NULL,			/* modeventhand_t	*/
	ng_mangle_constr,	/* ng_constructor_t	*/
	ng_mangle_rcvmsg,	/* ng_rcvmsg_t		*/
	NULL,			/* ng_close_t		*/
	ng_mangle_shutdown,	/* ng_shutdown_t	*/
	ng_mangle_newhook,	/* ng_newhook_t		*/
	NULL,			/* ng_findhook_t	*/
	NULL,			/* ng_connect_t		*/
	ng_mangle_rcvdata,	/* ng_rcvdata_t		*/
	ng_mangle_disconnect,	/* ng_disconnect_t	*/
	ng_mangle_cmdlist,	/* ng_cmdlist		*/
};


#endif /* _NETGRAPH_NG_MANGLE_H_ */
