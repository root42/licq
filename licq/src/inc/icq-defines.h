// ICQ definitions:
#ifndef ICQDEFINES_H
#define ICQDEFINES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Version constants
const unsigned short ICQ_VERSION                   = 0x0002;
const unsigned short ICQ_VERSION_TCP               = 0x0003;

// UDP commands
const unsigned short ICQ_CMDxRCV_ACK               = 0x000A;
const unsigned short ICQ_CMDxRCV_SETxOFFLINE       = 0x0028;
const unsigned short ICQ_CMDxRCV_NEWxUIN           = 0x0046;
const unsigned short ICQ_CMDxRCV_HELLO             = 0x005A;
const unsigned short ICQ_CMDxRCV_WRONGxPASSWD      = 0x0064;
const unsigned short ICQ_CMDxRCV_USERxONLINE       = 0x006E;
const unsigned short ICQ_CMDxRCV_USERxOFFLINE      = 0x0078;
const unsigned short ICQ_CMDxRCV_SEARCHxFOUND      = 0x008C;
const unsigned short ICQ_CMDxRCV_SEARCHxDONE       = 0x00A0;
const unsigned short ICQ_CMDxRCV_UPDATEDxBASIC     = 0x00B4;
const unsigned short ICQ_CMDxRCV_UPDATExBASICxFAIL = 0x00BE;
const unsigned short ICQ_CMDxRCV_UPDATEDxDETAIL    = 0x00C8;
const unsigned short ICQ_CMDxRCV_UPDATExDETAILxFAIL= 0x00D2;
const unsigned short ICQ_CMDxRCV_SYSxMSGxOFFLINE   = 0x00DC;
const unsigned short ICQ_CMDxRCV_SYSxMSGxDONE      = 0x00E6;
const unsigned short ICQ_CMDxRCV_ERROR             = 0x00F0;
const unsigned short ICQ_CMDxRCV_BUSY              = 0x00FA;
const unsigned short ICQ_CMDxRCV_SYSxMSGxONLINE    = 0x0104;
const unsigned short ICQ_CMDxRCV_USERxINFO         = 0x0118;
const unsigned short ICQ_CMDxRCV_USERxDETAILS      = 0x0122;
const unsigned short ICQ_CMDxRCV_USERxINVALIDxUIN  = 0x012C;
const unsigned short ICQ_CMDxRCV_USERxSTATUS       = 0x01A4;
//const unsigned short ICQ_CMDxRCV_UPDATEDxBASIC     = 0x01E0;
//const unsigned short ICQ_CMDxRCV_UPDATExBASICxFAIL = 0x01EA;
const unsigned short ICQ_CMDxRCV_USERxLISTxDONE    = 0x021C;

const unsigned short ICQ_CMDxSND_ACK               = 0x000A;
const unsigned short ICQ_CMDxSND_THRUxSERVER       = 0x010E;
const unsigned short ICQ_CMDxSND_PING              = 0x042E;
const unsigned short ICQ_CMDxSND_LOGON             = 0x03E8;
const unsigned short ICQ_CMDxSND_REGISTERxUSER     = 0x03FC;
const unsigned short ICQ_CMDxSND_USERxLIST         = 0x0406;
const unsigned short ICQ_CMDxSND_SEARCHxSTART      = 0x0424;
const unsigned short ICQ_CMDxSND_LOGOFF            = 0x0438;
const unsigned short ICQ_CMDxSND_SYSxMSGxDONExACK  = 0x0442;
const unsigned short ICQ_CMDxSND_SYSxMSGxREQ       = 0x044C;
const unsigned short ICQ_CMDxSND_AUTHORIZE         = 0x0456;
const unsigned short ICQ_CMDxSND_USERxGETINFO      = 0x0460;
const unsigned short ICQ_CMDxSND_USERxGETDETAILS   = 0x046A;
const unsigned short ICQ_CMDxSND_UPDATExBASIC      = 0x04A6;
const unsigned short ICQ_CMDxSND_UPDATExDETAIL     = 0x04B0;
const unsigned short ICQ_CMDxSND_SETxSTATUS        = 0x04D8;
//const unsigned short ICQ_CMDxSND_UPDATExBASIC      = 0x050A;
const unsigned short ICQ_CMDxSND_PING2             = 0x051E;
const unsigned short ICQ_CMDxSND_USERxADD          = 0x053C;
const unsigned short ICQ_CMDxSND_VISIBLExLIST      = 0x06AE;
const unsigned short ICQ_CMDxSND_INVISIBLExLIST    = 0x06A4;


// TCP commands
const unsigned short ICQ_CMDxTCP_START             = 0x07EE;
const unsigned short ICQ_CMDxTCP_CANCEL            = 0x07D0;
const unsigned short ICQ_CMDxTCP_ACK               = 0x07DA;
const unsigned short ICQ_CMDxTCP_READxAWAYxMSG     = 0x03E8;
const unsigned short ICQ_CMDxTCP_READxOCCUPIEDxMSG = 0x03E9;
const unsigned short ICQ_CMDxTCP_READxNAxMSG       = 0x03EA;
const unsigned short ICQ_CMDxTCP_READxDNDxMSG      = 0x03EB;
const unsigned char  ICQ_CMDxTCP_HANDSHAKE         =   0xFF;

// Sub Commands
const unsigned short ICQ_CMDxSUB_MSG               = 0x0001;
const unsigned short ICQ_CMDxSUB_CHAT              = 0x0002;
const unsigned short ICQ_CMDxSUB_FILE              = 0x0003;
const unsigned short ICQ_CMDxSUB_URL               = 0x0004;
const unsigned short ICQ_CMDxSUB_REQxAUTH          = 0x0006;
const unsigned short ICQ_CMDxSUB_ADDEDxTOxLIST     = 0x000C;
const unsigned short ICQ_CMDxSUB_WEBxPANEL         = 0x000D;
const unsigned short ICQ_CMDxSUB_EMAILxPAGER       = 0x000E;
const unsigned short ICQ_CMDxSUB_CONTACTxLIST      = 0x0013;
const unsigned short ICQ_CMDxSUB_USERxINFO         = 0x001A;  // not done
const unsigned short ICQ_CMDxSUB_FxMULTIREC        = 0x8000;


// Status constants
// Statuses must be checked in the following order:
//  DND, Occupied, NA, Away, Online
const unsigned short ICQ_STATUS_OFFLINE            = 0xFFFF;
const unsigned short ICQ_STATUS_ONLINE             = 0x0000;
const unsigned short ICQ_STATUS_AWAY               = 0x0001;
const unsigned short ICQ_STATUS_DND                = 0x0002;
const unsigned short ICQ_STATUS_NA                 = 0x0004;
const unsigned short ICQ_STATUS_OCCUPIED           = 0x0010;
const unsigned short ICQ_STATUS_FREEFORCHAT        = 0x0020;

// TCP status for ack packets
const unsigned short ICQ_TCPxACK_ONLINE            = 0x0000;
const unsigned short ICQ_TCPxACK_AWAY              = 0x0004;
const unsigned short ICQ_TCPxACK_OCCUPIED          = 0x0009;
const unsigned short ICQ_TCPxACK_DND               = 0x000A;
const unsigned short ICQ_TCPxACK_NA                = 0x000E;
const unsigned short ICQ_TCPxACK_ACCEPT            = 0x0000;
const unsigned short ICQ_TCPxACK_REFUSE            = 0x0001;
const unsigned short ICQ_TCPxACK_RETURN            = 0x0002;

// TCP message type (composed of (status | ..._Fx...)
const unsigned short ICQ_TCPxMSG_AUTOxREPLY        = 0x0000;
const unsigned short ICQ_TCPxMSG_NORMAL            = 0x0010;
const unsigned short ICQ_TCPxMSG_LIST              = 0x0020;  // not done
const unsigned short ICQ_TCPxMSG_URGENT            = 0x0040;  // not done
const unsigned short ICQ_TCPxMSG_FxONLINE          = 0x0000;
const unsigned short ICQ_TCPxMSG_FxINVISIBLE       = 0x0080;
const unsigned short ICQ_TCPxMSG_FxAWAY            = 0x0100;
const unsigned short ICQ_TCPxMSG_FxOCCUPIED        = 0x0200;
const unsigned short ICQ_TCPxMSG_FxNA              = 0x0800;
const unsigned short ICQ_TCPxMSG_FxDND             = 0x1000;

const unsigned long ICQ_STATUS_FxFLAGS             = 0xFFFF0000;
const unsigned long ICQ_STATUS_FxPRIVATE           = 0x00000100;
const unsigned long ICQ_STATUS_FxWEBxPRESENCE      = 0x00010000;
const unsigned long ICQ_STATUS_FxHIDExIP           = 0x00020000;
const unsigned long ICQ_STATUS_FxBIRTHDAY          = 0x00080000;  // not done

#endif
