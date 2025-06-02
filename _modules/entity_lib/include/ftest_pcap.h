#pragma once
#include <stdint.h>
#include <stdio.h>

int ftest_pcap_write_global_header();

void ftest_pcap_write_packet(const uint8_t *packet, uint32_t len,
                       uint64_t timestamp_millis);