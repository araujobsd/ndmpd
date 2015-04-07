/*
 * Copyright 2009 Sun Microsystems, Inc.  
 * Copyright 2015 Marcelo Araujo <araujo@FreeBSD.org>.
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

#include <ndmpd_callbacks.h>


#include <handler.h>

#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <ndmpd_table.h>
#include <ndmpd.h>
#include <ndmpd_session.h>
#include <ndmpd_util.h>
#include <ndmpd_func.h>

#include <handler.h>







/*
 * Message Id counter.  This number is increased by MOD_LOGV3 macro.
 * MOD_LOGCONTV3 macro uses the number generated by the last MOD_LOGV3.
 *
 */
int ndmp_log_msg_id = 0;



/*
 * ************************************************************************
 * NDMP V3 CALLBACKS
 * ************************************************************************
 */

/*
 * ndmpd_api_done_v3
 *
 * Called when the data module has completed.
 * Sends a notify_halt request to the NDMP client.
 *
 * Parameters:
 *   session (input) - session pointer.
 *   err     (input) - UNIX error code.
 *
 * Returns:
 *   void
 */
void
ndmpd_api_done_v3(void *cookie, int err)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;
	ndmp_data_halt_reason reason;

	ndmpd_log(LOG_DEBUG, "ndmpd_api_done_v3--------------------");

	switch (err) {
	case 0:
		reason = NDMP_DATA_HALT_SUCCESSFUL;
		break;

	case EINTR:
		reason = NDMP_DATA_HALT_ABORTED;
		break;

	case EIO:
		reason = NDMP_DATA_HALT_CONNECT_ERROR;
		break;

	default:
		reason = NDMP_DATA_HALT_INTERNAL_ERROR;
	}

	ndmpd_data_error(session, reason);
}

/*
 * ndmpd_api_log_v3
 *
 * Sends a log request to the NDMP client.
 *
 * Parameters:
 *   cookie (input) - session pointer.
 *   format (input) - printf style format.
 *   ...    (input) - format arguments.
 *
 * Returns:
 *   0 - success.
 *  -1 - error.
 */
/*ARGSUSED*/
int
ndmpd_api_log_v3(void *cookie, ndmp_log_type type, u_long msg_id,
    char *format, ...)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;
	ndmp_log_message_request_v3 request;
	static char buf[1024];
	va_list ap;

	if (session == NULL)
		return (-1);

	va_start(ap, format);

	/*LINTED variable format specifier */
	(void) vsnprintf(buf, sizeof (buf), format, ap);
	va_end(ap);

	request.entry = buf;
	request.log_type = type;
	request.message_id = msg_id;

	if (ndmp_send_request(session->ns_connection, NDMP_LOG_MESSAGE,
	    NDMP_NO_ERR, (void *)&request, 0) < 0) {
		ndmpd_log(LOG_DEBUG, "Error sending log message request.");
		return (-1);
	}
	return (0);
}

/*
 * ndmpd_api_write_v3
 *
 * Callback function called by the backup/restore module.
 * Writes data to the mover.
 * If the mover is remote, the data is written to the data connection.
 * If the mover is local, the data is buffered and written to the
 * tape device after a full record has been buffered.
 *
 * Parameters:
 *   client_data (input) - session pointer.
 *   data       (input) - data to be written.
 *   length     (input) - data length.
 *
 * Returns:
 *   0 - data successfully written.
 *  -1 - error.
 */
int
ndmpd_api_write_v3(void *client_data, char *data, u_long length)
{
	ndmpd_session_t *session = (ndmpd_session_t *)client_data;
	if (session == NULL){
		ndmpd_log(LOG_DEBUG, "ndmpd_api_write_v3 session == NULL");
		return (-1);
	}

	/*
	 * Write the data to the tape if the mover is local, otherwise,
	 * write the data to the data connection.
	 *
	 * The same write function for of v2 can be used in V3
	 * for writing data to the data connection to the mover.
	 * So we don't need ndmpd_remote_write_v3().
	 */

	if (session->ns_data.dd_data_addr.addr_type == NDMP_ADDR_LOCAL)
		return (ndmpd_local_write_v3(session, data, length));
	else
		return (ndmpd_remote_write(session, data, length));
}

/*
 * ndmpd_api_read_v3
 *
 * Callback function called by the backup/recover module.
 * Reads data from the mover.
 * If the mover is remote, the data is read from the data connection.
 * If the mover is local, the data is read from the tape device.
 *
 * Parameters:
 *   client_data (input) - session pointer.
 *   data       (input) - data to be written.
 *   length     (input) - data length.
 *
 * Returns:
 *   0 - data successfully read.
 *  -1 - error.
 *   1 - session terminated or operation aborted.
 */
int
ndmpd_api_read_v3(void *client_data, char *data, u_long length)
{
	ndmpd_session_t *session = (ndmpd_session_t *)client_data;

	if (session == NULL)
		return (-1);

	/*
	 * Read the data from the data connection if the mover is remote.
	 */
	if (session->ns_data.dd_data_addr.addr_type == NDMP_ADDR_LOCAL)
		return (ndmpd_local_read_v3(session, data, length));
	else
		return (ndmpd_remote_read_v3(session, data, length));
}


/*
 * ndmpd_api_get_name_v3
 *
 * Return the name entry at the specified index from the
 * recover file name list.
 *
 * Parameters:
 *       cookie    (input) - NDMP session pointer.
 *       name_index (input) - index of entry to be returned.
 *
 * Returns:
 *   Pointer to name entry.
 *   0 if requested entry does not exist.
 */
void *
ndmpd_api_get_name_v3(void *cookie, u_long name_index)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;

	if (session == NULL)
		return (NULL);

	if (name_index >= session->ns_data.dd_nlist_len)
		return (NULL);

	return (&session->ns_data.dd_nlist_v3[name_index]);
}


/*
 * ndmpd_api_file_recovered_v3
 *
 * Notify the NDMP client that the specified file was recovered.
 *
 * Parameters:
 *   cookie (input) - session pointer.
 *   name   (input) - name of recovered file.
 *   ssid   (input) - selection set id.
 *   error  (input) - 0 if file successfully recovered.
 *		    otherwise, error code indicating why recovery failed.
 *
 * Returns:
 *   0 - success.
 *  -1 - error.
 */
int
ndmpd_api_file_recovered_v3(void *cookie, char *name, int error)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;
	ndmp_log_file_request_v3 request;

	if (session == NULL)
		return (-1);

	request.name  = name;

	switch (error) {
	case 0:
		request.error = NDMP_NO_ERR;
		break;
	case ENOENT:
		request.error = NDMP_FILE_NOT_FOUND_ERR;
		break;
	default:
		request.error = NDMP_PERMISSION_ERR;
	}

	if (ndmp_send_request(session->ns_connection, NDMP_LOG_FILE,
	    NDMP_NO_ERR, (void *)&request, 0) < 0) {
		ndmpd_log(LOG_DEBUG, "Error sending log file request");
		return (-1);
	}

	return (0);
}


/*
 * ndmpd_api_seek_v3
 *
 * Seek to the specified position in the data stream and start a
 * read for the specified amount of data.
 *
 * Parameters:
 *   cookie (input) - session pointer.
 *   offset (input) - stream position to seek to.
 *   length (input) - amount of data that will be read using ndmpd_api_read
 *
 * Returns:
 *   0 - seek successful.
 *   1 - seek needed DMA(client) intervention.
 *  -1 - error.
 */
int
ndmpd_api_seek_v3(void *cookie, u_longlong_t offset, u_longlong_t length)
{
	return 0;
}


/*
 * ************************************************************************
 * NDMP V4 CALLBACKS
 * ************************************************************************
 */

/*
 * ndmpd_api_log_v4
 *
 * Sends a log request to the NDMP client.
 * No message association is supported now, but can be added later on
 * in this function.
 *
 * Parameters:
 *   cookie (input) - session pointer.
 *   format (input) - printf style format.
 *   ...    (input) - format arguments.
 *
 * Returns:
 *   0 - success.
 *  -1 - error.
 */
/*ARGSUSED*/
int
ndmpd_api_log_v4(void *cookie, ndmp_log_type type, u_long msg_id,
    char *format, ...)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;
	ndmp_log_message_request_v4 request;
	static char buf[1024];
	va_list ap;

	if (session == NULL)
		return (-1);

	va_start(ap, format);

	/*LINTED variable format specifier */
	(void) vsnprintf(buf, sizeof (buf), format, ap);
	va_end(ap);

	request.entry = buf;
	request.log_type = type;
	request.message_id = msg_id;
	request.associated_message_valid = NDMP_NO_ASSOCIATED_MESSAGE;
	request.associated_message_sequence = 0;

	if (ndmp_send_request(session->ns_connection, NDMP_LOG_MESSAGE,
	    NDMP_NO_ERR, (void *)&request, 0) < 0) {
		ndmpd_log(LOG_DEBUG, "Error sending log message request.");
		return (-1);
	}
	return (0);
}


/*
 * ndmpd_api_file_recovered_v4
 *
 * Notify the NDMP client that the specified file was recovered.
 *
 * Parameters:
 *   cookie (input) - session pointer.
 *   name   (input) - name of recovered file.
 *   ssid   (input) - selection set id.
 *   error  (input) - 0 if file successfully recovered.
 *		    otherwise, error code indicating why recovery failed.
 *
 * Returns:
 *   void.
 */
int
ndmpd_api_file_recovered_v4(void *cookie, char *name, int error)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;
	ndmp_log_file_request_v4 request;

	if (session == NULL)
		return (-1);

	request.name  = name;

	switch (error) {
	case 0:
		request.recovery_status = NDMP_RECOVERY_SUCCESSFUL;
		break;
	case EPERM:
		request.recovery_status = NDMP_RECOVERY_FAILED_PERMISSION;
		break;
	case ENOENT:
		request.recovery_status = NDMP_RECOVERY_FAILED_NOT_FOUND;
		break;
	case ENOTDIR:
		request.recovery_status = NDMP_RECOVERY_FAILED_NO_DIRECTORY;
		break;
	case ENOMEM:
		request.recovery_status = NDMP_RECOVERY_FAILED_OUT_OF_MEMORY;
		break;
	case EIO:
		request.recovery_status = NDMP_RECOVERY_FAILED_IO_ERROR;
		break;
	case EEXIST:
		request.recovery_status = NDMP_RECOVERY_FAILED_FILE_PATH_EXISTS;
		break;
	default:
		request.recovery_status = NDMP_RECOVERY_FAILED_UNDEFINED_ERROR;
		break;
	}

	if (ndmp_send_request(session->ns_connection, NDMP_LOG_FILE,
	    NDMP_NO_ERR, (void *)&request, 0) < 0) {
		ndmpd_log(LOG_DEBUG, "Error sending log file request");
		return (-1);
	}

	return (0);
}


/*
 * ************************************************************************
 * LOCALS
 * ************************************************************************
 */

/*
 * ndmpd_api_find_env
 *
 * Return the pointer of the environment variable from the variable
 * array for the spcified environment variable.
 *
 * Parameters:
 *       cookie (input) - NDMP session pointer.
 *       name   (input) - name of variable.
 *
 * Returns:
 *   Pointer to variable.
 *   NULL if variable not found.
 *
 */
ndmp_pval *
ndmpd_api_find_env(void *cookie, char *name)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;
	u_long i;
	ndmp_pval *envp;

	if (session == NULL)
		return (NULL);

	envp = session->ns_data.dd_env;
	for (i = 0; envp && i < session->ns_data.dd_env_len; envp++, i++)
		if (!strcmp(name, envp->name))
			return (envp);

	return (NULL);
}


/*
 * ndmpd_api_get_env
 *
 * Return the value of an environment variable from the variable array.
 *
 * Parameters:
 *       cookie (input) - NDMP session pointer.
 *       name   (input) - name of variable.
 *
 * Returns:
 *   Pointer to variable value.
 *   0 if variable not found.
 *
 */
char *
ndmpd_api_get_env(void *cookie, char *name)
{
	ndmp_pval *envp;

	envp = ndmpd_api_find_env(cookie, name);
	if (envp)
		return (envp->value);

	return (NULL);
}


/*
 * ndmpd_api_add_env
 *
 * Adds an environment variable name/value pair to the environment
 * variable list.
 *
 * Parameters:
 *   session (input) - session pointer.
 *   name    (input) - variable name.
 *   val     (input) - value.
 *
 * Returns:
 *   0 - success.
 *  -1 - error.
 */
int
ndmpd_api_add_env(void *cookie, char *name, char *value)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;
	char *namebuf;
	char *valbuf;

	if (session == NULL)
		return (-1);

	session->ns_data.dd_env = realloc((void *)session->ns_data.dd_env,
	    sizeof (ndmp_pval) * (session->ns_data.dd_env_len + 1));

	if (session->ns_data.dd_env == NULL) {
		ndmpd_log(LOG_ERR, "Out of memory.");
		return (-1);
	}
	namebuf = strdup(name);
	if (namebuf == NULL)
		return (-1);

	valbuf = strdup(value);
	if (valbuf == NULL) {
		free(namebuf);
		return (-1);
	}

	(void) mutex_lock(&session->ns_lock);
	session->ns_data.dd_env[session->ns_data.dd_env_len].name = namebuf;
	session->ns_data.dd_env[session->ns_data.dd_env_len].value = valbuf;
	session->ns_data.dd_env_len++;
	(void) mutex_unlock(&session->ns_lock);

	return (0);
}


/*
 * ndmpd_api_set_env
 *
 * Sets an environment variable name/value pair in the environment
 * variable list.  If the variable exists, it gets the new value,
 * otherwise it's added as a new variable.
 *
 * Parameters:
 *   session (input) - session pointer.
 *   name    (input) - variable name.
 *   val     (input) - value.
 *
 * Returns:
 *   0 - success.
 *  -1 - error.
 */
int
ndmpd_api_set_env(void *cookie, char *name, char *value)
{
	char *valbuf;
	int rv;
	ndmp_pval *envp;

	envp = ndmpd_api_find_env(cookie, name);
	if (!envp) {
		rv = ndmpd_api_add_env(cookie, name, value);
	} else if (!(valbuf = strdup(value))) {
		rv = -1;
	} else {
		rv = 0;
		free(envp->value);
		envp->value = valbuf;
	}

	return (rv);
}


/*
 * ndmpd_api_get_name
 *
 * Return the name entry at the specified index from the
 * recover file name list.
 *
 * Parameters:
 *   cookie    (input) - NDMP session pointer.
 *   name_index (input) - index of entry to be returned.
 *
 * Returns:
 *   Pointer to name entry.
 *   0 if requested entry does not exist.
 */
void *
ndmpd_api_get_name(void *cookie, u_long name_index)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;

	if (session == NULL)
		return (NULL);

	if (name_index >= session->ns_data.dd_nlist_len)
		return (NULL);

	return (&session->ns_data.dd_nlist[name_index]);
}


/*
 * ndmpd_api_dispatch
 *
 * Process pending NDMP client requests and check registered files for
 * data availability.
 *
 * Parameters:
 *   cookie (input) - session pointer.
 *   block  (input) -
 * 		TRUE 	block until a request has been processed or
 *			until a file handler has been called.
 *		FALSE	don't block.
 *
 * Returns:
 *  -1 - abort request received or connection closed.
 *   0 - success.
 */
int
ndmpd_api_dispatch(void *cookie, bool_t block)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;
	int err;

	if (session == NULL)
		return (-1);

	for (; ; ) {
		err = ndmpd_select(session, block, HC_ALL);
		if (err < 0 || session->ns_data.dd_abort == TRUE ||
		    session->ns_eof)
			return (-1);

		if (err == 0)
			return (0);

		/*
		 * Something was processed.
		 * Set the block flag to false so that we will return as
		 * soon as everything available to be processed has been
		 * processed.
		 */
		block = FALSE;
	}
	return (-1);
}


/*
 * ndmpd_api_add_file_handler
 *
 * Adds a file handler to the file handler list.
 * The file handler list is used by ndmpd_api_dispatch.
 *
 * Parameters:
 *   daemon_cookie (input) - session pointer.
 *   cookie  (input) - opaque data to be passed to file hander when called.
 *   fd      (input) - file descriptor.
 *   mode    (input) - bitmask of the following:
 *	NDMP_SELECT_MODE_READ = watch file for ready for reading
 *	NDMP_SELECT_MODE_WRITE = watch file for ready for writing
 *	NDMP_SELECT_MODE_EXCEPTION = watch file for exception
 *   func    (input) - function to call when the file meets one of the
 *		     conditions specified by mode.
 *
 * Returns:
 *   0 - success.
 *  -1 - error.
 */
int
ndmpd_api_add_file_handler(void *daemon_cookie, void *cookie, int fd,
    u_long mode, ndmpd_file_handler_func_t *func)
{
	ndmpd_session_t *session = (ndmpd_session_t *)daemon_cookie;

	return (ndmpd_add_file_handler(session, cookie, fd, mode, HC_MODULE,
	    func));
}


/*
 * ndmpd_api_remove_file_handler
 *
 * Removes a file handler from the file handler list.
 *
 * Parameters:
 *   cookie  (input) - session pointer.
 *   fd      (input) - file descriptor.
 *
 * Returns:
 *   0 - success.
 *  -1 - error.
 */
int
ndmpd_api_remove_file_handler(void *cookie, int fd)
{
	ndmpd_session_t *session = (ndmpd_session_t *)cookie;

	return (ndmpd_remove_file_handler(session, fd));
}