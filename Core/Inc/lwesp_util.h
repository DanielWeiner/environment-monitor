#ifndef INC_LWESP_UTIL_H_
#define INC_LWESP_UTIL_H_

#include "lwesp/lwesp.h"

static inline char *response_str(lwespr_t res) {
	switch (res) {
		case lwespOK:
			return "OK";
		case lwespOKIGNOREMORE:
			return "OKIGNOREMORE";
		case lwespERR:
			return "ERR";
		case lwespERRPAR:
			return "ERRPAR";
		case lwespERRMEM:
			return "ERRMEM";
		case lwespTIMEOUT:
			return "TIMEOUT";
		case lwespCONT:
			return "CONT";
		case lwespCLOSED:
			return "CLOSED";
		case lwespINPROG:
			return "INPROG";
		case lwespERRNOIP:
			return "ERRNOIP";
		case lwespERRNOFREECONN:
			return "ERRNOFREECONN";
		case lwespERRCONNTIMEOUT:
			return "ERRCONNTIMEOUT";
		case lwespERRPASS:
			return "ERRPASS";
		case lwespERRNOAP:
			return "ERRNOAP";
		case lwespERRCONNFAIL:
			return "ERRCONNFAIL";
		case lwespERRWIFINOTCONNECTED:
			return "ERRWIFINOTCONNECTED";
		case lwespERRNODEVICE:
			return "ERRNODEVICE";
		case lwespERRBLOCKING:
			return "ERRBLOCKING";
		case lwespERRCMDNOTSUPPORTED:
			return "ERRCMDNOTSUPPORTED";
		default:
			return "UNKNOWN";
	}
}

#endif /* INC_LWESP_UTIL_H_ */