#pragma once
/* xcpLite.h */

/* Copyright(c) Vector Informatik GmbH.All rights reserved.
   Licensed under the MIT license.See LICENSE file in the project root for details. */

#ifdef __XCPTL_CFG_H__
#error "Include dependency error!"
#endif
#ifdef __XCP_CFG_H__
#error "Include dependency error!"
#endif

#include "xcptl_cfg.h"  // Transport layer configuration

// Transport layer definitions and configuration
#include "xcpTl.h" 
#include "xcpEthTl.h"  // Ethernet transport layer specific functions

// Protocol layer definitions and configuration
#include "xcp_cfg.h"    // Protocol layer configuration
#include "xcp.h"        // XCP protocol defines



/****************************************************************************/
/* DAQ event channel information                                            */
/****************************************************************************/

#define XCP_UNDEFINED_EVENT_CHANNEL 0xFFFF

#ifdef XCP_ENABLE_DAQ_EVENT_LIST
  #ifndef XCP_MAX_EVENT_COUNT
    #define XCP_MAX_EVENT_COUNT 16
  #elif XCP_MAX_EVENT_COUNT > 16
    #warning "Memory consumption of event list is high, consider reducing XCP_MAX_EVENT_COUNT or XCP_MAX_EVENT_NAME"
  #endif
  #ifndef XCP_MAX_EVENT_NAME
    #define XCP_MAX_EVENT_NAME 8
  #endif

typedef struct {
    char shortName[XCP_MAX_EVENT_NAME+1]; // A2L XCP IF_DATA short event name, long name not supported
    uint32_t size; // ext event size
    uint8_t timeUnit; // timeCycle unit, 1ns=0, 10ns=1, 100ns=2, 1us=3, ..., 1ms=6, ...
    uint8_t timeCycle; // cycletime in units, 0 = sporadic or unknown
    uint16_t sampleCount; // packed event sample count
    uint16_t daqList; // associated DAQ list
    uint8_t priority; // priority 0 = queued, 1 = pushing, 2 = realtime

#ifdef XCP_ENABLE_MULTITHREAD_DAQ_EVENTS
    MUTEX mutex;
#endif
#ifdef XCP_ENABLE_TIMESTAMP_CHECK
    uint64_t time; // last event time stamp
#endif
} tXcpEvent;

#endif


/****************************************************************************/
/* Protocol layer interface                                                 */
/****************************************************************************/

/* Initialization for the XCP Protocol Layer */
extern void XcpInit();
extern void XcpStart();
extern void XcpReset();

/* XCP command processor */
extern uint8_t XcpCommand( const uint32_t* pCommand, uint8_t len );

/* Disconnect, stop DAQ, flush queue */
extern void XcpDisconnect();

/* Trigger a XCP data acquisition or stimulation event */
extern void XcpEvent(uint16_t event);
extern uint8_t XcpEventExt(uint16_t event, const uint8_t* base);
extern void XcpEventAt(uint16_t event, uint64_t clock);

/* Send an XCP event message */
extern void XcpSendEvent(uint8_t ev, uint8_t evc, const uint8_t* d, uint8_t l);

/* Print log message via XCP */
#ifdef XCP_ENABLE_SERV_TEXT
extern void XcpPrint( const char *str);
#endif

/* Check status */
extern BOOL XcpIsStarted();
extern BOOL XcpIsConnected();
extern uint16_t XcpGetSessionStatus();
extern BOOL XcpIsDaqRunning();
extern BOOL XcpIsDaqEventRunning(uint16_t event);
extern uint64_t XcpGetDaqStartTime();
extern uint32_t XcpGetDaqOverflowCount();

/* Time synchronisation */
#ifdef XCP_ENABLE_DAQ_CLOCK_MULTICAST
  #if XCP_PROTOCOL_LAYER_VERSION < 0x0103
    #error "Protocol layer version must be >=0x0103"
  #endif
extern uint16_t XcpGetClusterId();
#endif

/* Event list */
#ifdef XCP_ENABLE_DAQ_EVENT_LIST

// Clear event list
extern void XcpClearEventList();
// Add a measurement event to event list, return event number (0..MAX_EVENT-1)
extern uint16_t XcpCreateEvent(const char* name, uint32_t cycleTimeNs /* ns */, uint8_t priority /* 0-normal, >=1 realtime*/, uint16_t sampleCount, uint32_t size);
// Get event list
extern tXcpEvent* XcpGetEventList(uint16_t* eventCount);
// Lookup event
extern tXcpEvent* XcpGetEvent(uint16_t event);

#endif


/****************************************************************************/
/* Protocol layer external dependencies                                     */
/****************************************************************************/

// All callback functions supplied by the application
// Must be thread save

/* Callbacks on connect, disconnect, measurement prepare, start and stop */
extern BOOL ApplXcpConnect();
extern void ApplXcpDisconnect();
#if XCP_PROTOCOL_LAYER_VERSION >= 0x0104
extern BOOL ApplXcpPrepareDaq();
#endif
extern BOOL ApplXcpStartDaq();
extern void ApplXcpStopDaq();

/* Address conversions from A2L address to pointer and vice versa in absolute addressing mode */
#ifdef XCP_ENABLE_ABS_ADDRESSING
extern uint8_t* ApplXcpGetPointer(uint8_t xcpAddrExt, uint32_t xcpAddr); /* Create a pointer (uint8_t*) from xcpAddrExt and xcpAddr, returns NULL if no access */
extern uint32_t ApplXcpGetAddr(const uint8_t* p); // Calculate the xcpAddr address from a pointer
extern uint8_t *ApplXcpGetBaseAddr(); // Get the base address for DAQ data access */
#endif

/* Read and write memory */
#ifdef XCP_ENABLE_APP_ADDRESSING
extern uint8_t ApplXcpReadMemory(uint32_t src, uint8_t size, uint8_t* dst);
extern uint8_t ApplXcpWriteMemory(uint32_t dst, uint8_t size, const uint8_t* src);
#endif

/* User command */
#ifdef XCP_ENABLE_USER_COMMAND
extern uint8_t ApplXcpUserCommand(uint8_t cmd);
#endif

/*
 Note 1:
   For DAQ performance and memory optimization:
   XCPlite DAQ tables do not store address extensions and do not use ApplXcpGetPointer(), addr is stored as 32 Bit value and access is hardcoded by *(baseAddr+xcpAddr)
   All accesible DAQ data is within a 4GByte range starting at ApplXcpGetBaseAddr()
   Attempting to setup an ODT entry with address extension != XCP_ADDR_EXT_ABS or XCP_ADDR_EXT_DYN gives a CRC_ACCESS_DENIED error message

 Note 2:
   ApplXcpGetPointer may do address transformations according to active calibration page
*/


/* Switch calibration pages */
#ifdef XCP_ENABLE_CAL_PAGE
extern uint8_t ApplXcpSetCalPage(uint8_t segment, uint8_t page, uint8_t mode);
extern uint8_t ApplXcpGetCalPage(uint8_t segment, uint8_t mode);
extern uint8_t ApplXcpSetCalPage(uint8_t segment, uint8_t page, uint8_t mode);
#ifdef XCP_ENABLE_COPY_CAL_PAGE
extern uint8_t ApplXcpCopyCalPage(uint8_t srcSeg, uint8_t srcPage, uint8_t destSeg, uint8_t destPage);
#endif
#ifdef XCP_ENABLE_FREEZE_CAL_PAGE
extern uint8_t ApplXcpFreezeCalPage(uint8_t segment);
#endif
#endif
 
 
/* DAQ clock */
extern uint64_t ApplXcpGetClock64();
#define CLOCK_STATE_SYNCH_IN_PROGRESS                  (0)
#define CLOCK_STATE_SYNCH                              (1)
#define CLOCK_STATE_FREE_RUNNING                       (7)
#define CLOCK_STATE_GRANDMASTER_STATE_SYNCH             (1 << 3)
extern uint8_t ApplXcpGetClockState();

#ifdef XCP_ENABLE_PTP
#define CLOCK_STRATUM_LEVEL_UNKNOWN   255
#define CLOCK_STRATUM_LEVEL_ARB       16   // unsychronized
#define CLOCK_STRATUM_LEVEL_UTC       0    // Atomic reference clock
#define CLOCK_EPOCH_TAI 0 // Atomic monotonic time since 1.1.1970 (TAI)
#define CLOCK_EPOCH_UTC 1 // Universal Coordinated Time (with leap seconds) since 1.1.1970 (UTC)
#define CLOCK_EPOCH_ARB 2 // Arbitrary (epoch unknown)
extern BOOL ApplXcpGetClockInfoGrandmaster(uint8_t* uuid, uint8_t* epoch, uint8_t* stratum);
#endif

/* Get info for GET_ID command (pointer to and length of data) */
/* Supports IDT_ASCII, IDT_ASAM_NAME, IDT_ASAM_PATH, IDT_ASAM_URL, IDT_ASAM_EPK and IDT_ASAM_UPLOAD */
/* Returns 0 if not available */
extern uint32_t ApplXcpGetId(uint8_t id, uint8_t* buf, uint32_t bufLen);

/* Read a chunk (offset,size) of the A2L file for upload */
/* Return FALSE if out of bounds */
#ifdef XCP_ENABLE_IDT_A2L_UPLOAD // Enable A2L content upload to host (IDT_ASAM_UPLOAD)
extern BOOL ApplXcpReadA2L(uint8_t size, uint32_t offset, uint8_t* data);
#endif

