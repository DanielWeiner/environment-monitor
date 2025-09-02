#include "connection.h"

#include "cmsis_os.h"
#include "constants.h"
#include "lwesp/lwesp_types.h"
#include "lwesp_util.h"

static lwespr_t on_connection_event(struct lwesp_evt *evt) {
	Connection *connection;
	uint32_t	now = osKernelGetTickCount();

	switch (evt->type) {
		case LWESP_EVT_CONN_ACTIVE:
			connection = (Connection *)lwesp_conn_get_arg(lwesp_evt_conn_active_get_conn(evt));
			connection->status = CONNECTION_STATUS_CONNECTED;
			break;
		case LWESP_EVT_CONN_CLOSE:
			connection = (Connection *)lwesp_conn_get_arg(lwesp_evt_conn_close_get_conn(evt));
			if (connection->status == CONNECTION_STATUS_CONNECTED) {
				connection->status = lwesp_evt_conn_close_get_result(evt) == lwespOK ? CONNECTION_STATUS_DISCONNECTED
																					 : CONNECTION_STATUS_ERROR;
			}
			break;
		case LWESP_EVT_CONN_ERROR:
			connection = (Connection *)lwesp_evt_conn_error_get_arg(evt);
			connection->status = CONNECTION_STATUS_ERROR;
			break;
		case LWESP_EVT_CONN_SEND:
			connection = (Connection *)lwesp_conn_get_arg(lwesp_evt_conn_send_get_conn(evt));
			if (connection->status != CONNECTION_STATUS_CONNECTED) return lwespOK;
			connection->status = CONNECTION_STATUS_CONNECTED;
			break;
		case LWESP_EVT_CONN_RECV: {
			connection = (Connection *)lwesp_conn_get_arg(lwesp_evt_conn_recv_get_conn(evt));
			if (connection->status != CONNECTION_STATUS_CONNECTED) return lwespOK;
			lwesp_pbuf_p buf = lwesp_evt_conn_recv_get_buff(evt);
			if (!connection->buf) {
				connection->buf = buf;
				lwesp_pbuf_ref(connection->buf);
			} else {
				lwesp_pbuf_chain(connection->buf, buf);
			}
			return lwespOK;
		}
		case LWESP_EVT_CONN_POLL:
			connection = (Connection *)lwesp_conn_get_arg(lwesp_evt_conn_poll_get_conn(evt));
			if (connection->status == CONNECTION_STATUS_CONNECTED) {
				if (now - connection->lastActivity > REQUEST_TIMEOUT) {
					if (!connection->forceDisconnect) {
						// if we're not already force disconnecting, try to close the connection
						connection->timeout = true;
						connection->status = CONNECTION_STATUS_ERROR;
						connection->forceDisconnect = true;
						connection->lastActivity = now;
						lwesp_conn_close(connection->conn, 0);
						return lwespOK;
					} else {
						// if it took too long to disconnect, notify the task anyways to avoid a deadlock
						xTaskNotifyGive(connection->task);
						return lwespOK;
					}
				}
			}
			return lwespOK;
		default:
			return lwespOK;
	}
	connection->lastActivity = now;
	xTaskNotifyGive(connection->task);
	return lwespOK;
}

void start_connection(Connection *connection, lwesp_conn_type_t connType, const char *host, size_t port) {
	connection->buf = NULL;
	connection->status = CONNECTION_STATUS_UNINITIALIZED;
	connection->timeout = false;
	connection->lastActivity = xTaskGetTickCount();
	connection->forceDisconnect = false;

	ulTaskNotifyValueClear(connection->task, UINT32_MAX);

	if (lwesp_conn_start(&connection->conn, connType, host, port, connection, on_connection_event, 0) != lwespOK) {
		return;
	}

	connection->status = CONNECTION_STATUS_INITIALIZED;

	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

void wait_for_connection_closed(Connection *connection) {
	if (connection->status != CONNECTION_STATUS_CONNECTED) return;
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	if (connection->status == CONNECTION_STATUS_DISCONNECTED) {
		if (connection->onClose) connection->onClose(connection);
	}
	lwesp_pbuf_free_s(&connection->buf);
}

void send_request(Connection *connection, const uint8_t *data, size_t len) {
	if (!data || !len || connection->status != CONNECTION_STATUS_CONNECTED) return;

	lwespr_t resp = lwespOK;
	size_t	 bytesWritten = 0;
	// Send the data in chunks whose size is determined by the internal tx buffer
	while (len > 0 &&
		   (resp = lwesp_conn_send(connection->conn, (const void *)data, len, &bytesWritten, 0)) == lwespOK) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (connection->status == CONNECTION_STATUS_ERROR || connection->status == CONNECTION_STATUS_DISCONNECTED) {
			return;
		}
		len -= bytesWritten;
		data += bytesWritten;
	}

	if (resp != lwespOK) {
		printf("Error sending data: %s" CRLF, response_str(resp));
		connection->status = CONNECTION_STATUS_ERROR;
		return;
	}
}