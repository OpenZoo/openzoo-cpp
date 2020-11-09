#include <cstddef>
#include <cstdint>
#include "utils.h"
#include "world_serializer.h"
#include "gamevars.h"

using namespace ZZT;
using namespace ZZT::Utils;

static Tile ioReadTile(IOStream &stream) {
    uint8_t e = stream.read8();
    uint8_t c = stream.read8();
    return { .element = e, .color = c };
}

static void ioWriteTile(IOStream &stream, Tile &tile) {
    stream.write8(tile.element);
    stream.write8(tile.color);
}

size_t SerializerFormatZZT::estimate_buffer_size(Board &board) {
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

bool SerializerFormatZZT::serialize(Board &board, IOStream &stream) {
    stream.write_pstring(board.name, 50, false);

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
    stream.write_pstring(board.info.message, 58, false);
    stream.write8(board.info.start_player_x);
    stream.write8(board.info.start_player_y);
    stream.write16(board.info.time_limit_seconds);
    stream.skip(16);

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

        stream.write8(stat.x);
        stream.write8(stat.y);
        stream.write16(stat.step_x);
        stream.write16(stat.step_y);
        stream.write16(stat.cycle);
        stream.write8(stat.p1);
        stream.write8(stat.p2);
        stream.write8(stat.p3);
        stream.write16(stat.follower);
        stream.write16(stat.leader);
        ioWriteTile(stream, stat.under);
        stream.write32(0);
        stream.write16(stat.data_pos);
        stream.write16(len);
        stream.skip(8);

        if (len > 0) {
            stream.write((uint8_t*) stat.data.data, stat.data.len);
        }
    }

    return !stream.errored(); // TODO
}

bool SerializerFormatZZT::deserialize(Board &board, IOStream &stream) {
    stream.read_pstring(board.name, StrSize(board.name), 50, false);

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
    stream.read_pstring(board.info.message, StrSize(board.info.message), 58, false);
    board.info.start_player_x = stream.read8();
    board.info.start_player_y = stream.read8();
    board.info.time_limit_seconds = stream.read16();
    stream.skip(16);

    board.stats.count = stream.read16();

    for (int i = 0; i <= board.stats.count; i++) {
        Stat& stat = board.stats[i];

        stat.x = stream.read8();
        stat.y = stream.read8();
        stat.step_x = stream.read16();
        stat.step_y = stream.read16();
        stat.cycle = stream.read16();
        stat.p1 = stream.read8();
        stat.p2 = stream.read8();
        stat.p3 = stream.read8();
        stat.follower = stream.read16();
        stat.leader = stream.read16();
        stat.under = ioReadTile(stream);
        stream.skip(4);
        stat.data_pos = stream.read16();
        int16_t len = stream.read16();
        stream.skip(8);

        if (len < 0) {
            stat.data = board.stats[-len].data;
        } else {
            stat.data.alloc_data(len);
            if (len > 0) {
                stream.read((uint8_t*) stat.data.data, len);
            }
        }
    }

    return !stream.errored(); // TODO
}

bool SerializerFormatZZT::serialize(World &world, IOStream &stream, std::function<void(int)> ticker) {
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
        stream.write16(world.board_len[bid]);
        if (stream.errored()) break;
        stream.write(world.board_data[bid], world.board_len[bid]);
        if (stream.errored()) break;
    }
    return !stream.errored();
}

bool SerializerFormatZZT::deserialize(World &world, IOStream &stream, bool titleOnly, std::function<void(int)> ticker) {
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

        world.board_len[bid] = stream.read16();
        if (stream.errored()) break;

        world.board_data[bid] = (uint8_t*) malloc(world.board_len[bid]);
        stream.read(world.board_data[bid], world.board_len[bid]);
    }

    return !stream.errored();
}