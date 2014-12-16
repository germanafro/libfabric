/*
 * Copyright (c) 2014 Intel Corporation, Inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <rdma/fabric.h>
#include <rdma/fi_atomic.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_prov.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_trigger.h>

#include <fi.h>
#include <fi_enosys.h>
#include <fi_indexer.h>
#include "list.h"
#include <fi_rbuf.h>
#include <fi_list.h>

#ifndef _SOCK_H_
#define _SOCK_H_

#define SOCK_EP_MAX_MSG_SZ (1<<23)
#define SOCK_EP_MAX_INJECT_SZ ((1<<8) - 1)
#define SOCK_EP_MAX_BUFF_RECV (1<<23)
#define SOCK_EP_MAX_ORDER_RAW_SZ (0)
#define SOCK_EP_MAX_ORDER_WAR_SZ (0)
#define SOCK_EP_MAX_ORDER_WAW_SZ (0)
#define SOCK_EP_MEM_TAG_FMT (0)
#define SOCK_EP_MSG_ORDER (0)
#define SOCK_EP_MAX_EP_CNT (128)
#define SOCK_EP_MAX_TX_CNT (16)
#define SOCK_EP_MAX_RX_CNT (16)
#define SOCK_EP_MAX_IOV_LIMIT (8)
#define SOCK_EP_MAX_TX_CTX_SZ (1<<12)
#define SOCK_EP_MIN_MULTI_RECV (64)
#define SOCK_EP_MAX_ATOMIC_SZ (512)

#define SOCK_PE_POLL_TIMEOUT (100000)
#define SOCK_PE_MAX_ENTRIES (128)

#define SOCK_EQ_DEF_SZ (1<<8)
#define SOCK_CQ_DEF_SZ (1<<8)


#define SOCK_EP_RDM_CAP (FI_MSG | FI_RMA | FI_TAGGED | FI_ATOMICS | FI_DYNAMIC_MR | \
			 FI_NAMED_RX_CTX | FI_BUFFERED_RECV | FI_DIRECTED_RECV | \
			 FI_INJECT | FI_MULTI_RECV | FI_SOURCE | FI_READ | FI_WRITE | \
			 FI_RECV | FI_SEND | FI_REMOTE_READ | FI_REMOTE_WRITE |	\
			 FI_REMOTE_CQ_DATA | FI_COMPLETION | FI_REMOTE_SIGNAL |	\
			 FI_REMOTE_COMPLETE | FI_PEEK | FI_CANCEL)

#define SOCK_EP_MSG_CAP SOCK_EP_RDM_CAP

#define SOCK_EP_DGRAM_CAP (FI_MSG | FI_TAGGED | FI_DYNAMIC_MR | \
			   FI_NAMED_RX_CTX | FI_BUFFERED_RECV | FI_DIRECTED_RECV | \
			   FI_INJECT | FI_MULTI_RECV | FI_SOURCE | FI_RECV | FI_SEND | \
			   FI_REMOTE_CQ_DATA | FI_COMPLETION | FI_REMOTE_SIGNAL | \
			   FI_REMOTE_COMPLETE | FI_PEEK | FI_CANCEL)

#define SOCK_DEF_OPS (FI_SEND | FI_RECV |			\
		      FI_BUFFERED_RECV | FI_READ | FI_WRITE |	\
		      FI_REMOTE_READ | FI_REMOTE_WRITE )

#define SOCK_MODE (0)

#define SOCK_COMM_BUF_SZ (SOCK_EP_MAX_MSG_SZ)
#define SOCK_COMM_THRESHOLD (128 * 1024)

#define SOCK_MAJOR_VERSION 1
#define SOCK_MINOR_VERSION 0

extern const char const sock_fab_name[];
extern const char const sock_dom_name[];

struct sock_fabric{
	struct fid_fabric fab_fid;
	atomic_t ref;
};

struct sock_conn {
        int sock_fd;
};

struct sock_conn_map {
        struct sock_conn *table;
        int used;
        int size;
		struct sock_domain *domain;
};

struct sock_domain {
	struct fi_info info;
	struct fid_domain dom_fid;
	struct sock_fabric *fab;
	fastlock_t lock;
	atomic_t ref;

	struct sock_eq *eq;
	struct sock_eq *mr_eq;

	struct sock_pe *pe;
	struct index_map mr_idm;

	struct sock_conn_map u_cmap;
	struct sock_conn_map r_cmap;
};

struct sock_cntr {
	struct fid_cntr		cntr_fid;
	struct sock_domain	*domain;
	uint64_t		value;
	uint64_t		threshold;
	atomic_t		ref;
	atomic_t err_cnt;
	pthread_cond_t		cond;
	pthread_mutex_t		mut;
	struct dlist_entry rx_list;
	struct dlist_entry tx_list;
};

struct sock_mr {
	struct fid_mr		mr_fid;
	struct sock_domain	*dom;
	uint64_t		access;
	uint64_t		offset;
	uint64_t		key;
	size_t			iov_count;
	struct iovec		mr_iov[1];
};

struct sock_av {
	struct fid_av		av_fid;
	struct sock_domain	*domain;
	atomic_t		ref;
	struct fi_av_attr	attr;
	size_t			count;
	struct sockaddr_in	*table;
	struct sock_conn_map	*cmap;
};

struct sock_poll {
	struct fid_poll		poll_fid;
	struct sock_domain	*dom;
};

struct sock_wait {
	struct fid_wait wait_fid;
	struct sock_domain *dom;
};

enum {
	/* wire protocol */
	SOCK_OP_SEND = 0,
	SOCK_OP_TSEND = 1,
	SOCK_OP_SEND_COMPLETE = 2,

	SOCK_OP_WRITE = 3,
	SOCK_OP_WRITE_COMPLETE = 4,
	SOCK_OP_WRITE_ERROR = 5,

	SOCK_OP_READ = 6,
	SOCK_OP_READ_COMPLETE = 7,
	SOCK_OP_READ_ERROR = 8,

	SOCK_OP_ATOMIC_WRITE = 9,
	SOCK_OP_ATOMIC_READ_WRITE = 10,
	SOCK_OP_ATOMIC_COMP_WRITE = 11,

	SOCK_OP_ATOMIC_COMPLETE = 12,
	SOCK_OP_ATOMIC_ERROR = 13,

	/* internal */
	SOCK_OP_RECV,
	SOCK_OP_TRECV,
};

/*
 * Transmit context - ring buffer data:
 *    tx_op + flags + context + dest_addr + conn + [data] + [tag] + tx_iov
 *     8B       8B      8B         8B         8B       8B      24B+
 * data - only present if flags indicate
 * tag - only present for TSEND op
 */
struct sock_op {
	uint8_t			op;
	uint8_t			src_iov_len;
	uint8_t			dest_iov_len;
	union {
		struct {
			uint8_t	op;
			uint8_t	datatype;
			uint8_t	res_iov_len;
			uint8_t	cmp_iov_len;
		} atomic;
		uint8_t		reserved[5];
	};
};

struct sock_op_send {
	struct sock_op op;
	uint64_t flags;
	uint64_t context;
	uint64_t dest_addr;
	struct sock_conn *conn;
	uint64_t buf;
	struct sock_ep *ep;
};

struct sock_op_tsend {
	struct sock_op op;
	uint64_t flags;
	uint64_t context;
	uint64_t dest_addr;
	struct sock_conn *conn;
	uint64_t tag;
	uint64_t buf;
	struct sock_ep *ep;
};

union sock_iov {
	struct fi_rma_iov	iov;
	struct fi_rma_ioc	ioc;
};

struct sock_rxtx {
	struct ringbuffd	rbfd;
	fastlock_t		wlock;
	fastlock_t		rlock;
};

struct sock_eq_entry{
	uint32_t type;
	size_t len;
	uint64_t flags;
	struct dlist_entry entry;
	char event[0];
};

struct sock_eq{
	struct fid_eq eq;
	struct fi_eq_attr attr;
	struct sock_fabric *sock_fab;

	struct dlistfd_head list;
	struct dlistfd_head err_list;
	fastlock_t lock;
};

struct sock_comp {
	uint8_t send_cq_event;
	uint8_t recv_cq_event;
	uint8_t read_cq_event;
	uint8_t write_cq_event;
	uint8_t rem_read_cq_event;
	uint8_t rem_write_cq_event;
	char reserved[2];

	struct sock_cq	*send_cq;
	struct sock_cq	*recv_cq;
	struct sock_cq	*read_cq;
	struct sock_cq	*write_cq;
	struct sock_cq *rem_read_cq;
	struct sock_cq *rem_write_cq;

	struct sock_cntr *send_cntr;
	struct sock_cntr *recv_cntr;
	struct sock_cntr *read_cntr;
	struct sock_cntr *write_cntr;
	struct sock_cntr *rem_read_cntr;
	struct sock_cntr *rem_write_cntr;

	struct sock_eq *eq;
};

struct sock_ep {
	union{
		struct fid_ep ep;
		struct fid_sep sep;
		struct fid_pep pep;
	};
	size_t fclass;
	uint64_t op_flags;

	uint16_t buffered_len;
	uint16_t min_multi_recv;
	char reserved[4];

	atomic_t ref;
	struct sock_comp comp;

	struct sock_eq *eq;
	struct sock_av *av;
	struct sock_domain *domain;	

	struct sock_rx_ctx *rx_ctx;
	struct sock_tx_ctx *tx_ctx;

	struct sock_rx_ctx **rx_array;
	struct sock_tx_ctx **tx_array;
	atomic_t num_rx_ctx;
	atomic_t num_tx_ctx;

	struct dlist_entry rx_ctx_entry;
	struct dlist_entry tx_ctx_entry;

	struct fi_info info;
	struct fi_ep_attr ep_attr;
	struct fi_tx_attr tx_attr;
	struct fi_rx_attr rx_attr;

	enum fi_ep_type ep_type;
	struct sockaddr_in *src_addr;
	struct sockaddr_in *dest_addr;
	fi_addr_t conn_addr;
};


struct sock_pep {
	struct fid_pep		pep;
	struct sock_domain  *dom;
	
	int sock_fd;

	struct sock_eq 	*eq;

	struct sock_cq 	*send_cq;
	struct sock_cq 	*recv_cq;

	uint64_t			op_flags;
	uint64_t			pep_cap;
};

struct sock_rx_entry {
	struct sock_op rx_op;
	uint8_t is_buffered;
	uint8_t is_busy;
	uint8_t is_claimed;
	uint8_t reserved[5];

	uint64_t used;
	uint64_t total_len;

	uint64_t flags;
	uint64_t context;
	uint64_t addr;
	uint64_t data;
	uint64_t tag;
	uint64_t ignore;
	struct sock_comp *comp;
	
	union sock_iov iov[SOCK_EP_MAX_IOV_LIMIT];
	struct dlist_entry entry;
};

struct sock_rx_ctx {
	struct fid_ep ctx;

	uint16_t rx_id;
	uint8_t enabled;
	uint8_t progress;

	uint8_t recv_cq_event;
	uint8_t rem_read_cq_event;
	uint8_t rem_write_cq_event;
	uint16_t buffered_len;
	uint16_t min_multi_recv;
	uint8_t reserved[7];

	uint64_t addr;
	struct sock_comp comp;

	struct sock_ep *ep;
	struct sock_av *av;
	struct sock_eq *eq;
 	struct sock_domain *domain;

	struct dlist_entry pe_entry;
	struct dlist_entry cq_entry;
	struct dlist_entry cntr_entry;

	struct dlist_entry pe_entry_list;
	struct dlist_entry rx_entry_list;
	struct dlist_entry rx_buffered_list;
	struct dlist_entry ep_list;
	fastlock_t lock;

	struct fi_rx_attr attr;
};

struct sock_tx_ctx {
	union {
		struct fid_ep ctx;
		struct fid_stx stx;
	};
	size_t fclass;

	struct ringbuffd	rbfd;
	fastlock_t		wlock;
	fastlock_t		rlock;

	uint16_t tx_id;
	uint8_t enabled;
	uint8_t progress;

	uint64_t addr;
	struct sock_comp comp;

	struct sock_ep *ep;
	struct sock_av *av;
	struct sock_eq *eq;
 	struct sock_domain *domain;

	struct dlist_entry pe_entry;
	struct dlist_entry cq_entry;
	struct dlist_entry cntr_entry;

	struct dlist_entry pe_entry_list;
	struct dlist_entry ep_list;
	fastlock_t lock;

	struct fi_tx_attr attr;
};

#define SOCK_WIRE_PROTO_VERSION (0)

struct sock_msg_hdr{
	uint8_t version;
	uint8_t op_type;
	uint16_t rx_id;
	uint8_t reserved[4];

	uint64_t src_addr;
	uint64_t flags;
	uint64_t msg_len;
};

struct sock_msg_send{
	struct sock_msg_hdr msg_hdr;
	/* data */
	/* user data */
};

struct sock_tx_iov {
	union sock_iov src;
	union sock_iov dst;
};

struct sock_tx_pe_entry{
	struct sock_op tx_op;	
	uint8_t header_sent;
	uint8_t reserved[7];

	union {
		struct sock_tx_iov tx_iov[SOCK_EP_MAX_IOV_LIMIT];
		char inject_data[SOCK_EP_MAX_INJECT_SZ];
	};
};

struct sock_rx_pe_entry{
	struct sock_op rx_op;
	void *raw_data;
	union sock_iov rx_iov[SOCK_EP_MAX_IOV_LIMIT];
};

/* PE entry type */
enum{
	SOCK_PE_RX,
	SOCK_PE_TX,
};

struct sock_pe_entry{
	union{
		struct sock_tx_pe_entry tx;
		struct sock_rx_pe_entry rx;
	};

	struct sock_msg_hdr msg_hdr;

	uint64_t flags;
	uint64_t context;
	uint64_t addr;
	uint64_t data;
	uint64_t tag;

	uint8_t type;
	uint8_t reserved[7];

	uint64_t done_len;
	struct sock_ep *ep;
	struct sock_cq *cq;
	struct dlist_entry entry;
};

typedef int (*sock_cq_report_fn) (struct sock_cq *cq, fi_addr_t addr,
				  struct sock_pe_entry *pe_entry);
struct sock_cq {
	struct fid_cq cq_fid;
	struct sock_domain *domain;
	ssize_t cq_entry_size;
	atomic_t ref;
	struct fi_cq_attr attr;

	struct ringbuf addr_rb;
	struct ringbuffd cq_rbfd;
	struct ringbuf cqerr_rb;
	fastlock_t lock;

	struct dlist_entry ep_list;
	struct dlist_entry rx_list;
	struct dlist_entry tx_list;

	sock_cq_report_fn report_completion;
};

int sock_verify_info(struct fi_info *hints);
int sock_verify_fabric_attr(struct fi_fabric_attr *attr);
int sock_verify_domain_attr(struct fi_domain_attr *attr);

int sock_rdm_verify_ep_attr(struct fi_ep_attr *ep_attr, struct fi_tx_attr *tx_attr,
			    struct fi_rx_attr *rx_attr);
int sock_dgram_verify_ep_attr(struct fi_ep_attr *ep_attr, struct fi_tx_attr *tx_attr,
			      struct fi_rx_attr *rx_attr);
int sock_msg_verify_ep_attr(struct fi_ep_attr *ep_attr, struct fi_tx_attr *tx_attr,
			    struct fi_rx_attr *rx_attr);


struct fi_info *sock_fi_info(enum fi_ep_type ep_type, 
			     struct fi_info *hints, void *src_addr, void *dest_addr);
int sock_rdm_getinfo(uint32_t version, const char *node, const char *service,
		     uint64_t flags, struct fi_info *hints, struct fi_info **info);
int sock_dgram_getinfo(uint32_t version, const char *node, const char *service,
		       uint64_t flags, struct fi_info *hints, struct fi_info **info);
int sock_msg_getinfo(uint32_t version, const char *node, const char *service,
		       uint64_t flags, struct fi_info *hints, struct fi_info **info);

int sock_alloc_endpoint(struct fid_domain *domain, struct fi_info *info,
		  struct sock_ep **ep, void *context, size_t fclass);
int sock_rdm_ep(struct fid_domain *domain, struct fi_info *info,
		struct fid_ep **ep, void *context);
int sock_rdm_sep(struct fid_domain *domain, struct fi_info *info,
		 struct fid_sep **sep, void *context);

int sock_dgram_ep(struct fid_domain *domain, struct fi_info *info,
		  struct fid_ep **ep, void *context);
int sock_dgram_sep(struct fid_domain *domain, struct fi_info *info,
		 struct fid_sep **sep, void *context);

int sock_msg_ep(struct fid_domain *domain, struct fi_info *info,
		  struct fid_ep **ep, void *context);
int sock_msg_sep(struct fid_domain *domain, struct fi_info *info,
		 struct fid_sep **sep, void *context);
int sock_msg_passive_ep(struct fid_fabric *fabric, struct fi_info *info,
		 struct fid_pep **pep, void *context);


int sock_domain(struct fid_fabric *fabric, struct fi_info *info,
		struct fid_domain **dom, void *context);


int sock_av_open(struct fid_domain *domain, struct fi_av_attr *attr,
		struct fid_av **av, void *context);
fi_addr_t _sock_av_lookup(struct sock_av *av, struct sockaddr *addr);
struct sock_conn *sock_av_lookup_addr(struct sock_av *av, fi_addr_t addr);


int sock_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
		 struct fid_cq **cq, void *context);
int _sock_cq_report_error(struct sock_cq *sock_cq, struct fi_cq_err_entry *error);


int sock_eq_open(struct fid_fabric *fabric, struct fi_eq_attr *attr,
		struct fid_eq **eq, void *context);
ssize_t sock_eq_report_event(struct sock_eq *sock_eq, uint32_t event, 
			     const void *buf, size_t len, uint64_t flags);
ssize_t sock_eq_report_error(struct sock_eq *sock_eq, fid_t fid, void *context,
			     int err, int prov_errno, void *err_data);


int sock_cntr_open(struct fid_domain *domain, struct fi_cntr_attr *attr,
		struct fid_cntr **cntr, void *context);


int sock_rdm_ep(struct fid_domain *domain, struct fi_info *info,
		struct fid_ep **ep, void *context);
int sock_dgram_ep(struct fid_domain *domain, struct fi_info *info,
		  struct fid_ep **ep, void *context);
int sock_passive_ep(struct fid_fabric *fabric, struct fi_info *info,
		   struct fid_pep **pep, void *context);


int sock_ep_connect(struct fid_ep *ep, const void *addr,
		    const void *param, size_t paramlen);


struct sock_rx_ctx *sock_rx_ctx_alloc(struct fi_rx_attr *attr,
				      void *context);
void sock_rx_ctx_add_ep(struct sock_rx_ctx *rx_ctx, struct sock_ep *ep);
void sock_rx_ctx_free(struct sock_rx_ctx *rx_ctx);


struct sock_tx_ctx *sock_tx_ctx_alloc(struct fi_tx_attr *attr,
				      void *context);
void sock_tx_ctx_add_ep(struct sock_tx_ctx *tx_ctx, struct sock_ep *ep);
void sock_tx_ctx_free(struct sock_tx_ctx *tx_ctx);
void sock_tx_ctx_start(struct sock_tx_ctx *tx_ctx);
void sock_tx_ctx_write(struct sock_tx_ctx *tx_ctx, const void *buf, size_t len);
void sock_tx_ctx_commit(struct sock_tx_ctx *tx_ctx);
void sock_tx_ctx_abort(struct sock_tx_ctx *tx_ctx);
int sock_tx_ctx_read(struct sock_tx_ctx *tx_ctx, void *buf, size_t len);


int sock_poll_open(struct fid_domain *domain, struct fi_poll_attr *attr,
		struct fid_poll **pollset);
int sock_wait_open(struct fid_domain *domain, struct fi_wait_attr *attr,
		struct fid_wait **waitset);

struct sock_pe *sock_pe_init(struct sock_domain *domain);
int sock_pe_add_tx_ctx(struct sock_pe *pe, struct sock_tx_ctx *ctx);
int sock_pe_add_rx_ctx(struct sock_pe *pe, struct sock_rx_ctx *ctx);
int sock_pe_progress_rx_ctx(struct sock_pe *pe, struct sock_rx_ctx *rx_ctx);
int sock_pe_progress_tx_ctx(struct sock_pe *pe, struct sock_tx_ctx *tx_ctx);
void sock_pe_finalize(struct sock_pe *pe);


struct sock_rx_entry *sock_rx_new_entry(struct sock_rx_ctx *rx_ctx);
struct sock_rx_entry *sock_rx_new_buffered_entry(struct sock_rx_ctx *rx_ctx,
						 size_t len);
struct sock_rx_entry *sock_rx_check_buffered_list(struct sock_rx_ctx *rx_ctx,
						   const struct fi_msg *msg, uint64_t flags);
struct sock_rx_entry *sock_rx_check_buffered_tlist(struct sock_rx_ctx *rx_ctx,
						    const struct fi_msg_tagged *msg, 
						    uint64_t flags);
struct sock_rx_entry *sock_rx_get_entry(struct sock_rx_ctx *rx_ctx, 
					uint64_t addr, uint64_t tag);
size_t sock_rx_avail_len(struct sock_rx_entry *rx_entry);
void sock_rx_release_entry(struct sock_rx_entry *rx_entry);


void free_fi_info(struct fi_info *info);


#endif
