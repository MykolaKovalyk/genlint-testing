#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#define PCAP_MAGIC 0xA1B2C3D4
#define PCAP_VERSION_MAJOR 2
#define PCAP_VERSION_MINOR 4
#define LINKTYPE_ETHERNET 1

static FILE *pcap_file = NULL;

// PCAP Global Header Structure
struct pcap_global_header {
    uint32_t magic_number;
    uint16_t version_major;
    uint16_t version_minor;
    int32_t  thiszone;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t network;
};

// PCAP Packet Header Structure
struct pcap_packet_header {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
};

__attribute__((constructor)) 
static void create_pcap_file(void) {
    pcap_file = fopen("capture.pcap", "wb");
    if (!pcap_file) {
        perror("Failed to create PCAP file");
        exit(EXIT_FAILURE);
    }

    // Write global header
    struct pcap_global_header gh = {
        .magic_number = htonl(PCAP_MAGIC),
        .version_major = htons(PCAP_VERSION_MAJOR),
        .version_minor = htons(PCAP_VERSION_MINOR),
        .thiszone = 0,
        .sigfigs = 0,
        .snaplen = htonl(65535),
        .network = htonl(LINKTYPE_ETHERNET)
    };

    fwrite(&gh, sizeof(gh), 1, pcap_file);
}

__attribute__((destructor)) 
static void close_pcap_file(void) {
    if (pcap_file) {
        fclose(pcap_file);
    }
}

void ftest_pcap_write_packet(const uint8_t *packet, uint32_t len, uint64_t timestamp_millis) {
    if (!pcap_file) return;

    // Convert milliseconds to seconds and microseconds
    struct pcap_packet_header ph = {
        .ts_sec = htonl(timestamp_millis / 1000),
        .ts_usec = htonl((timestamp_millis % 1000) * 1000),
        .incl_len = htonl(len),
        .orig_len = htonl(len)
    };

    // Write packet header
    fwrite(&ph, sizeof(ph), 1, pcap_file);
    
    // Write packet data
    fwrite(packet, 1, len, pcap_file);
    fflush(pcap_file);
}