#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define SOF_BYTE 0xAA  // Start of Frame
#define MAX_PAYLOAD 8

// Definition av ett nätverkspaket
typedef struct {
    uint8_t start_byte;
    uint8_t command_id;
    uint8_t length;
    uint8_t payload[MAX_PAYLOAD];
    uint8_t checksum;
} Packet_t;

// Funktion för att beräkna kontrollsumma (XOR)
uint8_t calculate_checksum(uint8_t cmd, uint8_t len, uint8_t *payload) {
    uint8_t crc = cmd ^ len;
    for (int i = 0; i < len; i++) {
        crc ^= payload[i];
    }
    return crc;
}

// Simulator för att parsa inkommande rådata (Stream Parser)
void process_raw_byte_stream(uint8_t byte) {
    static enum { STATE_IDLE, STATE_GET_CMD, STATE_GET_LEN, STATE_GET_PAYLOAD, STATE_GET_CRC } state = STATE_IDLE;
    static Packet_t rx_packet;
    static uint8_t payload_index = 0;

    switch (state) {
        case STATE_IDLE:
            if (byte == SOF_BYTE) {
                rx_packet.start_byte = byte;
                payload_index = 0;
                state = STATE_GET_CMD;
            }
            break;

        case STATE_GET_CMD:
            rx_packet.command_id = byte;
            state = STATE_GET_LEN;
            break;

        case STATE_GET_LEN:
            if (byte <= MAX_PAYLOAD) {
                rx_packet.length = byte;
                if (rx_packet.length == 0) {
                    state = STATE_GET_CRC;
                } else {
                    state = STATE_GET_PAYLOAD;
                }
            } else {
                state = STATE_IDLE; // Felaktig längd, återställ
            }
            break;

        case STATE_GET_PAYLOAD:
            rx_packet.payload[payload_index++] = byte;
            if (payload_index >= rx_packet.length) {
                state = STATE_GET_CRC;
            }
            break;

        case STATE_GET_CRC:
            rx_packet.checksum = byte;
            uint8_t calculated_crc = calculate_checksum(rx_packet.command_id, rx_packet.length, rx_packet.payload);
            
            // Validera paket
            if (calculated_crc == rx_packet.checksum) {
                printf("[PARSER] Lyckat paket mottaget! CMD: 0x%02X, Längd: %d\n", rx_packet.command_id, rx_packet.length);
                if (rx_packet.command_id == 0x01) {
                    printf("  -> Systemstatus: OK\n");
                } else if (rx_packet.command_id == 0x02) {
                    printf("  -> Sensorvärde: %d\n", rx_packet.payload[0]);
                }
            } else {
                printf("[ERROR] Kontrollsumma matchar inte! Korrupt data detekterad.\n");
            }
            state = STATE_IDLE;
            break;
    }
}

int main() {
    printf("--- Startar Embedded Packet Parser Simulator ---\n\n");

    // 1. Simulera en giltig dataström (Status OK)
    // SOF (0xAA), CMD (0x01), LEN (0x00), CRC (0x01)
    uint8_t valid_stream1[] = { 0xAA, 0x01, 0x00, 0x01 };
    printf("Skickar giltig dataström 1 (Statuskommando)...\n");
    for (int i = 0; i < sizeof(valid_stream1); i++) {
        process_raw_byte_stream(valid_stream1[i]);
    }

    // 2. Simulera en giltig dataström med payload (Sensorvärde 42)
    // SOF (0xAA), CMD (0x02), LEN (0x01), PAYLOAD (0x2A = 42), CRC (0x02 ^ 0x01 ^ 0x2A = 0x29)
    uint8_t valid_stream2[] = { 0xAA, 0x02, 0x01, 0x2A, 0x29 };
    printf("\nSkickar giltig dataström 2 (Sensorvärde 42)...\n");
    for (int i = 0; i < sizeof(valid_stream2); i++) {
        process_raw_byte_stream(valid_stream2[i]);
    }

    // 3. Simulera korrupt data (Felaktig checksumma)
    uint8_t corrupt_stream[] = { 0xAA, 0x02, 0x01, 0x2A, 0x99 }; // 0x99 är fel CRC
    printf("\nSkickar korrupt dataström...\n");
    for (int i = 0; i < sizeof(corrupt_stream); i++) {
        process_raw_byte_stream(corrupt_stream[i]);
    }

    return 0;
}