/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*       Internal SystemView header — included only by SEGGER_SYSVIEW.c
*       SEGGER_SYSVIEW_C is defined before this include, which causes
*       SEGGER_SYSVIEW.h to emit definitions instead of extern decls.
**********************************************************************
*/
#ifndef SEGGER_SYSVIEW_INT_H
#define SEGGER_SYSVIEW_INT_H

#include "SEGGER_SYSVIEW.h"

/*********************************************************************
*
*       Command IDs — sent from SystemView host down to target.
*       All SYSVIEW_EVTID_* values are already in SEGGER_SYSVIEW.h.
*
**********************************************************************
*/
#define SEGGER_SYSVIEW_COMMAND_ID_START             1
#define SEGGER_SYSVIEW_COMMAND_ID_STOP              2
#define SEGGER_SYSVIEW_COMMAND_ID_GET_SYSTIME       3
#define SEGGER_SYSVIEW_COMMAND_ID_GET_TASKLIST      4
#define SEGGER_SYSVIEW_COMMAND_ID_GET_SYSDESC       5
#define SEGGER_SYSVIEW_COMMAND_ID_GET_NUMMODULES    6
#define SEGGER_SYSVIEW_COMMAND_ID_GET_MODULEDESC    7
#define SEGGER_SYSVIEW_COMMAND_ID_GET_MODULE        8
#define SEGGER_SYSVIEW_COMMAND_ID_HEARTBEAT         127

#endif /* SEGGER_SYSVIEW_INT_H */
