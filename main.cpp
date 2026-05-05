#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// All multi-byte values in .slp are big-endian
uint16_t read_u16(const uint8_t* buf) {
    return (uint16_t(buf[0]) << 8) | buf[1];
}

uint32_t read_u32(const uint8_t* buf) {
    return (uint32_t(buf[0]) << 24) | (uint32_t(buf[1]) << 16) |
           (uint32_t(buf[2]) << 8)  | buf[3];
}

float read_f32(const uint8_t* buf) {
    uint32_t raw = read_u32(buf);
    float f;
    std::memcpy(&f, &raw, 4);
    return f;
}

// Returns the byte offset where the raw event stream begins, and its length
bool parse_ubjson_header(const uint8_t* data, size_t file_size,
                         size_t& raw_offset, uint32_t& raw_length) {
    // Expected lead-in: { U \x03 r a w [ $ U # l <4-byte-length>
    const uint8_t magic[] = {'{', 'U', 0x03, 'r', 'a', 'w', '[', '$', 'U', '#', 'l'};
    if (file_size < 15 || std::memcmp(data, magic, sizeof(magic)) != 0) {
        std::cerr << "Not a valid .slp file\n";
        return false;
    }
    raw_length = read_u32(data + 11);
    raw_offset = 15;
    return true;
}

const char* stage_name(uint16_t id) {
    switch (id) {
        case 0x02: return "Fountain of Dreams";
        case 0x03: return "Pokémon Stadium";
        case 0x04: return "Princess Peach's Castle";
        case 0x05: return "Kongo Jungle";
        case 0x06: return "Brinstar";
        case 0x07: return "Corneria";
        case 0x08: return "Yoshi's Story";
        case 0x09: return "Onett";
        case 0x0A: return "Mute City";
        case 0x0B: return "Rainbow Cruise";
        case 0x0C: return "Jungle Japes";
        case 0x0D: return "Great Bay";
        case 0x0E: return "Hyrule Temple";
        case 0x0F: return "Brinstar Depths";
        case 0x10: return "Yoshi's Island";
        case 0x11: return "Green Greens";
        case 0x12: return "Fourside";
        case 0x13: return "Mushroom Kingdom I";
        case 0x14: return "Mushroom Kingdom II";
        case 0x16: return "Venom";
        case 0x17: return "Poké Floats";
        case 0x18: return "Big Blue";
        case 0x19: return "Icicle Mountain";
        case 0x1A: return "Icetop";
        case 0x1B: return "Flat Zone";
        case 0x1C: return "Dream Land N64";
        case 0x1D: return "Yoshi's Island N64";
        case 0x1E: return "Kongo Jungle N64";
        case 0x1F: return "Battlefield";
        case 0x20: return "Final Destination";
        default:   return "Unknown Stage";
    }
}

const char* character_name(uint8_t id) {
    switch (id) {
        case 0x00: return "Captain Falcon";
        case 0x01: return "Donkey Kong";
        case 0x02: return "Fox";
        case 0x03: return "Mr. Game & Watch";
        case 0x04: return "Kirby";
        case 0x05: return "Bowser";
        case 0x06: return "Link";
        case 0x07: return "Luigi";
        case 0x08: return "Mario";
        case 0x09: return "Marth";
        case 0x0A: return "Mewtwo";
        case 0x0B: return "Ness";
        case 0x0C: return "Peach";
        case 0x0D: return "Pikachu";
        case 0x0E: return "Ice Climbers";
        case 0x0F: return "Jigglypuff";
        case 0x10: return "Samus";
        case 0x11: return "Yoshi";
        case 0x12: return "Zelda";
        case 0x13: return "Sheik";
        case 0x14: return "Falco";
        case 0x15: return "Young Link";
        case 0x16: return "Dr. Mario";
        case 0x17: return "Roy";
        case 0x18: return "Pichu";
        case 0x19: return "Ganondorf";
        default:   return "Unknown";
    }
}

const char* player_type_name(uint8_t t) {
    switch (t) {
        case 0: return "Human";
        case 1: return "CPU";
        case 2: return "Demo";
        case 3: return "Empty";
        default: return "Unknown";
    }
}

void parse_game_start(const uint8_t* event) {
    // event[0] == 0x36, event[1..4] == version
    printf("Version:  %u.%u.%u\n", event[1], event[2], event[3]);

    const uint8_t* gib = event + 0x5; // Game Info Block

    uint16_t stage = read_u16(gib + 0x0E);
    printf("Stage:    %s (0x%02X)\n", stage_name(stage), stage);

    bool is_teams = gib[0x08] != 0;
    printf("Teams:    %s\n", is_teams ? "Yes" : "No");
    printf("PAL:      %s\n", event[0x1A1] ? "Yes" : "No");

    printf("\n");

    for (int i = 0; i < 4; i++) {
        uint8_t player_type = gib[0x61 + 36 * i];
        if (player_type == 3) continue; // empty slot

        uint8_t char_id    = gib[0x60 + 36 * i];
        uint8_t stocks     = gib[0x62 + 36 * i];
        uint8_t costume    = gib[0x63 + 36 * i];

        // Display name (Shift-JIS, up to 15 chars + null, 31 bytes per port)
        const uint8_t* name_ptr = event + 0x1A5 + 31 * i;
        char display_name[32] = {};
        std::memcpy(display_name, name_ptr, 31);

        printf("Port %d:   %s  |  %s  |  type=%s  |  stocks=%u  |  costume=%u\n",
               i + 1,
               display_name[0] ? display_name : "(no name)",
               character_name(char_id),
               player_type_name(player_type),
               stocks,
               costume);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.slp>\n";
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Could not open file: " << argv[1] << "\n";
        return 1;
    }

    size_t file_size = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> data(file_size);
    file.read(reinterpret_cast<char*>(data.data()), file_size);

    size_t raw_offset;
    uint32_t raw_length;
    if (!parse_ubjson_header(data.data(), file_size, raw_offset, raw_length))
        return 1;

    const uint8_t* raw = data.data() + raw_offset;

    // First event must be Event Payloads (0x35)
    if (raw[0] != 0x35) {
        std::cerr << "Expected Event Payloads (0x35), got 0x" << std::hex << int(raw[0]) << "\n";
        return 1;
    }

    // Build a size table: command byte -> payload size (not including command byte)
    uint16_t payload_sizes[256] = {};
    uint8_t payloads_block_size = raw[1];
    int num_entries = (payloads_block_size - 1) / 3;
    for (int i = 0; i < num_entries; i++) {
        uint8_t cmd = raw[2 + 3 * i];
        payload_sizes[cmd] = read_u16(raw + 3 + 3 * i);
    }

    // Scan for Game Start (0x36)
    // Event Payloads total size = 1 (cmd byte) + payloads_block_size
    size_t pos = 1 + payloads_block_size;
    while (pos < raw_length) {
        uint8_t cmd = raw[pos];
        if (cmd == 0x36) {
            parse_game_start(raw + pos);
            return 0;
        }
        pos += 1 + payload_sizes[cmd];
    }

    std::cerr << "Game Start event not found\n";
    return 1;
}
