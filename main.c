#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define SOF_BYTE 0xAA  // Start of Frame
#define MAX_PAYLOAD 8
#define RING_BUFFER_SIZE 32

// --- RING BUFFER ---
typedef struct {
    uint8_t buffer[RING_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} RingBuffer_t;

RingBuffer_t rx_buffer = { .head = 0, .tail = 0, .count = 0 };

// Lägg till ett byte i bufferten (Simulerar hårdvaru-interrupt / ISR)
bool ring_buffer_push(uint8_t byte) {
    if (rx_buffer.count >= RING_BUFFER_SIZE) {
        return false; // Bufferten är full (Overflow!)
    }
    rx_buffer.buffer[rx_buffer.head] = byte;
    rx_buffer.head = (rx_buffer.head + 1) % RING_BUFFER_SIZE;
    rx_buffer.count++;
    return true;
}

// Hämta ett byte från bufferten (Simulerar mjukvarans bearbetning)
bool ring_buffer_pop(uint8_t *byte) {
    if (rx_buffer.count == 0) {
        return false; // Bufferten är tom
    }
    *byte = rx_buffer.buffer[rx_buffer.tail];
    rx_buffer.tail = (rx_buffer.tail + 1) % RING_BUFFER_SIZE;
    rx_buffer.count--;
    return true;
}
// ---------------------------

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
                state = STATE_IDLE;
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
    printf("--- Startar Embedded Parser med Ring Buffer ---\n\n");

    // Giltigt paket: SOF (0xAA), CMD (0x02), LEN (0x01), PAYLOAD (0x2A), CRC (0x29)
    uint8_t raw_data[] = { 0xAA, 0x02, 0x01, 0x2A, 0x29 };

    // 1. Simulera hårdvaruankomst: Tryck in bytes i vår Ring Buffer
    printf("[HÅRDVARA] Tar emot %zu bytes till Ring Buffer...\n", sizeof(raw_data));
    for (int i = 0; i < sizeof(raw_data); i++) {
        if (!ring_buffer_push(raw_data[i])) {
            printf("[ERROR] Buffer Overflow!\n");
        }
    }

    // 2. Simulera mjukvaru-loopen: Töm bufferten och skicka till parsern
    printf("[MJUKVARA] Tömmer Ring Buffer och skickar till FSM Parser:\n");
    uint8_t byte_from_buffer;
    while (ring_buffer_pop(&byte_from_buffer)) {
        process_raw_byte_stream(byte_from_buffer);
    }

    return 0;
}