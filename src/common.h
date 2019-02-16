#include <stdint.h>

#define BAUD_RATE 115200
#define SPI_RATE 5000000

#define MINISUB_ADDR "1Node"
#define GROUNDSTATION_ADDR "2Node"

struct in_pack_t {
    int8_t shaft_voltage;
    uint8_t fore_plane_set;
    uint8_t aft_plane_set;
    uint8_t rudder_set;
    uint16_t ballast_set;
    uint16_t spool_set;
    uint8_t checksum;
};

struct out_pack_t {
    uint16_t ballast_pos;
    uint16_t spool_pos;
    uint8_t flooded;
    uint8_t shaft_voltage;
    uint8_t checksum;
};
