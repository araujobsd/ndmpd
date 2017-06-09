/*
 * Copyright 2009 Sun Microsystems, Inc.  
 * All rights reserved.
 *
 * Use is subject to license terms.
 */

/*
 * BSD 3 Clause License
 *
 * Copyright (c) 2007, The Storage Networking Industry Association.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in
 *        the documentation and/or other materials provided with the
 *        distribution.
 *
 *      - Neither the name of The Storage Networking Industry Association (SNIA)
 *        nor the names of its contributors may be used to endorse or promote
 *        products derived from this software without specific prior written
 *        permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <handler.h>

#include <ndmp.h>
#include <ndmpd.h>
#include <ndmpd_util.h>
#include <ndmpd_func.h>
#include <ndmpd_session.h>
#include <ndmpd_fhistory.h>
#include <ndmpd_tar_v3.h>

/*
 * ************************************************************************
 * NDMP V3 HANDLERS
 * ************************************************************************
 */

/*
 * ndmpd_data_get_env_v3
 *
 * Request handler. Returns the environment variable array sent
 * with the backup request. This request may only be sent with
 * a backup operation is in progress.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
/*ARGSUSED*/
void
ndmpd_data_get_env_v3(ndmp_connection_t *connection, void *body)
{
	ndmp_data_get_env_reply reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);

	ndmpd_log(LOG_DEBUG, "ndmpd_data_get_env_v3");
	(void) memset((void*)&reply, 0, sizeof (reply));
	if (session->ns_data.dd_operation != NDMP_DATA_OP_BACKUP) {
		ndmpd_log(LOG_ERR, "Backup operation not active.");
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		reply.env.env_len = 0;
	} else {
		reply.error = NDMP_NO_ERR;
		reply.env.env_len = session->ns_data.dd_env_len;
		reply.env.env_val = session->ns_data.dd_env;
	}

	ndmp_send_reply(connection, &reply, "sending data_get_env reply");
}

/*
 * ndmpd_data_get_state_v3
 *
 * Request handler. Returns current data state.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
/*ARGSUSED*/
void
ndmpd_data_get_state_v3(ndmp_connection_t *connection, void *body)
{
	ndmp_data_get_state_reply_v3 reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);

	ndmpd_log(LOG_DEBUG, "ndmpd_data_get_state_v3");

	(void) memset((void*)&reply, 0, sizeof (reply));

	reply.error = NDMP_NO_ERR;
	reply.invalid = NDMP_DATA_STATE_EST_BYTES_REMAIN_INVALID
	    | NDMP_DATA_STATE_EST_TIME_REMAIN_INVALID;
	reply.operation = session->ns_data.dd_operation;
	reply.state = session->ns_data.dd_state;
	reply.halt_reason = session->ns_data.dd_halt_reason;

	if (reply.operation == NDMP_DATA_OP_BACKUP)
		reply.bytes_processed =
		    long_long_to_quad(
		    session->ns_data.dd_module.dm_stats.ms_bytes_processed);
	else
		reply.bytes_processed =
		    long_long_to_quad(ndmpd_data_get_info(session));

	reply.est_bytes_remain = long_long_to_quad(0LL);
	reply.est_time_remain = 0;
	if (session->ns_data.dd_state != NDMP_DATA_STATE_IDLE)
		ndmp_copy_addr_v3(&reply.data_connection_addr,
		    &session->ns_data.dd_data_addr);
	reply.read_offset = long_long_to_quad(session->ns_data.dd_read_offset);
	reply.read_length = long_long_to_quad(session->ns_data.dd_read_length);

	ndmp_send_reply(connection, &reply,
	    "sending ndmp_data_get_state_v3 reply");
}

/*
 * ndmpd_data_start_backup_v3
 *
 * Request handler. Starts a backup.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
void
ndmpd_data_start_backup_v3(ndmp_connection_t *connection, void *body)
{
	ndmp_data_start_backup_request_v3 *request;
	ndmp_data_start_backup_reply_v3 reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);
	ndmp_error err;

	request = (ndmp_data_start_backup_request_v3 *)body;

	(void) memset((void*)&reply, 0, sizeof (reply));

	err = start_backup_v3(session, request->bu_type, request->env.env_val,
	    request->env.env_len);
	/*
	 * start_backup_v3 sends the reply if the backup is
	 * successfully started.  Otherwise, send the reply
	 * containing the error here.
	 */
	if (err != NDMP_NO_ERR) {
		reply.error = err;
		ndmp_send_reply(connection, &reply,
		    "sending data_start_backup_v3 reply");
		ndmpd_data_cleanup(session);
	}
}

/*
 * ndmpd_data_start_recover_v3
 *
 * Request handler. Starts a restore.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
void
ndmpd_data_start_recover_v3(ndmp_connection_t *connection, void *body)
{
	ndmp_data_start_recover_request_v3 *request;
	ndmp_data_start_recover_reply_v3 reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);
	ndmp_error err;

	request = (ndmp_data_start_recover_request_v3 *)body;

	(void) memset((void*)&reply, 0, sizeof (reply));

	err = start_recover_v3(session, request->bu_type, request->env.env_val,
	    request->env.env_len, request->nlist.nlist_val,
	    request->nlist.nlist_len);

	/*
	 * start_recover_v3 sends the reply if the recover is
	 * successfully started.  Otherwise, send the reply
	 * containing the error here.
	 */
	if (err != NDMP_NO_ERR) {
		reply.error = err;
		ndmp_send_reply(connection, &reply,
		    "sending data_start_recover_v3 reply");
		ndmpd_data_error(session, NDMP_DATA_HALT_INTERNAL_ERROR);
		ndmpd_data_cleanup(session);
	}
}

/*
 * ndmpd_data_abort_v3
 *
 * Request handler. Aborts the current backup/restore. The operation
 * state is not changed to the halted state until after the operation
 * has actually been aborted and the notify_halt request has been sent.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
/*ARGSUSED*/
void
ndmpd_data_abort_v3(ndmp_connection_t *connection, void *body)
{
	ndmp_data_abort_reply reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);

	switch (session->ns_data.dd_state) {
	case NDMP_DATA_STATE_IDLE:
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		ndmpd_log(LOG_ERR, "Invalid state to process abort request.");
		break;
	case NDMP_DATA_STATE_ACTIVE:
		/*
		 * Don't go to HALTED state yet.  Need to wait for data
		 * operation to abort.  When this happens, ndmpd_done_v3
		 * will get called and will perform the halt processing.
		 */
		reply.error = NDMP_NO_ERR;
		session->ns_data.dd_abort = TRUE;
		if (session->ns_data.dd_module.dm_abort_func)
			(*session->ns_data.dd_module.dm_abort_func)(
			    session->ns_data.dd_module.dm_module_cookie);
		break;
	case NDMP_DATA_STATE_HALTED:
	case NDMP_DATA_STATE_LISTEN:
	case NDMP_DATA_STATE_CONNECTED:
		reply.error = NDMP_NO_ERR;
		session->ns_data.dd_abort = TRUE;
		ndmpd_data_error(session, NDMP_DATA_HALT_ABORTED);
		break;
	default:
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		ndmpd_log(LOG_DEBUG, "Unknown data V3 state %d",
		    session->ns_data.dd_state);
	}

	ndmp_send_reply(connection, &reply,
	    "sending data_abort_v3 reply");
}

/*
 * ndmpd_data_stop_v3
 *
 * Request handler. Stops the current data operation.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
/*ARGSUSED*/
void
ndmpd_data_stop_v3(ndmp_connection_t *connection, void *body)
{
	ndmp_data_stop_reply reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);

	if (session->ns_data.dd_state != NDMP_DATA_STATE_HALTED) {
		ndmpd_log(LOG_ERR, "Invalid state to process stop request.");
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		ndmp_send_reply(connection, &reply,
		    "sending data_stop_v3 reply");
		return;
	}
	ndmp_waitfor_op(session);
	ndmpd_data_cleanup(session);
	ndmpd_file_history_cleanup(session, FALSE);

	/* prepare for another data operation */
	(void) ndmpd_data_init(session);
	ndmpd_file_history_init(session);

	reply.error = NDMP_NO_ERR;
	ndmp_send_reply(connection, &reply,
	    "sending data_stop_v3 reply");
}

/*
 * ndmpd_data_listen_v3
 *
 * Request handler. Configures the server to listen for a connection
 * from a remote mover.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
void
ndmpd_data_listen_v3(ndmp_connection_t *connection, void *body)
{
	ndmp_data_listen_request_v3 *request;
	ndmp_data_listen_reply_v3 reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);
	u_long addr;
	u_short port;

	request = (ndmp_data_listen_request_v3 *)body;

	(void) memset((void*)&reply, 0, sizeof (reply));

	if (session->ns_data.dd_state != NDMP_DATA_STATE_IDLE) {
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		ndmpd_log(LOG_DEBUG,
		    "Invalid internal data state to process listen request.");
	} else if (session->ns_mover.md_state != NDMP_MOVER_STATE_IDLE) {
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		ndmpd_log(LOG_DEBUG,
		    "Invalid mover state to process listen request.");
	} else
		reply.error = NDMP_NO_ERR;

	if (reply.error != NDMP_NO_ERR) {
		ndmp_send_reply(connection, &reply,
		    "ndmp_data_listen_request_v3 reply");
		return;
	}

	switch (request->addr_type) {
	case NDMP_ADDR_LOCAL:
		reply.data_connection_addr.addr_type = request->addr_type;
		session->ns_data.dd_data_addr.addr_type = NDMP_ADDR_LOCAL;
		break;
	case NDMP_ADDR_TCP:
		if (create_listen_socket_v3(session, &addr, &port) < 0) {
			reply.error = NDMP_IO_ERR;
			break;
		}
		reply.error = NDMP_NO_ERR;
		reply.data_connection_addr.addr_type = request->addr_type;
		reply.data_connection_addr.tcp_ip_v3 = htonl(addr);
		reply.data_connection_addr.tcp_port_v3 = htons(port);
		session->ns_data.dd_data_addr.addr_type = NDMP_ADDR_TCP;
		session->ns_data.dd_data_addr.tcp_ip_v3 = addr;
		session->ns_data.dd_data_addr.tcp_port_v3 = ntohs(port);
		break;
	default:
		ndmpd_log(LOG_DEBUG, "Invalid address type: %d",
		    request->addr_type);
		reply.error = NDMP_ILLEGAL_ARGS_ERR;
		break;
	}

	if (reply.error == NDMP_NO_ERR)
		session->ns_data.dd_state = NDMP_DATA_STATE_LISTEN;

	ndmp_send_reply(connection, &reply,
	    "ndmp_data_listen_request_v3 reply");
}

/*
 * ndmpd_data_connect_v3
 *
 * Request handler. Connects the data server to either a local
 * or remote mover.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
void
ndmpd_data_connect_v3(ndmp_connection_t *connection, void *body)
{
	ndmp_data_connect_request_v3 *request;
	ndmp_data_connect_reply_v3 reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);

	request = (ndmp_data_connect_request_v3 *)body;

	(void) memset((void*)&reply, 0, sizeof (reply));

	if (!ndmp_valid_v3addr_type(request->addr.addr_type)) {
		reply.error = NDMP_ILLEGAL_ARGS_ERR;
		ndmpd_log(LOG_DEBUG, "Invalid address type %d",
		    request->addr.addr_type);
	} else if (session->ns_data.dd_state != NDMP_DATA_STATE_IDLE) {
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		ndmpd_log(LOG_ERR, "Invalid state to process connect request.");
	} else {
		reply.error = NDMP_NO_ERR;
	}

	if (reply.error != NDMP_NO_ERR) {
		ndmp_send_reply(connection, &reply,
		    "sending ndmp_data_connect_v3 reply");
		return;
	}

	switch (request->addr.addr_type) {
	case NDMP_ADDR_LOCAL:
		/*
		 * Verify that the mover is listening for a
		 * local connection
		 */
		if (session->ns_mover.md_state != NDMP_MOVER_STATE_LISTEN ||
		    session->ns_mover.md_listen_sock != -1) {
			reply.error = NDMP_ILLEGAL_STATE_ERR;
			ndmpd_log(LOG_ERR,
			    "Mover is not in local listen state.");
		} else {
			session->ns_mover.md_state = NDMP_MOVER_STATE_ACTIVE;
		}
		break;
	case NDMP_ADDR_TCP:
		reply.error = data_connect_sock_v3(session,
		    request->addr.tcp_ip_v3, request->addr.tcp_port_v3);
		break;
	default:
		reply.error = NDMP_ILLEGAL_ARGS_ERR;
		ndmpd_log(LOG_DEBUG, "Invalid address type %d",
		    request->addr.addr_type);
	}

	if (reply.error == NDMP_NO_ERR)
		session->ns_data.dd_state = NDMP_DATA_STATE_CONNECTED;

	ndmp_send_reply(connection, &reply,
	    "sending ndmp_data_connect_v3 reply");
}

/*
 * ************************************************************************
 * NDMP V4 HANDLERS
 * ************************************************************************
 */

/*
 * ndmpd_data_get_env_v4
 *
 * Request handler. Returns the environment variable array sent
 * with the backup request. This request may only be sent with
 * a backup operation is in progress.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
/*ARGSUSED*/
void
ndmpd_data_get_env_v4(ndmp_connection_t *connection, void *body)
{
	ndmp_data_get_env_reply reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);

	(void) memset((void*)&reply, 0, sizeof (reply));

	if (session->ns_data.dd_state != NDMP_DATA_STATE_ACTIVE &&
	    session->ns_data.dd_state != NDMP_DATA_STATE_HALTED) {
		ndmpd_log(LOG_ERR, "Invalid state for the data server.");
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		reply.env.env_len = 0;
	} else if (session->ns_data.dd_operation != NDMP_DATA_OP_BACKUP) {
		ndmpd_log(LOG_ERR, "Backup operation not active.");
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		reply.env.env_len = 0;
	} else {
		reply.error = NDMP_NO_ERR;
		reply.env.env_len = session->ns_data.dd_env_len;
		reply.env.env_val = session->ns_data.dd_env;
	}

	int i=0;
	for(i=0;i<reply.env.env_len;i++){
		ndmpd_log(LOG_DEBUG, "%d reply.env.env_val[%s]=%s",i, reply.env.env_val[i].name, reply.env.env_val[i].value);
	}

	ndmp_send_reply(connection, &reply, "sending data_get_env reply");
}

/*
 * ndmpd_data_get_state_v4
 *
 * Request handler. Returns current data state.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
/*ARGSUSED*/
void
ndmpd_data_get_state_v4(ndmp_connection_t *connection, void *body)
{
	ndmp_data_get_state_reply_v4 reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);

	(void) memset((void*)&reply, 0, sizeof (reply));

	reply.error = NDMP_NO_ERR;
	reply.unsupported = NDMP_DATA_STATE_EST_BYTES_REMAIN_INVALID
	    | NDMP_DATA_STATE_EST_TIME_REMAIN_INVALID;
	reply.operation = session->ns_data.dd_operation;
	reply.state = session->ns_data.dd_state;
	reply.halt_reason = session->ns_data.dd_halt_reason;
	if (reply.operation == NDMP_DATA_OP_BACKUP) {
		reply.bytes_processed = long_long_to_quad(
		    session->ns_data.dd_module.dm_stats.ms_bytes_processed);
	} else
		reply.bytes_processed =
		    long_long_to_quad(ndmpd_data_get_info(session));

	reply.est_bytes_remain = long_long_to_quad(0LL);
	reply.est_time_remain = 0;

	if (session->ns_data.dd_state != NDMP_DATA_STATE_IDLE)
		ndmp_copy_addr_v4(&reply.data_connection_addr, &session->ns_data.dd_data_addr_v4);

	reply.read_offset = long_long_to_quad(session->ns_data.dd_read_offset);
	reply.read_length = long_long_to_quad(session->ns_data.dd_read_length);

	ndmpd_log(LOG_DEBUG,"reply.operation = %d ", reply.operation);
	ndmpd_log(LOG_DEBUG,"reply.state = %d ", reply.state);
	ndmpd_log(LOG_DEBUG,"reply.halt_reason = %d ", reply.halt_reason);
	ndmpd_log(LOG_DEBUG,"reply.bytes_processed high= %lu low= %lu ",
		reply.bytes_processed.high, reply.bytes_processed.low);
	ndmpd_log(LOG_DEBUG,"reply.est_bytes_remainhigh= %lu low= %lu ",
		reply.est_bytes_remain.high, reply.est_bytes_remain.low);
	ndmpd_log(LOG_DEBUG,"reply.est_time_remain = %lu ", reply.est_time_remain);
	ndmpd_log(LOG_DEBUG,"reply.read_offset high= %lu low= %lu ", reply.read_offset.high,
		reply.read_offset.low);
	ndmpd_log(LOG_DEBUG,"reply.read_length high= %lu low= %lu ", reply.read_length.high,
		reply.read_length.low);
	ndmp_send_reply(connection, &reply,
	    "sending ndmp_data_get_state_v4 reply");

	free(reply.data_connection_addr.tcp_addr_v4);
}

/*
 * ndmpd_data_connect_v4
 *
 * Request handler. Connects the data server to either a local
 * or remote mover.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
void
ndmpd_data_connect_v4(ndmp_connection_t *connection, void *body)
{
	ndmp_data_connect_request_v4 *request;
	ndmp_data_connect_reply_v4 reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);

	request = (ndmp_data_connect_request_v4 *)body;

	(void) memset((void*)&reply, 0, sizeof (reply));

	if (!ndmp_valid_v3addr_type(request->addr.addr_type)) {
		reply.error = NDMP_ILLEGAL_ARGS_ERR;
		ndmpd_log(LOG_DEBUG, "Invalid address type %d",
		    request->addr.addr_type);
	} else if (session->ns_data.dd_state != NDMP_DATA_STATE_IDLE) {
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		ndmpd_log(LOG_ERR, "Invalid state to process connect request.");
	} else
		reply.error = NDMP_NO_ERR;

	if (reply.error != NDMP_NO_ERR) {
		ndmp_send_reply(connection, &reply,
		    "sending ndmp_data_connect_v4 reply");
		return;
	}

	switch (request->addr.addr_type) {
	case NDMP_ADDR_LOCAL:
		/*
		 * Verify that the mover is listening for a
		 * local connection
		 */
		if (session->ns_mover.md_state != NDMP_MOVER_STATE_LISTEN ||
		    session->ns_mover.md_listen_sock != -1) {
			reply.error = NDMP_ILLEGAL_STATE_ERR;
			ndmpd_log(LOG_ERR,
			    "Mover is not in local listen state.");
		} else {
			session->ns_mover.md_state = NDMP_MOVER_STATE_ACTIVE;
		}
		break;
	case NDMP_ADDR_TCP:
		reply.error = data_connect_sock_v3(session,
		    request->addr.tcp_ip_v4(0), request->addr.tcp_port_v4(0));
		break;
	default:
		reply.error = NDMP_ILLEGAL_ARGS_ERR;
		ndmpd_log(LOG_DEBUG, "Invalid address type %d",
		    request->addr.addr_type);
	}

	if (reply.error == NDMP_NO_ERR)
		session->ns_data.dd_state = NDMP_DATA_STATE_CONNECTED;

	ndmp_send_reply(connection, &reply,
	    "sending ndmp_data_connect_v4 reply");
}

/*
 * ndmpd_data_listen_v4
 *
 * Request handler. Configures the server to listen for a connection
 * from a remote mover.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
void
ndmpd_data_listen_v4(ndmp_connection_t *connection, void *body)
{
	ndmp_data_listen_request_v4 *request;
	ndmp_data_listen_reply_v4 reply;
	ndmpd_session_t *session = ndmp_get_client_data(connection);
	u_long addr;
	u_short port;

	request = (ndmp_data_listen_request_v4 *)body;

	(void) memset((void*)&reply, 0, sizeof (reply));

	if (session->ns_data.dd_state != NDMP_DATA_STATE_IDLE) {
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		ndmpd_log(LOG_ERR,
		    "Invalid internal data state to process listen request.");
	} else if (session->ns_mover.md_state != NDMP_MOVER_STATE_IDLE) {
		reply.error = NDMP_ILLEGAL_STATE_ERR;
		ndmpd_log(LOG_ERR,
		    "Invalid mover state to process listen request.");
	} else
		reply.error = NDMP_NO_ERR;

	if (reply.error != NDMP_NO_ERR) {
		ndmp_send_reply(connection, &reply,
		    "ndmp_data_listen_request_v4 reply");
		return;
	}

	switch (request->addr_type) {
	case NDMP_ADDR_LOCAL:
		reply.connect_addr.addr_type = request->addr_type;
		session->ns_data.dd_data_addr.addr_type = NDMP_ADDR_LOCAL;
		break;
	case NDMP_ADDR_TCP:
		if (create_listen_socket_v3(session, &addr, &port) < 0) {
			ndmpd_log(LOG_DEBUG, "create_listen_socket_v3 fail");
			reply.error = NDMP_IO_ERR;
			break;
		}

		reply.error = NDMP_NO_ERR;
		reply.connect_addr.addr_type = request->addr_type;
		reply.connect_addr.tcp_addr_v4 = ndmp_malloc(sizeof (ndmp_tcp_addr_v4));

		reply.connect_addr.tcp_ip_v4(0) = htonl(addr);
		reply.connect_addr.tcp_port_v4(0) = htons(port);
		reply.connect_addr.tcp_len_v4 = 1;

		session->ns_data.dd_data_addr_v4.addr_type = NDMP_ADDR_TCP;
		session->ns_data.dd_data_addr_v4.tcp_addr_v4 = ndmp_malloc(sizeof (ndmp_tcp_addr_v4));

		session->ns_data.dd_data_addr_v4.tcp_ip_v4(0) = addr;
		session->ns_data.dd_data_addr_v4.tcp_port_v4(0) = ntohs(port);
		session->ns_data.dd_data_addr_v4.tcp_len_v4 = 1;

		/* Copy that to data_addr for compatibility */
		session->ns_data.dd_data_addr.addr_type = NDMP_ADDR_TCP;
		session->ns_data.dd_data_addr.tcp_ip_v3 = addr;
		session->ns_data.dd_data_addr.tcp_port_v3 = ntohs(port);
		break;
	default:
		ndmpd_log(LOG_DEBUG, "Invalid address type: %d",
		    request->addr_type);
		reply.error = NDMP_ILLEGAL_ARGS_ERR;
		break;
	}

	if (reply.error == NDMP_NO_ERR)
		session->ns_data.dd_state = NDMP_DATA_STATE_LISTEN;

	ndmp_send_reply(connection, &reply,
	    "ndmp_data_listen_request_v4 reply");

	free(reply.connect_addr.tcp_addr_v4);
}

/*
 * ndmpd_data_start_recover_filehist_v4
 *
 * Request handler. Recovers the file history (not supported yet)
 * This command has an optional support in V4.
 *
 * Parameters:
 *   connection (input) - connection handle.
 *   body       (input) - request message body.
 *
 * Returns:
 *   void
 */
/*ARGSUSED*/
void
ndmpd_data_start_recover_filehist_v4(ndmp_connection_t *connection, void *body)
{
	ndmp_data_start_recover_filehist_reply_v4 reply;

	ndmpd_log(LOG_DEBUG, "Request not supported");
	reply.error = NDMP_NOT_SUPPORTED_ERR;

	ndmp_send_reply(connection, &reply,
	    "sending ndmp_data_start_recover_filehist_reply_v4 reply");
}

/*
 * ************************************************************************
 * LOCALS
 * ************************************************************************
 */

/*
 * ndmpd_data_error_send
 *
 * This function sends the notify message to the client.
 *
 * Parameters:
 *   session (input) - session pointer.
 *   reason  (input) - halt reason.
 *
 * Returns:
 *   Error code
 */
/*ARGSUSED*/
int
ndmpd_data_error_send(ndmpd_session_t *session, ndmp_data_halt_reason reason)
{
	ndmp_notify_data_halted_request req;

	req.reason = session->ns_data.dd_halt_reason;
	req.text_reason = "";

	return (ndmp_send_request(session->ns_connection,
	    NDMP_NOTIFY_DATA_HALTED, NDMP_NO_ERR, &req, 0));
}

/*
 * ndmpd_data_error_send_v4
 *
 * This function sends the notify message to the client.
 *
 * Parameters:
 *   session (input) - session pointer.
 *   reason  (input) - halt reason.
 *
 * Returns:
 *   Error code
 */
/*ARGSUSED*/
int
ndmpd_data_error_send_v4(ndmpd_session_t *session, ndmp_data_halt_reason reason)
{
	ndmp_notify_data_halted_request_v4 req;

	req.reason = session->ns_data.dd_halt_reason;

	return ndmp_send_request(session->ns_connection,
	    NDMP_NOTIFY_DATA_HALTED, NDMP_NO_ERR, &req, 0);
}

/*
 * ndmpd_data_error
 *
 * This function is called when a data error has been detected.
 * A notify message is sent to the client and the data server is
 * placed into the halted state.
 *
 * Parameters:
 *   session (input) - session pointer.
 *   reason  (input) - halt reason.
 *
 * Returns:
 *   void
 */
void
ndmpd_data_error(ndmpd_session_t *session, ndmp_data_halt_reason reason)
{
	if (session->ns_data.dd_state == NDMP_DATA_STATE_IDLE ||
	    session->ns_data.dd_state == NDMP_DATA_STATE_HALTED)
		return;

	if (session->ns_data.dd_operation == NDMP_DATA_OP_BACKUP) {
		/*
		 * Send/discard any buffered file history data.
		 */
		ndmpd_file_history_cleanup(session,
		    (reason == NDMP_DATA_HALT_SUCCESSFUL ? TRUE : FALSE));

		/*
		 * If mover local and successful backup, write any
		 * remaining buffered data to tape.
		 */
		if (session->ns_data.dd_data_addr.addr_type
		    == NDMP_ADDR_LOCAL && reason == NDMP_DATA_HALT_SUCCESSFUL)
			(void) ndmpd_local_write_v3(session, 0, 0);
	}

	session->ns_data.dd_state = NDMP_DATA_STATE_HALTED;
	session->ns_data.dd_halt_reason = reason;

	if (session->ns_protocol_version == NDMPV4) {
		if (ndmpd_data_error_send_v4(session, reason) < 0)
			ndmpd_log(LOG_DEBUG, "Error sending notify_data_halted request");
	} else {
		if (ndmpd_data_error_send(session, reason) < 0)
			ndmpd_log(LOG_DEBUG, "Error sending notify_data_halted request");
	}

	if (session->ns_data.dd_data_addr.addr_type == NDMP_ADDR_TCP) {
		if (session->ns_data.dd_sock != -1) {
			(void) ndmpd_remove_file_handler(session,
			    session->ns_data.dd_sock);
			/*
			 * ndmpcopy: we use the same socket for the mover,
			 * so expect to close when mover is done!
			 */
			if (session->ns_data.dd_sock != session->ns_mover.md_sock){
				(void) close(session->ns_data.dd_sock);
			}
			session->ns_data.dd_sock = -1;
		}
		if (session->ns_data.dd_listen_sock != -1) {
			(void) ndmpd_remove_file_handler(session,
				session->ns_data.dd_listen_sock);
			(void) close(session->ns_data.dd_listen_sock);
			session->ns_data.dd_listen_sock = -1;
		}
	} else
		ndmpd_mover_error(session, NDMP_MOVER_HALT_CONNECT_CLOSED);
}

/*
 * data_accept_connection_v3
 *
 * Accept a data connection from a remote mover.
 * Called by ndmpd_select when a connection is pending on
 * the data listen socket.
 *
 * Parameters:
 *   cookie  (input) - session pointer.
 *   fd      (input) - file descriptor.
 *   mode    (input) - select mode.
 *
 * Returns:
 *   void
 */
/*ARGSUSED*/
void
data_accept_connection_v3(void *cookie, int fd, u_long mode)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;
	socklen_t from_len;
	struct sockaddr_in from;
	int flag = 1;

	from_len = sizeof (from);
	session->ns_data.dd_sock = accept(fd, (struct sockaddr *)&from,
	    &from_len);

	ndmpd_log(LOG_DEBUG, "sin: port %d addr %s", ntohs(from.sin_port),
		inet_ntoa(IN_ADDR(from.sin_addr.s_addr)));

	ndmpd_remove_file_handler(session, fd);
	(void) close(session->ns_data.dd_listen_sock);
	session->ns_data.dd_listen_sock = -1;

	if (session->ns_data.dd_sock < 0) {
		ndmpd_log(LOG_DEBUG, "Accept error: %m");
		ndmpd_data_error(session, NDMP_DATA_HALT_CONNECT_ERROR);
		return;
	}

	/*
	 * Save the peer address.
	 */
	session->ns_data.dd_data_addr.tcp_ip_v3 = from.sin_addr.s_addr;
	session->ns_data.dd_data_addr.tcp_port_v3 = ntohs(from.sin_port);

	/*
	 * Set the parameter of the new socket.
	 */
	(void) setsockopt(session->ns_data.dd_sock, SOL_SOCKET, SO_KEEPALIVE,
	    &flag, sizeof (flag));
	ndmp_set_socket_nodelay(session->ns_data.dd_sock);
	if (ndmp_sbs > 0)
		ndmp_set_socket_snd_buf(session->ns_data.dd_sock,
		    ndmp_sbs * KB);
	if (ndmp_rbs > 0)
		ndmp_set_socket_rcv_buf(session->ns_data.dd_sock,
		    ndmp_rbs * KB);

	session->ns_data.dd_state = NDMP_DATA_STATE_CONNECTED;
}

/*
 * create_listen_socket_v3
 *
 * Creates the data sockets for listening for a remote mover/data
 * incoming connections.
 */
int
create_listen_socket_v3(ndmpd_session_t *session, u_long *addr, u_short *port)
{
	session->ns_data.dd_listen_sock = ndmp_create_socket(addr, port);

	if (session->ns_data.dd_listen_sock < 0)
		return (-1);

	/*
	 * Add a file handler for the listen socket.
	 * ndmpd_select will call data_accept_connection when a
	 * connection is ready to be accepted.
	 */
	if (ndmpd_add_file_handler(session, (void*)session,
	    session->ns_data.dd_listen_sock, NDMPD_SELECT_MODE_READ, HC_MOVER,
	    			data_accept_connection_v3) < 0)
	{
		(void) close(session->ns_data.dd_listen_sock);
		session->ns_data.dd_listen_sock = -1;
		return (-1);
	}
	ndmpd_log(LOG_DEBUG, "addr: %s:%d",
	    inet_ntoa(IN_ADDR(*addr)), ntohs(*port));

	return (0);
}

/*
 * data_connect_sock_v3
 *
 * Connect the data interface socket to the specified ip/port
 *
 * Parameters:
 *   session (input) - session pointer.
 *   addr    (input) - IP address
 *   port    (input) - port number
 *
 * Returns:
 *   NDMP_NO_ERR - backup successfully started.
 *   otherwise - error code of backup start error.
 */
ndmp_error
data_connect_sock_v3(ndmpd_session_t *session, u_long addr, u_short port)
{
	int sock;

	sock = ndmp_connect_sock_v3(addr, port);
	if (sock < 0)
		return (NDMP_CONNECT_ERR);

	session->ns_data.dd_sock = sock;
	session->ns_data.dd_data_addr.addr_type = NDMP_ADDR_TCP;
	session->ns_data.dd_data_addr.tcp_ip_v3 = ntohl(addr);
	session->ns_data.dd_data_addr.tcp_port_v3 = port;

	return (NDMP_NO_ERR);
}

/*
 * start_backup_v3
 *
 * Start the backup work
 *
 * Parameters:
 *   session (input) - session pointer.
 *   bu_type (input) - backup type.
 *   env_val (input) - environment variable array.
 *   env_len (input) - length of env_val.
 *
 * Returns:
 *   NDMP_NO_ERR - backup successfully started.
 *   otherwise - error code of backup start error.
 */
ndmp_error
start_backup_v3(ndmpd_session_t *session, char *bu_type, ndmp_pval *env_val,
    u_long env_len)
{
	int err;
	ndmp_lbr_params_t *nlp;
	ndmpd_module_params_t *params;
	ndmp_data_start_backup_reply_v3 reply;

	ndmpd_log(LOG_DEBUG, "start_backup_v3");

	(void) memset((void*)&reply, 0, sizeof (reply));

	if (session->ns_data.dd_state != NDMP_DATA_STATE_CONNECTED) {
		ndmpd_log(LOG_DEBUG,
		    "Can't start new backup in current state. %d",session->ns_data.dd_state);
		ndmpd_log(LOG_DEBUG,
		    "Connection to the mover is not established.");
		return (NDMP_ILLEGAL_STATE_ERR);
	}

	if (session->ns_data.dd_data_addr.addr_type == NDMP_ADDR_LOCAL) {
		return (NDMP_NO_DEVICE_ERR);
	}

	if (strcmp(bu_type, NDMP_DUMP_TYPE) != 0 &&
	    strcmp(bu_type, NDMP_TAR_TYPE) != 0) {
		ndmpd_log(LOG_ERR, "Invalid backup type: %s.", bu_type);
		ndmpd_log(LOG_ERR, "Supported backup types are tar and dump.");
		return (NDMP_ILLEGAL_ARGS_ERR);
	}

	err = ndmpd_save_env(session, env_val, env_len);
	if (err != NDMP_NO_ERR)
		return (err);

	nlp = ndmp_get_nlp(session);
	NDMP_FREE(nlp->nlp_params);
	params = nlp->nlp_params = ndmp_malloc(sizeof (ndmpd_module_params_t));
	if (!params)
		return (NDMP_NO_MEM_ERR);
	/* setup params for ndmpd_mover	*/
	params->mp_daemon_cookie = (void *)session;
	params->mp_module_cookie = &session->ns_data.dd_module.dm_module_cookie;
	params->mp_protocol_version = session->ns_protocol_version;
	params->mp_operation = NDMP_DATA_OP_BACKUP;
	params->mp_get_env_func = ndmpd_api_get_env;
	params->mp_add_env_func = ndmpd_api_add_env;
	params->mp_set_env_func = ndmpd_api_set_env;
	params->mp_get_name_func = 0;
	params->mp_dispatch_func = ndmpd_api_dispatch;
	params->mp_done_func = ndmpd_api_done_v3;

	if (session->ns_protocol_version == NDMPV4)
		params->mp_log_func_v3 = ndmpd_api_log_v4;
	else
		params->mp_log_func_v3 = ndmpd_api_log_v3;

	params->mp_add_file_handler_func = ndmpd_api_add_file_handler;
	params->mp_remove_file_handler_func = ndmpd_api_remove_file_handler;
	params->mp_write_func = ndmpd_api_write_v3;

	params->mp_read_func = 0;

	params->mp_file_recovered_func = 0;

	params->mp_stats = &session->ns_data.dd_module.dm_stats;
	session->ns_data.dd_module.dm_module_cookie = 0;

	if (strcmp(bu_type, NDMP_DUMP_TYPE) == 0) {
		NLP_SET(nlp, NLPF_DUMP);
		params->mp_file_history_path_func = 0;
		params->mp_file_history_dir_func = ndmpd_api_file_history_dir_v3;
		params->mp_file_history_node_func = ndmpd_api_file_history_node_v3;

	} else if (strcmp(bu_type, NDMP_TAR_TYPE) == 0) {
		NLP_SET(nlp, NLPF_TAR);
		params->mp_file_history_path_func = ndmpd_api_file_history_file_v3;
		params->mp_file_history_dir_func = 0;
		params->mp_file_history_node_func = 0;
	}
	else {
		NLP_UNSET(nlp, NLPF_DUMP);
		NLP_UNSET(nlp, NLPF_TAR);
	}

	/* ndmpd_tar_backup_starter_v3 and ndmpd_tar_backup_abort_v3 are defined in ndmpd_tar_v3 */
	session->ns_data.dd_module.dm_start_func = ndmpd_tar_backup_starter_v3;
	session->ns_data.dd_module.dm_abort_func = ndmpd_tar_backup_abort_v3;

	session->ns_data.dd_module.dm_stats.ms_est_bytes_remaining = 0;
	session->ns_data.dd_module.dm_stats.ms_est_time_remaining  = 0;
	session->ns_data.dd_nlist_v3 = 0;
	session->ns_data.dd_nlist_len = 0;
	session->ns_data.dd_bytes_left_to_read = 0;
	session->ns_data.dd_position = 0;
	session->ns_data.dd_discard_length = 0;
	session->ns_data.dd_read_offset = 0;
	session->ns_data.dd_read_length = 0;

	session->ns_data.dd_state = NDMP_DATA_STATE_ACTIVE;
	session->ns_data.dd_operation = NDMP_DATA_OP_BACKUP;
	session->ns_data.dd_abort = FALSE;
	err=ndmp_backup_get_params_v3(session, params);
	if(err!=NDMP_NO_ERR)
	{
		ndmp_send_response(session->ns_connection, err, NULL);
		/* use MOD_DONE() to graceful close the operation (For Symantec BackExec) */
		MOD_DONE(params, err);
		NDMP_FREE(nlp->nlp_params);
		// return no error here. we do not need the caller
		// to send the error. (For Symantec BackExec)
		return (NDMP_NO_ERR);
	}
	reply.error = NDMP_NO_ERR;
	if (ndmp_send_response(session->ns_connection, NDMP_NO_ERR,
	    &reply) < 0) {
		ndmpd_log(LOG_DEBUG, "Sending data_start_backup_v3 Acknowledge reply");
		return (NDMP_NO_ERR);
	}
	pthread_attr_t tattr;
	pthread_t thread;
	(void) pthread_attr_init(&tattr);
	(void) pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	err = pthread_create(&thread, &tattr,
		(funct_t)session->ns_data.dd_module.dm_start_func,params);
	(void) pthread_attr_destroy(&tattr);

	if (err != 0) {
		ndmpd_log(LOG_ERR, "Can't start backup session.");
		return (NDMP_ILLEGAL_ARGS_ERR);
	}
	return (NDMP_NO_ERR);
}


/*
 * start_recover_v3
 *
 * Start the restore work
 *
 * Parameters:
 *   session (input) - session pointer.
 *   bu_type   (input) - backup type.
 *   env_val   (input) - environment variable array.
 *   env_len   (input) - length of env_val.
 *
 * Returns:
 *   NDMP_NO_ERR - recover successfully started.
 *   otherwise   - error code of recover start error.
 */
ndmp_error
start_recover_v3(ndmpd_session_t *session, char *bu_type, ndmp_pval *env_val,
    u_long env_len, ndmp_name_v3 *nlist_val, u_long nlist_len)
{
	ndmp_data_start_recover_reply_v3 reply;
	ndmpd_module_params_t *params;
	ndmp_lbr_params_t *nlp;
	int err;

	ndmpd_log(LOG_DEBUG, "++++++++start_recover_v3++++++++");

	(void) memset((void*)&reply, 0, sizeof (reply));

	if (session->ns_data.dd_state != NDMP_DATA_STATE_CONNECTED) {
		ndmpd_log(LOG_ERR, "Can't start new recover in current state.");
		return (NDMP_ILLEGAL_STATE_ERR);
	}

	if (strcmp(bu_type, NDMP_DUMP_TYPE) != 0 &&
	    strcmp(bu_type, NDMP_TAR_TYPE) != 0) {
		ndmpd_log(LOG_ERR, "Invalid backup type: %s.", bu_type);
		ndmpd_log(LOG_ERR, "Supported backup types are tar and dump.");
		return (NDMP_ILLEGAL_ARGS_ERR);
	}

	nlp = ndmp_get_nlp(session);
	NDMP_FREE(nlp->nlp_params);
	params = nlp->nlp_params = ndmp_malloc(sizeof (ndmpd_module_params_t));

	if (!params)
		return (NDMP_NO_MEM_ERR);

	reply.error = ndmpd_save_env(session, env_val, env_len);
	if (reply.error != NDMP_NO_ERR) {
		NDMP_FREE(nlp->nlp_params);
		return (NDMP_NO_MEM_ERR);
	}

	reply.error = ndmpd_save_nlist_v3(session, nlist_val, nlist_len);
	if (reply.error != NDMP_NO_ERR) {
		NDMP_FREE(nlp->nlp_params);
		return (NDMP_NO_MEM_ERR);
	}

	/*
	 * Setup restore parameters.
	 */
	params->mp_daemon_cookie = (void *)session;
	params->mp_module_cookie = &session->ns_data.dd_module.dm_module_cookie;
	params->mp_protocol_version = session->ns_protocol_version;
	params->mp_operation = NDMP_DATA_OP_RECOVER;
	params->mp_get_env_func = ndmpd_api_get_env;
	params->mp_add_env_func = ndmpd_api_add_env;
	params->mp_set_env_func = ndmpd_api_set_env;
	params->mp_get_name_func = ndmpd_api_get_name_v3;
	params->mp_dispatch_func = ndmpd_api_dispatch;
	params->mp_done_func = ndmpd_api_done_v3;

	if (session->ns_protocol_version == NDMPV4) {
		params->mp_log_func_v3 = ndmpd_api_log_v4;
		params->mp_file_recovered_func = ndmpd_api_file_recovered_v4;
	} else {
		params->mp_log_func_v3 = ndmpd_api_log_v3;
		params->mp_file_recovered_func = ndmpd_api_file_recovered_v3;
	}

	params->mp_add_file_handler_func = ndmpd_api_add_file_handler;
	params->mp_remove_file_handler_func = ndmpd_api_remove_file_handler;
	params->mp_write_func = 0;
	params->mp_file_history_path_func = 0;
	params->mp_file_history_dir_func = 0;
	params->mp_file_history_node_func = 0;
	params->mp_read_func = ndmpd_api_read_v3;
	params->mp_seek_func = ndmpd_api_seek_v3;
	params->mp_stats = &session->ns_data.dd_module.dm_stats;

	session->ns_data.dd_module.dm_module_cookie = 0;
	session->ns_data.dd_module.dm_start_func = ndmpd_tar_restore_starter_v3;
	session->ns_data.dd_module.dm_abort_func = ndmpd_tar_restore_abort_v3;
	session->ns_data.dd_module.dm_stats.ms_est_bytes_remaining = 0;
	session->ns_data.dd_module.dm_stats.ms_est_time_remaining = 0;
	session->ns_data.dd_bytes_left_to_read = 0;
	session->ns_data.dd_position = 0;
	session->ns_data.dd_discard_length = 0;
	session->ns_data.dd_read_offset = 0;
	session->ns_data.dd_read_length = 0;

	err = ndmp_restore_get_params_v3(session, params);
	if (err != NDMP_NO_ERR) {
		NDMP_FREE(nlp->nlp_params);
		return (err);
	}

	reply.error = NDMP_NO_ERR;
	if (ndmp_send_response(session->ns_connection, NDMP_NO_ERR,
	    &reply) < 0) {
		NDMP_FREE(nlp->nlp_params);
		ndmpd_free_nlist_v3(session);
		ndmpd_log(LOG_DEBUG,
		    "Error sending ndmp_data_start_recover_reply");
		ndmpd_data_error(session, NDMP_DATA_HALT_CONNECT_ERROR);
		return (NDMP_NO_ERR);
	}

	session->ns_data.dd_state = NDMP_DATA_STATE_ACTIVE;
	session->ns_data.dd_operation = NDMP_DATA_OP_RECOVER;
	session->ns_data.dd_abort = FALSE;

	pthread_attr_t tattr;
	pthread_t thread;

	(void) pthread_attr_init(&tattr);
	(void) pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	err = pthread_create(&thread, &tattr,
		(funct_t)session->ns_data.dd_module.dm_start_func,params);
	(void) pthread_attr_destroy(&tattr);

	if (err != 0) {
		ndmpd_log(LOG_ERR, "Can't start recover session.");
		return (NDMP_ILLEGAL_ARGS_ERR);
	}

	ndmpd_log(LOG_DEBUG, "--------start_recover_v3--------");

	return (NDMP_NO_ERR);
}

/*
 * nlp_release_job_stat
 *
 * Unreference the job statistics
 *
 * Parameters:
 *   session (input) - session pointer.
 *
 * Returns:
 *   void
 */
void
nlp_release_job_stat(ndmpd_session_t *session)
{
	ndmp_lbr_params_t *nlp;

	if ((nlp = ndmp_get_nlp(session)) == NULL) {
		ndmpd_log(LOG_DEBUG, "nlp == NULL");
		return;
	}
	if (nlp->nlp_jstat != NULL) {
		nlp->nlp_bytes_total =
		    (u_longlong_t)nlp->nlp_jstat->js_bytes_total;
		tlm_un_ref_job_stats(nlp->nlp_jstat->js_job_name);
		free(nlp->nlp_jstat);
	} else
		ndmpd_log(LOG_DEBUG, "JSTAT == NULL");
}

/* *** ndmpd global internal functions *********************************** */

/*
 * ndmpd_data_init
 *
 * Initializes data specific session variables.
 *
 * Parameters:
 *   session (input) - session pointer.
 *
 * Returns:
 *   void
 */
int
ndmpd_data_init(ndmpd_session_t *session)
{
	session->ns_data.dd_operation = NDMP_DATA_OP_NOACTION;
	session->ns_data.dd_state = NDMP_DATA_STATE_IDLE;
	session->ns_data.dd_halt_reason = NDMP_DATA_HALT_NA;
	session->ns_data.dd_abort = FALSE;
	session->ns_data.dd_env = 0;
	session->ns_data.dd_env_len = 0;
	session->ns_data.dd_nlist = 0;
	session->ns_data.dd_nlist_len = 0;
	session->ns_data.dd_mover.addr_type = NDMP_ADDR_LOCAL;

	session->ns_data.dd_sock = -1;

	session->ns_data.dd_read_offset = 0;
	session->ns_data.dd_read_length = 0;
	session->ns_data.dd_module.dm_stats.ms_est_bytes_remaining = 0;
	session->ns_data.dd_module.dm_stats.ms_est_time_remaining = 0;
	/*
	 * NDMP V3
	 */
	session->ns_data.dd_state = NDMP_DATA_STATE_IDLE;
	session->ns_data.dd_nlist_v3 = 0;
	session->ns_data.dd_data_addr.addr_type = NDMP_ADDR_LOCAL;
	session->ns_data.dd_listen_sock = -1;
	session->ns_data.dd_bytes_left_to_read = 0LL;
	session->ns_data.dd_position = 0LL;
	session->ns_data.dd_discard_length = 0LL;

	return (0);
}

/*
 * ndmpd_data_cleanup
 *
 * Releases resources allocated during a data operation.
 *
 * Parameters:
 *   session (input) - session pointer.
 *
 * Returns:
 *   void
 */
void
ndmpd_data_cleanup(ndmpd_session_t *session)
{
	if (session->ns_data.dd_listen_sock != -1) {
		(void) ndmpd_remove_file_handler(session, session->ns_data.dd_listen_sock);
		(void) close(session->ns_data.dd_listen_sock);
		session->ns_data.dd_listen_sock = -1;
	}

	if (session->ns_data.dd_sock != -1) {
		/*
		 * ndmpcopy: we use the same socket for the mover,
		 * so expect to close when mover is done!
		 */
		if (session->ns_data.dd_sock != session->ns_mover.md_sock)
			(void) close(session->ns_data.dd_sock);
		session->ns_data.dd_sock = -1;
	}

	ndmpd_free_tcp(session);
	ndmpd_free_env(session);
	ndmpd_free_nlist(session);
}

/*
 * ndmpd_data_get_info
 *
 * Return the total number of bytes processed
 *
 * Parameters:
 *   session   (input) - session pointer.
 *
 * Returns:
 *   the number of bytes processed
 */
u_longlong_t
ndmpd_data_get_info(ndmpd_session_t *session)
{
	ndmp_lbr_params_t *nlp;

	nlp = ndmp_get_nlp(session);

	if (nlp == NULL)
		return ((u_longlong_t)0);

	if (nlp->nlp_jstat == NULL)
		return (nlp->nlp_bytes_total);

	return ((u_longlong_t)nlp->nlp_jstat->js_bytes_total);
}
