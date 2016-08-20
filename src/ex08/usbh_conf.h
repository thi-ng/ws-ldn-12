#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f7xx.h"

#define USBH_MAX_NUM_ENDPOINTS 2
#define USBH_MAX_NUM_INTERFACES 2
#define USBH_MAX_NUM_CONFIGURATION 1
#define USBH_MAX_NUM_SUPPORTED_CLASS 1
#define USBH_KEEP_CFG_DESCRIPTOR 1
#define USBH_MAX_SIZE_CONFIGURATION 0x200
#define USBH_MAX_DATA_BUFFER 0x200
#define USBH_DEBUG_LEVEL 0
#define USBH_USE_OS 0
#define USE_USB_FS 0

/* CMSIS OS macros */
#if (USBH_USE_OS == 1)
#include "cmsis_os.h"
#define USBH_PROCESS_PRIO osPriorityNormal
#endif

/* Memory management macros */
#define USBH_malloc malloc
#define USBH_free free
#define USBH_memset memset
#define USBH_memcpy memcpy

/* DEBUG macros */

#if (USBH_DEBUG_LEVEL > 0)
#define USBH_UsrLog(...) \
  printf(__VA_ARGS__);   \
  printf("\n");
#else
#define USBH_UsrLog(...)
#endif

#if (USBH_DEBUG_LEVEL > 1)

#define USBH_ErrLog(...) \
  printf("ERROR: ");     \
  printf(__VA_ARGS__);   \
  printf("\n");
#else
#define USBH_ErrLog(...)
#endif

#if (USBH_DEBUG_LEVEL > 2)
#define USBH_DbgLog(...) \
  printf("DEBUG : ");    \
  printf(__VA_ARGS__);   \
  printf("\n");
#else
#define USBH_DbgLog(...)
#endif
