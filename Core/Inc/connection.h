#ifndef INC_CONNECTION_H_

#define INC_CONNECTION_H_

#include <stdbool.h>

#include "constants.h"
#include "lwesp/lwesp.h"
#include "lwesp/lwesp_conn.h"

#define REQUEST_TIMEOUT pdMS_TO_TICKS(10000)

typedef enum ConnectionStatus {
	CONNECTION_STATUS_UNINITIALIZED,
	CONNECTION_STATUS_INITIALIZED,
	CONNECTION_STATUS_ERROR,
	CONNECTION_STATUS_CONNECTED,
	CONNECTION_STATUS_DISCONNECTED
} ConnectionStatus;

typedef struct Connection {
	void (*onClose)(struct Connection *connection);
	lwesp_conn_p	 conn;
	lwesp_pbuf_p	 buf;
	TaskHandle_t	 task;
	uint32_t		 lastActivity;
	ConnectionStatus status;
	bool			 timeout;
	bool			 forceDisconnect;
} Connection;

void start_connection(Connection *connection, lwesp_conn_type_t connType, const char *host, size_t port);
void send_request(Connection *connection, const uint8_t *data, size_t len);
void wait_for_connection_closed(Connection *connection);

#endif /* INC_CONNECTION_H_ */
