#include "ringbuffer.h"

#define FTEST_ETH_INPROC_BUFFER_SIZE 65536 // 64KB

extern ringbuffer_t ftest_eth_inproc_rb;

extern uint8_t ftest_eth_inproc_rb_buffer[FTEST_ETH_INPROC_BUFFER_SIZE];