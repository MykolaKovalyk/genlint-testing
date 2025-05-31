#include "ftest_eth_buf.h"
#include "ringbuffer.h"
#include <stdint.h>

ringbuffer_t ftest_eth_inproc_rb = {0};

uint8_t ftest_eth_inproc_rb_buffer[FTEST_ETH_INPROC_BUFFER_SIZE] = {0}; // 64KB buffer