#include <cstddef>
#include <cstdint>
#include "world_serializer.h"
#include "gamevars.h"

using namespace ZZT;

// utility functions

static Tile ioReadTile(IOStream &stream) {
    uint8_t e = stream.read8();
    uint8_t c = stream.read8();
    return { .element = e, .color = c };
}

static void ioWriteTile(IOStream &stream, Tile &tile) {
    stream.write8(tile.element);
    stream.write8(tile.color);
}

// ZZT-format serializer

SerializerFormatZZT::SerializerFormatZZT(WorldFormat format) {
    this->format = format;
}

size_t SerializerFormatZZT::estimate_board_size(Board &board) {
    size_t len = 51; // name size
    len += board.width() * board.height() * 3; // maximum RLE size
    len += 86 + 2; // header size + stat count
    len += 33 * (board.stats.count + 1); // stat size
    for (int i = 0; i <= board.stats.count; i++) {
        if (board.stats[i].data.len > 0) {
            bool found = false;
            for (int j = (i - 1); j >= 1; j--) {
                if (board.stats[j].data == board.stats[i].data) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                len += board.stats[i].data.len; // stat data size
            }
        }
    }
    return len;
}

bool SerializerFormatZZT::serialize_board(Board &board, IOStream &stream) {
    bool packed = format == WorldFormatInternal;

    stream.write_pstring(board.name, 50, packed);

    int16_t ix = 1;
    int16_t iy = 1;
    RLETile rle = {
        .count = 1,
        .tile = board.tiles.get(ix, iy)
    };
    do {
        ix++;
        if (ix > board.width()) {
            ix = 1;
            iy++;
        }
        Tile cur = board.tiles.get(ix, iy);
        if (cur.color == rle.tile.color && cur.element == rle.tile.element && rle.count < 255 && iy <= board.height()) {
            rle.count++;
        } else {
            stream.write8(rle.count);
            ioWriteTile(stream, rle.tile);
            rle.tile = cur;
            rle.count = 1;
        }
    } while (iy <= board.height());

    stream.write8(board.info.max_shots);
    stream.write_bool(board.info.is_dark);
    for (int i = 0; i < 4; i++)
        stream.write8(board.info.neighbor_boards[i]);
    stream.write_bool(board.info.reenter_when_zapped);
    stream.write_pstring(board.info.message, 58, packed);
    stream.write8(board.info.start_player_x);
    stream.write8(board.info.start_player_y);
    stream.write16(board.info.time_limit_seconds);
    if (!packed) stream.skip(16);

    stream.write16(board.stats.count);
    for (int i = 0; i <= board.stats.count; i++) {
        Stat& stat = board.stats[i];
        int16_t len = stat.data.len;

        if (len > 0) {
            // OpenZoo: Reverse order mimics ZZT's missing break.
            for (int j = (i - 1); j >= 1; j--) {
                if (board.stats[j].data == stat.data) {
                    len = -j;
                    break;
                }
            }
        }

        bool storeStepXY = !packed || ((stat.step_x != 0) || (stat.step_y != 0));
        bool storeFollower = !packed || ((stat.follower != -1) || (stat.leader != -1));
        if (packed) {
            uint8_t flags = (storeFollower ? 1 : 0) | (storeStepXY ? 2 : 0);
            stream.write8(flags);
        }

        stream.write8(stat.x);
        stream.write8(stat.y);
        if (storeStepXY) {
            stream.write16(stat.step_x);
            stream.write16(stat.step_y);
        }
        stream.write16(stat.cycle);
        stream.write8(stat.p1);
        stream.write8(stat.p2);
        stream.write8(stat.p3);
        if (storeFollower) {
            stream.write16(stat.follower);
            stream.write16(stat.leader);
        }
        ioWriteTile(stream, stat.under);
        if (!packed) stream.write32(0); // Data pointer
        stream.write16(stat.data_pos);
        stream.write16(len);

        if (format == WorldFormatZZT) {
            // ZZT contains eight unused bytes at the end.
            stream.skip(8);
        }

        if (len > 0) {
#ifdef ROM_POINTERS
            if (format == WorldFormatInternal) {
                // RAM (ROM + difference)
                stream.write32((uint32_t) stat.data.data_rom);
                uint16_t diffs = 0;
                if (memcmp(stat.data.data_rom, stat.data.data, len) != 0) {
                    for (uint16_t di = 0; di < len; di++) {
                        if (stat.data.data_rom[di] != stat.data.data[di]) {
                            diffs++;
                        }
                    }
                    stream.write16(diffs);
                    for (uint16_t di = 0; di < len && diffs > 0; di++) {
                        if (stat.data.data_rom[di] != stat.data.data[di]) {
                            stream.write16(di);
                            stream.write8(stat.data.data[di]);
                            diffs--;
                        }
                    }
                } else {
                    stream.write16(0);
                }
            } else {
                stream.write((uint8_t*) stat.data.data, stat.data.len);
            }
#else
            stream.write((uint8_t*) stat.data.data, stat.data.len);
#endif
        }
    }

    return !stream.errored(); // TODO
}

bool SerializerFormatZZT::deserialize_board(Board &board, IOStream &stream) {
    bool packed = format == WorldFormatInternal;

    stream.read_pstring(board.name, StrSize(board.name), 50, packed);

    int16_t ix = 1;
    int16_t iy = 1;
    RLETile rle = {
        .count = 0
    };
    do {
        if (rle.count <= 0) {
            rle.count = stream.read8();
            rle.tile = ioReadTile(stream);
        }
        board.tiles.set(ix, iy, rle.tile);
        ix++;
        if (ix > board.width()) {
            ix = 1;
            iy++;
        }
        rle.count--;
    } while (iy <= board.height());

    board.info.max_shots = stream.read8();
    board.info.is_dark = stream.read_bool();
    for (int i = 0; i < 4; i++)
        board.info.neighbor_boards[i] = stream.read8();
    board.info.reenter_when_zapped = stream.read_bool();
    stream.read_pstring(board.info.message, StrSize(board.info.message), 58, packed);
    board.info.start_player_x = stream.read8();
    board.info.start_player_y = stream.read8();
    board.info.time_limit_seconds = stream.read16();
    if (!packed) stream.skip(16);

    board.stats.count = stream.read16();

    for (int i = 0; i <= board.stats.count; i++) {
        Stat& stat = board.stats[i];

        uint8_t flags = packed ? stream.read8() : 0xFF;
        bool storeStepXY = (flags & 2) != 0;
        bool storeFollower = (flags & 1) != 0;

        stat.x = stream.read8();
        stat.y = stream.read8();
        stat.step_x = storeStepXY ? stream.read16() : 0;
        stat.step_y = storeStepXY ? stream.read16() : 0;
        stat.cycle = stream.read16();
        stat.p1 = stream.read8();
        stat.p2 = stream.read8();
        stat.p3 = stream.read8();
        stat.follower = storeFollower ? stream.read16() : -1;
        stat.leader = storeFollower ? stream.read16() : -1;
        stat.under = ioReadTile(stream);
        if (!packed) stream.skip(4); // Data pointer
        stat.data_pos = stream.read16();
        int16_t len = stream.read16();

        if (format == WorldFormatZZT) {
            // ZZT contains eight unused bytes at the end.
            stream.skip(8);
        }

        if (len < 0) {
            stat.data = board.stats[-len].data;
        } else {
            stat.data.alloc_data(len);
            if (len > 0) {
#ifdef ROM_POINTERS
                if (format == WorldFormatInternal) {
                    // RAM (ROM + difference)
                    stat.data.data_rom = (const char*) stream.read32();
                    memcpy(stat.data.data, stat.data.data_rom, len);
                    uint16_t diffs = stream.read16();
                    for (uint16_t di = 0; di < diffs; di++) {
                        uint16_t pos = stream.read16();
                        char value = (char) stream.read8();
                        stat.data.data[pos] = value;
                    }
                } else {
                    // ROM
                    stat.data.data_rom = (const char *) (static_cast<MemoryIOStream*>(&stream)->ptr());
                    stream.read((uint8_t*) stat.data.data, len);
                }
#else
                stream.read((uint8_t*) stat.data.data, len);
#endif
            }
        }
    }

    return !stream.errored(); // TODO
}

bool SerializerFormatZZT::serialize_world(World &world, IOStream &stream, std::function<void(int)> ticker) {
    if (format == WorldFormatInternal) {
        return false; // we do not serialize/deserialize to internal
    }

    stream.write16(-1); /* version */
    stream.write16(world.board_count);

    stream.write16(world.info.ammo);
    stream.write16(world.info.gems);
    for (int i = 0; i < 7; i++) {
        stream.write_bool(world.info.keys[i]);
    }
    stream.write16(world.info.health);
    stream.write16(world.info.current_board);
    stream.write16(world.info.torches);
    stream.write16(world.info.torch_ticks);
    stream.write16(world.info.energizer_ticks);
    stream.write16(0);
    stream.write16(world.info.score);
    stream.write_pstring(world.info.name, 20, false);
    for (int i = 0; i < MAX_FLAG; i++) {
        stream.write_pstring(world.info.flags[i], 20, false);
    }
    stream.write16(world.info.board_time_sec);
    stream.write16(world.info.board_time_hsec);
    stream.write_bool(world.info.is_save);

    stream.skip(512 - stream.tell());

    if (stream.errored()) return false;

    for (int bid = 0; bid <= world.board_count; bid++) {
        uint8_t *data;
        uint16_t len;
        bool temporary;

        world.get_board(bid, data, len, temporary, format);

        stream.write16(len);
        stream.write(data, len);
        if (temporary) free(data);

        if (stream.errored()) break;
    }
    return !stream.errored();
}

bool SerializerFormatZZT::deserialize_world(World &world, IOStream &stream, bool titleOnly, std::function<void(int)> ticker) {
    if (format == WorldFormatInternal) {
        return false; // we do not serialize/deserialize to internal
    }

    world.board_count = stream.read16();
    if (world.board_count < 0) {
        if (world.board_count != -1) {
            // TODO UserInterface
            // video->draw_string(63, 5, 0x1E, "You need a newer");
            // video->draw_string(63, 6, 0x1E, " version of ZZT!");
            return false;
        } else {
            world.board_count = stream.read16();
        }
    }

    world.info.ammo = stream.read16();
    world.info.gems = stream.read16();
    for (int i = 0; i < 7; i++) {
        world.info.keys[i] = stream.read_bool();
    }
    world.info.health = stream.read16();
    world.info.current_board = stream.read16();
    world.info.torches = stream.read16();
    world.info.torch_ticks = stream.read16();
    world.info.energizer_ticks = stream.read16();
    stream.read16();
    world.info.score = stream.read16();
    stream.read_pstring(world.info.name, StrSize(world.info.name), 20, false);
    for (int i = 0; i < MAX_FLAG; i++) {
        stream.read_pstring(world.info.flags[i], StrSize(world.info.flags[i]), 20, false);
    }
    world.info.board_time_sec = stream.read16();
    world.info.board_time_hsec = stream.read16();
    world.info.is_save = stream.read_bool();

    stream.skip(512 - stream.tell());

    if (titleOnly) {
        world.board_count = 0;
        world.info.current_board = 0;
        world.info.is_save = true;
    }

    for (int bid = 0; bid <= world.board_count; bid++) {
        if (ticker != nullptr) {
            ticker(bid);
        }

        uint16_t len = stream.read16();
        if (stream.errored()) break;

        uint8_t *data = stream.ptr();
        if (data == nullptr) {
            data = (uint8_t*) malloc(len);
            stream.read(data, len);
            if (!stream.errored()) {
                world.set_board(bid, data, len, false, format);
            }
            free(data);
        } else {
            stream.skip(len);
#ifdef ROM_POINTERS
            world.set_board(bid, data, len, true, format);
#else
            world.set_board(bid, data, len, false, format);
#endif
        }

        if (stream.errored()) break;
    }

    return !stream.errored();
}