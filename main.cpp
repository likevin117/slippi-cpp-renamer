#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// All multi-byte values in .slp files are big-endian
uint16_t read_u16(const uint8_t* buf) {
    return (uint16_t(buf[0]) << 8) | buf[1];
}

uint32_t read_u32(const uint8_t* buf) {
    return (uint32_t(buf[0]) << 24) | (uint32_t(buf[1]) << 16) |
           (uint32_t(buf[2]) << 8)  | buf[3];
}

// ─── Data structures ──────────────────────────────────────────────────────────

struct PlayerInfo {
    uint8_t char_id    = 0xFF;
    uint8_t type       = 3;    // 0=human, 1=CPU, 2=demo, 3=empty
    uint8_t costume    = 0;
    uint8_t team_id    = 0;
    std::string nametag;
    std::string display_name;
    std::string netplay_name;  // from metadata UBJSON
};

struct SlpGame {
    uint8_t  version[4] = {};
    uint16_t stage_id   = 0;
    bool     is_teams   = false;
    PlayerInfo players[4];
    std::string start_at;      // ISO timestamp from metadata
};

// ─── Lookup tables ────────────────────────────────────────────────────────────

const char* stage_name(uint16_t id) {
    switch (id) {
        case 0x02: return "Fountain of Dreams";
        case 0x03: return "Pokemon Stadium";
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
        case 0x17: return "Poke Floats";
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
        case 0x03: return "Mr Game and Watch";
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
        case 0x16: return "Dr Mario";
        case 0x17: return "Roy";
        case 0x18: return "Pichu";
        case 0x19: return "Ganondorf";
        default:   return "Unknown";
    }
}

// Returns "Default" for the default costume, otherwise the color name.
// Used to match slippi-renamer's (color,netplay) label format.
const char* costume_color(uint8_t char_id, uint8_t costume) {
    switch (char_id) {
        case 0x00: // Captain Falcon
            switch (costume) { case 1: return "Black"; case 2: return "Red";
                               case 3: return "White"; case 4: return "Green"; case 5: return "Blue"; } break;
        case 0x01: // Donkey Kong
            switch (costume) { case 1: return "Black"; case 2: return "Red"; case 3: return "Blue"; case 4: return "Green"; } break;
        case 0x02: // Fox
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Green"; } break;
        case 0x04: // Kirby
            switch (costume) { case 1: return "Yellow"; case 2: return "Blue"; case 3: return "Red";
                               case 4: return "Green"; case 5: return "White"; } break;
        case 0x05: // Bowser
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Black"; } break;
        case 0x06: // Link
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Black"; case 4: return "White"; } break;
        case 0x07: // Luigi
            switch (costume) { case 1: return "White"; case 2: return "Blue"; case 3: return "Pink"; } break;
        case 0x08: // Mario
            switch (costume) { case 1: return "Yellow"; case 2: return "Black"; case 3: return "Blue"; case 4: return "Green"; } break;
        case 0x09: // Marth
            switch (costume) { case 1: return "Red"; case 2: return "Green"; case 3: return "Black"; case 4: return "White"; } break;
        case 0x0A: // Mewtwo
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Green"; } break;
        case 0x0B: // Ness
            switch (costume) { case 1: return "Yellow"; case 2: return "Blue"; case 3: return "Green"; } break;
        case 0x0C: // Peach
            switch (costume) { case 1: return "Daisy"; case 2: return "White"; case 3: return "Blue"; case 4: return "Green"; } break;
        case 0x0D: // Pikachu
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Green"; } break;
        case 0x0E: // Ice Climbers
            switch (costume) { case 1: return "Green"; case 2: return "Orange"; case 3: return "Red"; } break;
        case 0x0F: // Jigglypuff
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Green"; case 4: return "Crown"; } break;
        case 0x10: // Samus
            switch (costume) { case 1: return "Pink"; case 2: return "Black"; case 3: return "Green"; case 4: return "Purple"; } break;
        case 0x11: // Yoshi
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Yellow";
                               case 4: return "Pink"; case 5: return "Cyan"; } break;
        case 0x12: // Zelda
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "White"; case 4: return "Green"; } break;
        case 0x13: // Sheik
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Green"; case 4: return "White"; } break;
        case 0x14: // Falco
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Green"; } break;
        case 0x15: // Young Link
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "White"; case 4: return "Black"; } break;
        case 0x16: // Dr Mario
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Green"; case 4: return "Black"; } break;
        case 0x17: // Roy
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Green"; case 4: return "Yellow"; } break;
        case 0x18: // Pichu
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Green"; } break;
        case 0x19: // Ganondorf
            switch (costume) { case 1: return "Red"; case 2: return "Blue"; case 3: return "Green"; } break;
    }
    return "Default";
}

// ─── UBJSON metadata scanner ──────────────────────────────────────────────────

// Scans for: U <key_len> <key_bytes> S U <val_len> <val_bytes>
// Advances `pos` past the match so sequential calls find successive occurrences.
std::string scan_ubjson_string(const uint8_t* data, size_t size,
                               const std::string& key, size_t& pos) {
    uint8_t klen = static_cast<uint8_t>(key.size());
    for (; pos + key.size() + 5 < size; pos++) {
        if (data[pos] == 'U' && data[pos + 1] == klen &&
            memcmp(data + pos + 2, key.data(), key.size()) == 0) {
            size_t vpos = pos + 2 + key.size();
            if (vpos + 3 <= size && data[vpos] == 'S' && data[vpos + 1] == 'U') {
                uint8_t str_len = data[vpos + 2];
                vpos += 3;
                if (vpos + str_len <= size) {
                    pos = vpos + str_len;
                    return std::string(reinterpret_cast<const char*>(data + vpos), str_len);
                }
            }
        }
    }
    return "";
}

// ─── Core parser ─────────────────────────────────────────────────────────────

// Parses game start from a head buffer (UBJSON header + first ~4KB of raw events)
// and a separate meta buffer (the UBJSON metadata section at end of file).
// This avoids reading the large frame data block in the middle of the file.
bool parse_slp(const uint8_t* head, size_t head_size,
               const uint8_t* meta, size_t meta_size,
               SlpGame& game) {
    const uint8_t magic[] = {'{','U',0x03,'r','a','w','[','$','U','#','l'};
    if (head_size < 15 || memcmp(head, magic, sizeof(magic)) != 0)
        return false;

    const uint8_t* raw = head + 15;
    size_t raw_avail   = head_size - 15;   // how much raw we actually read

    if (raw_avail == 0 || raw[0] != 0x35) return false;

    // Build payload size table from Event Payloads block (0x35)
    uint16_t payload_sizes[256] = {};
    uint8_t  payloads_block_size = raw[1];
    int      num_entries         = (payloads_block_size - 1) / 3;
    for (int i = 0; i < num_entries; i++) {
        uint8_t cmd        = raw[2 + 3 * i];
        payload_sizes[cmd] = read_u16(raw + 3 + 3 * i);
    }

    // Scan for Game Start event (0x36) within the head buffer
    size_t pos = 1 + payloads_block_size;
    while (pos < raw_avail) {
        uint8_t cmd = raw[pos];
        if (cmd == 0x36) {
            const uint8_t* ev  = raw + pos;
            const uint8_t* gib = ev + 0x5;

            game.version[0] = ev[1];
            game.version[1] = ev[2];
            game.version[2] = ev[3];
            game.stage_id   = read_u16(gib + 0x0E);
            game.is_teams   = gib[0x08] != 0;

            for (int i = 0; i < 4; i++) {
                PlayerInfo& p = game.players[i];
                p.type    = gib[0x61 + 36 * i];
                p.char_id = gib[0x60 + 36 * i];
                p.costume = gib[0x63 + 36 * i];
                p.team_id = gib[0x69 + 36 * i];

                // Nametag: 8 UCS-2 characters at event[0x161 + 16*i]
                const uint8_t* tag = ev + 0x161 + 16 * i;
                for (int j = 0; j < 8; j++) {
                    uint8_t hi = tag[j * 2], lo = tag[j * 2 + 1];
                    if (hi == 0 && lo == 0) break;
                    if (hi == 0) p.nametag += static_cast<char>(lo);
                }

                // Display name: 31-byte Shift-JIS at event[0x1A5 + 31*i]
                const uint8_t* dn = ev + 0x1A5 + 31 * i;
                for (int j = 0; j < 30 && dn[j]; j++)
                    p.display_name += static_cast<char>(dn[j]);
            }
            break;
        }
        pos += 1 + payload_sizes[cmd];
    }

    if (game.stage_id == 0) return false;

    // Parse metadata for startAt and per-player netplay names
    if (meta && meta_size > 0) {
        size_t scan_pos = 0;
        game.start_at = scan_ubjson_string(meta, meta_size, "startAt", scan_pos);

        // Look up netplay name per port by finding each port's specific key.
        // Metadata doesn't write ports in port order, so sequential scan is wrong.
        // Port i is encoded as: U \x01 <'0'+i> { in the players UBJSON object.
        for (int i = 0; i < 4; i++) {
            if (game.players[i].type == 3) continue;
            uint8_t port_digit = static_cast<uint8_t>('0' + i);
            for (size_t j = 0; j + 4 < meta_size; j++) {
                if (meta[j] == 'U' && meta[j+1] == 0x01 &&
                    meta[j+2] == port_digit && meta[j+3] == '{') {
                    size_t mp = j + 4;
                    game.players[i].netplay_name =
                        scan_ubjson_string(meta, meta_size, "netplay", mp);
                    break;
                }
            }
        }
    }

    return true;
}

// ─── Filename generation ──────────────────────────────────────────────────────

// Extracts date from filename (Game_20240322T231153.slp -> "20240322T231153")
// or falls back to the ISO timestamp from metadata.
std::string date_prefix(const std::string& filename, const std::string& start_at) {
    auto underscore = filename.find('_');
    if (underscore != std::string::npos) {
        auto dot = filename.rfind('.');
        size_t len = (dot != std::string::npos) ? dot - underscore - 1 : std::string::npos;
        std::string candidate = filename.substr(underscore + 1, len);
        if (!candidate.empty()) return candidate;
    }
    if (!start_at.empty()) {
        std::string ts = start_at;
        ts.erase(std::remove(ts.begin(), ts.end(), '-'), ts.end());
        ts.erase(std::remove(ts.begin(), ts.end(), ':'), ts.end());
        if (!ts.empty() && ts.back() == 'Z') ts.pop_back();
        return ts;
    }
    return "";
}

// Builds "Fox (Red,kev11)" to match slippi-renamer's label format.
std::string player_label(const PlayerInfo& p) {
    std::string label = character_name(p.char_id);

    std::vector<std::string> ids;
    if (!p.nametag.empty()) {
        ids.push_back(p.nametag);
    } else {
        const char* color = costume_color(p.char_id, p.costume);
        if (std::string(color) != "Default")
            ids.push_back(color);
    }
    if (!p.netplay_name.empty() && p.netplay_name != "Player")
        ids.push_back(p.netplay_name);

    if (!ids.empty()) {
        label += " (";
        for (size_t i = 0; i < ids.size(); i++) {
            if (i > 0) label += ",";
            label += ids[i];
        }
        label += ")";
    }
    return label;
}

std::string generate_filename(const SlpGame& game, const std::string& original_name) {
    std::string date = date_prefix(original_name, game.start_at);
    if (date.empty()) return "";

    std::string matchup;
    if (game.is_teams) {
        std::map<uint8_t, std::vector<std::string>> teams;
        for (int i = 0; i < 4; i++) {
            if (game.players[i].type == 3) continue;
            teams[game.players[i].team_id].push_back(player_label(game.players[i]));
        }
        bool first_team = true;
        for (auto& [tid, members] : teams) {
            if (!first_team) matchup += " vs ";
            first_team = false;
            for (size_t i = 0; i < members.size(); i++) {
                if (i > 0) matchup += " & ";
                matchup += members[i];
            }
        }
    } else {
        bool first = true;
        for (int i = 0; i < 4; i++) {
            if (game.players[i].type == 3) continue;
            if (!first) matchup += " vs ";
            first = false;
            matchup += player_label(game.players[i]);
        }
    }

    return date + " - " + matchup + " - " + stage_name(game.stage_id) + ".slp";
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    bool dry_run = false;
    std::string target;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-n") dry_run = true;
        else             target  = arg;
    }

    if (target.empty()) {
        std::cerr << "Usage: " << argv[0] << " [-n] <file.slp | directory>\n";
        return 1;
    }

    std::vector<fs::path> files;
    if (fs::is_directory(target)) {
        for (auto& entry : fs::directory_iterator(target)) {
            if (entry.path().extension() == ".slp")
                files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());
    } else {
        files.push_back(target);
    }

    int ok = 0, failed = 0;
    for (auto& path : files) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "Could not open: " << path << "\n";
            failed++;
            continue;
        }
        size_t file_size = static_cast<size_t>(file.tellg());
        file.seekg(0);

        // Read only the beginning (header + Game Start) and end (metadata).
        // Skips the large frame data block in the middle.
        // 4KB is enough for Event Payloads + Game Start in all known Slippi versions.
        const size_t RAW_HEAD = 4096;
        size_t head_size = std::min(15 + RAW_HEAD, file_size);
        std::vector<uint8_t> head(head_size);
        file.read(reinterpret_cast<char*>(head.data()), head_size);

        if (head_size < 15) {
            std::cerr << "File too small: " << path.filename().string() << "\n";
            failed++;
            continue;
        }

        uint32_t raw_length = read_u32(head.data() + 11);
        size_t meta_offset  = 15 + raw_length;

        std::vector<uint8_t> meta_buf;
        if (meta_offset < file_size) {
            size_t meta_size = file_size - meta_offset;
            meta_buf.resize(meta_size);
            file.seekg(static_cast<std::streamoff>(meta_offset));
            file.read(reinterpret_cast<char*>(meta_buf.data()), meta_size);
        }

        SlpGame game;
        const uint8_t* meta_ptr  = meta_buf.empty() ? nullptr : meta_buf.data();
        size_t         meta_size = meta_buf.size();
        if (!parse_slp(head.data(), head_size, meta_ptr, meta_size, game)) {
            std::cerr << "Failed to parse: " << path.filename().string() << "\n";
            failed++;
            continue;
        }

        std::string new_name = generate_filename(game, path.filename().string());
        if (new_name.empty()) {
            std::cerr << "Could not generate name for: " << path.filename().string() << "\n";
            failed++;
            continue;
        }

        fs::path new_path = path.parent_path() / new_name;
        std::cout << path.filename().string() << " -> " << new_name << "\n";

        if (!dry_run && path != new_path) {
            std::error_code ec;
            fs::rename(path, new_path, ec);
            if (ec) std::cerr << "Rename failed: " << ec.message() << "\n";
        }
        ok++;
    }

    std::cerr << ok << " files processed, " << failed << " failed.\n";
    return failed > 0 ? 1 : 0;
}
