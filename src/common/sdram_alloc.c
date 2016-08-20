#include "sdram_alloc.h"
#include "ct-head/math.h"

static uint32_t sdram_heap_ptr = SDRAM_DEVICE_ADDR;
static uint32_t sdram_heap_end = SDRAM_DEVICE_ADDR + SDRAM_DEVICE_SIZE;
static uint32_t sdram_zero[32] = {0};

void* ct_sdram_malloc(size_t size) {
  void* ptr = NULL;
  if (sdram_heap_ptr + size < sdram_heap_end) {
    ptr = (void*)sdram_heap_ptr;
    sdram_heap_ptr += ct_ceil_multiple_pow2(size, 4);
  }
  return ptr;
}

void* ct_sdram_calloc(size_t num, size_t size) {
  size *= num;
  void* ptr = ct_sdram_malloc(size);
  if (ptr) {
    uint32_t addr = (uint32_t)ptr;
    while (size > 32) {
      BSP_SDRAM_WriteData(addr, sdram_zero, 32);
      addr += 32;
      size -= 32;
    }
    if (size > 0) {
      BSP_SDRAM_WriteData(addr, sdram_zero, size);
    }
  }
  return ptr;
}
