#include <cstdlib>
#include <sys/stat.h>
#include <dirent.h>
#include "editor.h"
#include "gamevars.h"
#include "txtwind.h"

using namespace ZZT;
using namespace ZZT::Utils;

static const uint8_t ProgressAnimColors[8] = {0x14, 0x1C, 0x15, 0x1D, 0x16, 0x1E, 0x17, 0x1F};
static const char *ProgressAnimStrings[8] = {
    "....|", "...*/", "..*.-", ".*..\\", "*...|", "..../", "....-", "....\\"
};
const char ZZT::ColorNames[7][8] = {
    "Blue", "Green", "Cyan", "Red", "Purple", "Yellow", "White"
};

const int16_t ZZT::DiagonalDeltaX[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
const int16_t ZZT::DiagonalDeltaY[8] = {1, 1, 1, 0, -1, -1, -1, 0};
const int16_t ZZT::NeighborDeltaX[4] = {0, 0, -1, 1};
const int16_t ZZT::NeighborDeltaY[4] = {-1, 1, 0, 0};

static const Tile TileBorder = {.element = ENormal, .color = 0x0E};
static const Tile TileBoardEdge = {.element = EBoardEdge, .color = 0x00};

const uint8_t ZZT::LineChars[16] = {
    249, 208, 210, 186, 181, 188, 187, 185, 198, 200, 201, 204, 205, 202, 203, 206
};

void Game::SidebarClearLine(int16_t y) {
    video->draw_string(60, y, 0x11, "                    ");
}

void Game::SidebarClear(void) {
    for (int i = 3; i <= 24; i++) {
        SidebarClearLine(i);
    }
}

void Game::GenerateTransitionTable(void) {
    transitionTableSize = 0;
    for (int16_t iy = 1; iy <= board.height(); iy++) {
        for (int16_t ix = 1; ix <= board.width(); ix++) {
            transitionTable[transitionTableSize] = {
                .x = ix,
                .y = iy
            };
            transitionTableSize++;
        }
    }

    for (int i = 0; i < transitionTableSize; i++) {
        int j = random.Next(transitionTableSize);
        Coord tmp = transitionTable[j];
        transitionTable[j] = transitionTable[i];
        transitionTable[i] = tmp;
    }
}

static Tile ioReadTile(IOStream &stream) {
    uint8_t e = stream.read8();
    uint8_t c = stream.read8();
    return { .element = e, .color = c };
}

static void ioWriteTile(IOStream &stream, Tile &tile) {
    stream.write8(tile.element);
    stream.write8(tile.color);
}

void Game::BoardClose() {
    MemoryIOStream stream(ioTmpBuf, sizeof(ioTmpBuf), true);

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
        if (stat.data_len > 0) {
            for (int j = 1; j <= (i - 1); j++) {
                if (board.stats[j].data == stat.data) {
                    stat.data_len = -j;
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
        stream.write16(stat.data_len);
        stream.skip(8);

        if (stat.data_len > 0) {
            stream.write((uint8_t*) stat.data, stat.data_len);
        }
    }

    if (world.board_len[world.info.current_board] > 0) {
        free(world.board_data[world.info.current_board]);
    }
    world.board_len[world.info.current_board] = stream.tell();
    world.board_data[world.info.current_board] = (uint8_t*) malloc(world.board_len[world.info.current_board]);
    memcpy(world.board_data[world.info.current_board], ioTmpBuf, world.board_len[world.info.current_board]);
}

void Game::BoardOpen(int16_t board_id) {
    if (board_id > world.board_count) {
        board_id = world.info.current_board;
    }

    MemoryIOStream stream(world.board_data[board_id], world.board_len[board_id], false);

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
        stat.data_len = stream.read16();
        stream.skip(8);

        if (stat.data_len > 0) {
            stat.data = (char*) malloc(stat.data_len);
            stream.read((uint8_t*) stat.data, stat.data_len);
        } else if (stat.data_len < 0) {
            stat.data = board.stats[-stat.data_len].data;
            stat.data_len = board.stats[-stat.data_len].data_len;
        }
    }
    
    world.info.current_board = board_id;
}

void Game::BoardChange(int16_t board_id) {
    board.tiles.set(board.stats[0].x, board.stats[0].y, {
        .element = EPlayer,
        .color = elementDefs[EPlayer].color
    });
    BoardClose();
    BoardOpen(board_id);
}

void Game::BoardCreate(void) {
    board.name[0] = '\0';
    board.info.message[0] = '\0';
    board.info.max_shots = 255;
    board.info.is_dark = false;
    board.info.reenter_when_zapped = false;
    board.info.time_limit_seconds = 0;
    for (int i = 0; i < 4; i++) {
        board.info.neighbor_boards[i] = 0;
    }

    for (int ix = 0; ix <= board.width() + 1; ix++) {
        board.tiles.set(ix, 0, TileBoardEdge);
        board.tiles.set(ix, board.height() + 1, TileBoardEdge);
    }
    for (int iy = 0; iy <= board.height() + 1; iy++) {
        board.tiles.set(0, iy, TileBoardEdge);
        board.tiles.set(board.width() + 1, iy, TileBoardEdge);
    }

    for (int ix = 1; ix <= board.width(); ix++) {
        for (int iy = 1; iy <= board.height(); iy++) {
            board.tiles.set(ix, iy, {.element = EEmpty, .color = 0x00});
        }
    }

    for (int ix = 1; ix <= board.width(); ix++) {
        board.tiles.set(ix, 1, TileBorder);
        board.tiles.set(ix, board.height(), TileBorder);
    }
    for (int iy = 1; iy <= board.height(); iy++) {
        board.tiles.set(1, iy, TileBorder);
        board.tiles.set(board.width(), iy, TileBorder);
    }

    int16_t playerX = board.width() / 2;
    int16_t playerY = board.height() / 2;

    board.tiles.set(playerX, playerY, {.element = EPlayer, .color = elementDefs[EPlayer].color});

    board.stats.count = 0;
    board.stats[0] = Stat();
    board.stats[0].x = playerX;
    board.stats[0].y = playerY;
    board.stats[0].cycle = 1;
}

void Game::WorldCreate(void) {
    InitElementsGame();
    world.board_count = 0;
    world.board_len[0] = 0;
    playerDirX = 0; // from ELEMENTS -> InitEditorStatSettings
    playerDirY = 0;
    ResetMessageNotShownFlags();
    BoardCreate();
    world.info.is_save = false;
    world.info.current_board = 0;
    world.info.ammo = 0;
    world.info.gems = 0;
    world.info.health = 100;
    world.info.energizer_ticks = 0;
    world.info.torches = 0;
    world.info.torch_ticks = 0;
    world.info.score = 0;
    world.info.board_time_sec = 0;
    world.info.board_time_hsec = 0;
    for (int i = 0; i < 7; i++) {
        world.info.keys[i] = false;
    }
    for (int i = 0; i < 10; i++) {
        world.info.flags[i][0] = '\0';
    }
    BoardChange(0);
    StrCopy(board.name, "Title screen");
    loadedGameFileName[0] = '\0';
    world.info.name[0] = '\0';
}

void Game::TransitionDrawToFill(uint8_t chr, uint8_t color) {
    for (int i = 0; i < transitionTableSize; i++) {
        video->draw_char(transitionTable[i].x - 1, transitionTable[i].y - 1, color, chr);
    }
}

void Game::BoardDrawTile(int16_t x, int16_t y) {
    Tile tile = board.tiles.get(x, y);
    if (!board.info.is_dark
        || elementDefs[tile.element].visible_in_dark
        || (
            (world.info.torch_ticks > 0)
            && ((Sqr(board.stats[0].x - x) + Sqr(board.stats[0].y - y) * 2) < TORCH_DIST_SQR)
        ) || forceDarknessOff
    ) {
        if (tile.element == EEmpty) {
            video->draw_char(x - 1, y - 1, 0x0F, ' ');
        } else if (elementDefs[tile.element].has_draw_proc) {
            uint8_t ch;
            elementDefs[tile.element].draw(*this, x, y, ch);
            video->draw_char(x - 1, y - 1, tile.color, ch);
        } else if (tile.element < ETextBlue) {
            video->draw_char(x - 1, y - 1, tile.color, elementDefs[tile.element].character);
        } else {
            if (tile.element == ETextWhite) {
                video->draw_char(x - 1, y - 1, 0x0F, tile.color);
            } else if (video->monochrome) {
                video->draw_char(x - 1, y - 1, ((tile.element - ETextBlue + 1) << 4), tile.color);
            } else {
                video->draw_char(x - 1, y - 1, ((tile.element - ETextBlue + 1) << 4) | 0x0F, tile.color);
            }
        }
    } else {
        // darkness
        video->draw_char(x - 1, y - 1, 0x07, 176);
    }
}

void Game::BoardDrawBorder(void) {
    for (int ix = 1; ix <= board.width(); ix++) {
        BoardDrawTile(ix, 1);
        BoardDrawTile(ix, board.height());
    }

    for (int iy = 1; iy <= board.height(); iy++) {
        BoardDrawTile(1, iy);
        BoardDrawTile(board.width(), iy);
    }
}

void Game::TransitionDrawToBoard(void) {
    for (int i = 0; i < transitionTableSize; i++) {
        BoardDrawTile(transitionTable[i].x, transitionTable[i].y);
    }
}

void Game::SidebarPromptCharacter(bool editable, int16_t x, int16_t y, const char *prompt, uint8_t &value) {
    SidebarClearLine(y);
    video->draw_string(x, y, editable ? 0x1F : 0x1E, prompt);
    SidebarClearLine(y + 1);
    video->draw_char(x + 5, y + 1, 0x9F, 31);
    SidebarClearLine(y + 2);

    do {
        for (int i = value - 4; i <= value + 4; i++) {
            video->draw_char(x + i - value + 5, y + 2, 0x1E, (uint8_t) (i & 0xFF));
        }

        if (editable) {
            sound->delay(25);
            input->update_input();
            if (input->keyPressed == KeyTab) {
                input->deltaX = 9;
            }

            uint8_t new_value = (uint8_t) (value + input->deltaX);
            if (value != new_value) {
                value = new_value;
                SidebarClearLine(y + 2);
            }
        }
    } while (input->keyPressed != KeyEnter && input->keyPressed != KeyEscape && editable && !input->shiftPressed);

    video->draw_char(x + 5, y + 1, 0x1F, 31);
}

void Game::SidebarPromptSlider(bool editable, int16_t x, int16_t y, const char *prompt, uint8_t &value) {
    sstring<20> str;
    char startChar = '1', endChar = '9';
    int prompt_len = strlen(prompt);

    StrCopy(str, prompt);
    if (prompt_len >= 3 && prompt[prompt_len - 3] == ';') {
        startChar = prompt[prompt_len - 2];
        endChar = prompt[prompt_len - 1];
        str[prompt_len - 3] = 0;
    }

    SidebarClearLine(y);
    video->draw_string(x, y, editable ? 0x1F : 0x1E, str);
    SidebarClearLine(y + 1);
    SidebarClearLine(y + 2);
    StrCopy(str, "?....:....?");
    str[0] = startChar;
    str[10] = endChar;
    video->draw_string(x, y + 2, 0x1E, str);

    do {
        if (editable) {
            if (input->joystickMoved) {
                sound->delay(45);
            } else {
                sound->idle(IMUntilFrame);
            }
            video->draw_char(x + value + 1, y + 1, 0x9F, 31);

            input->update_input();
            if (input->keyPressed >= '1' && input->keyPressed <= '9') {
                value = input->keyPressed - 49;
                SidebarClearLine(y + 1);
            } else {
                uint8_t new_value = (uint8_t) (value + input->deltaX);
                if (value != new_value && new_value >= 0 && new_value <= 8) {
                    value = new_value;
                    SidebarClearLine(y + 1);
                }
            }
        }
    } while (input->keyPressed != KeyEnter && input->keyPressed != KeyEscape && editable && !input->shiftPressed);

    video->draw_char(x + value + 1, y + 1, 0x1F, 31);
}

void Game::SidebarPromptChoice(bool editable, int16_t y, const char *prompt, const char *choiceStr, uint8_t &result) {
    SidebarClearLine(y);
    SidebarClearLine(y + 1);
    SidebarClearLine(y + 2);
    video->draw_string(63, y, editable ? 0x1F : 0x1E, prompt);
    video->draw_string(63, y + 2, 0x1E, choiceStr);

    int choice_count = 1;
    int choiceStr_len = strlen(choiceStr);
    for (int i = 0; i < choiceStr_len; i++) {
        if (choiceStr[i] == ' ') choice_count++;
    }

    int16_t i, j;
    do {
        j = 0;
        i = 0;
        while (j < result && i < choiceStr_len) {
            if (choiceStr[i] == ' ') j++;
            i++;
        }

        if (editable) {
            video->draw_char(63 + i, y + 1, 0x9F, 31);
            sound->delay(35);
            input->update_input();

            uint8_t new_result = (uint8_t) (result + input->deltaX);
            if (new_result != result && new_result >= 0 && new_result < choice_count) {
                result = new_result;
                SidebarClearLine(y + 1);
            }
        }
    } while (input->keyPressed != KeyEnter && input->keyPressed != KeyEscape && editable && !input->shiftPressed);

    video->draw_char(63 + i, y + 1, 0x1F, 31);
}

void Game::SidebarPromptDirection(bool editable, int16_t y, const char *prompt, int16_t &dx, int16_t &dy) {
    uint8_t choice;
    if (dy == -1) choice = 0;
    else if (dy == 1) choice = 1;
    else if (dx == -1) choice = 2;
    else choice = 3;
    SidebarPromptChoice(editable, y, prompt, "\x18 \x19 \x1B \x1A", choice);
    dx = NeighborDeltaX[choice];
    dy = NeighborDeltaY[choice];
}

void Game::PromptString(int16_t x, int16_t y, uint8_t arrowColor, uint8_t color, int16_t width, PromptMode mode, char *buffer, int buflen) {
    sstring<255> oldBuffer;
    StrCopy(oldBuffer, buffer);
    bool firstKeyPress = true;
    do {
        for (int i = 0; i < (width - 1); i++) {
            video->draw_char(x + i, y, color, ' ');
            video->draw_char(x + i, y - 1, arrowColor, ' ');
        }
        video->draw_char(x + width, y - 1, arrowColor, ' ');
        video->draw_char(x + strlen(buffer), y - 1, arrowColor | 0x0F, 31);
        video->draw_string(x, y, color, buffer);

        input->read_wait_key();

        if (strlen(buffer) < width && input->keyPressed >= 32 && input->keyPressed < 128) {
            if (firstKeyPress) {
                buffer[0] = 0;
            }
            char chAppend = 0;
            switch (mode) {
                case PMNumeric:
                    if (input->keyPressed >= '0' && input->keyPressed <= '9') {
                        chAppend = input->keyPressed;
                    }
                    break;
                case PMAny:
                    chAppend = input->keyPressed;
                    break;
                case PMAlphanum:
                    if (
                        (input->keyPressed >= 'A' && input->keyPressed <= 'Z')
                        || (input->keyPressed >= 'a' && input->keyPressed <= 'z')
                        || (input->keyPressed >= '0' && input->keyPressed <= '9')
                        || (input->keyPressed == '-')
                    ) {
                        chAppend = UpCase(input->keyPressed);
                    }
                    break;
            }
            int pos = strlen(buffer);
            if (chAppend != 0 && pos < (buflen - 1)) {
                buffer[pos++] = chAppend;
                buffer[pos] = 0;
            }
        } else if (input->keyPressed == KeyLeft || input->keyPressed == KeyBackspace) {
            int pos = strlen(buffer);
            if (pos > 0) {
                buffer[pos - 1] = 0;
            }
        }

        firstKeyPress = false;
    } while (input->keyPressed != KeyEnter && input->keyPressed != KeyEscape);

    if (input->keyPressed == KeyEscape) {
        strcpy(buffer, oldBuffer);
    }
}

bool Game::SidebarPromptYesNo(const char *message, bool defaultReturn) {
    SidebarClearLine(3);
    SidebarClearLine(4);
    SidebarClearLine(5);
    video->draw_string(63, 5, 0x1F, message);
    video->draw_char(63 + strlen(message), 5, 0x9E, '_');

    do {
        input->read_wait_key();
    } while (input->keyPressed != KeyEscape && UpCase(input->keyPressed) != 'Y' && UpCase(input->keyPressed) != 'N');
    defaultReturn = UpCase(input->keyPressed) == 'Y';

    SidebarClearLine(5);
    return defaultReturn;
}

void Game::SidebarPromptString(const char *prompt, const char *extension, char *filename, int filenameLen, PromptMode mode) {
    SidebarClearLine(3);
    SidebarClearLine(4);
    SidebarClearLine(5);
    video->draw_string(75 - strlen(prompt), 3, 0x1F, prompt);
    video->draw_string(63, 5, 0x0F, "        ");
    video->draw_string(71, 5, 0x0F, extension);
    
    PromptString(63, 5, 0x1E, 0x0F, 8, mode, filename, filenameLen);

    SidebarClearLine(3);
    SidebarClearLine(4);
    SidebarClearLine(5);
}

void Game::PauseOnError(void) {
    uint8_t error_notes[32];
    sound->sound_queue(1, error_notes, SoundParse("s004x114x9", error_notes, 32));
    sound->delay(2000);
}

void Game::DisplayIOError(IOStream &stream) {
    if (!stream.errored()) return;

    // TODO [ZZT]
}

void Game::WorldUnload(void) {
    BoardClose();
    for (int i = 0; i <= world.board_count; i++) {
        if (world.board_len[i] > 0) {
            free(world.board_data[i]);
        }
    }
}

bool Game::WorldLoad(const char *filename, const char *extension, bool titleOnly) {
    SidebarClearLine(4);
    SidebarClearLine(5);
    video->draw_string(62, 5, 0x1F, "Loading.....");

    char joinedName[256];
    StrJoin(joinedName, 2, filename, extension);
    FileIOStream stream(joinedName, false);

    if (!stream.errored()) {
        WorldUnload();
        world.board_count = stream.read16();
        if (world.board_count < 0) {
            if (world.board_count != -1) {
                video->draw_string(63, 5, 0x1E, "You need a newer");
                video->draw_string(63, 6, 0x1E, " version of ZZT!");
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
            world.board_len[bid] = stream.read16();
            if (stream.errored()) break;

            world.board_data[bid] = (uint8_t*) malloc(world.board_len[bid]);
            stream.read(world.board_data[bid], world.board_len[bid]);
        }

        if (!stream.errored()) {
            BoardOpen(world.info.current_board);
            StrCopy(loadedGameFileName, filename);
            highScoreList.Load(world.info.name);
            SidebarClearLine(5);
            return true;
        }
    }

    if (stream.errored()) {
        DisplayIOError(stream);
    }
    return false;
}

bool Game::WorldSave(const char *filename, const char *extension) {
    BoardClose();
    video->draw_string(63, 5, 0x1F, "Saving...");

    char joinedName[256];
    StrJoin(joinedName, 2, filename, extension);
    FileIOStream stream(joinedName, true);

    if (!stream.errored()) {
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

        if (!stream.errored()) {
            for (int bid = 0; bid <= world.board_count; bid++) {
                stream.write16(world.board_len[bid]);
                if (stream.errored()) break;
                stream.write(world.board_data[bid], world.board_len[bid]);
                if (stream.errored()) break;
            }

            if (!stream.errored()) {
                BoardOpen(world.info.current_board);
                SidebarClearLine(5);
                return true;
            }
        }
    }

    if (stream.errored()) {
        stream = FileIOStream(joinedName, true);
    }
    BoardOpen(world.info.current_board);
    SidebarClearLine(5);
    return false;
}

void Game::GameWorldSave(const char *prompt, char* filename, size_t filename_len, const char *extension) {
    sstring<50> newFilename;
    StrCopy(newFilename, filename);
    SidebarPromptString(prompt, extension, newFilename, sizeof(newFilename), PMAlphanum);
    if (input->keyPressed != KeyEscape && newFilename[0] != 0) {
        strncpy(filename, newFilename, filename_len - 1);
        filename[filename_len - 1] = 0;

        if (StrEquals(extension, ".ZZT")) {
            StrCopy(world.info.name, filename);
        }

        WorldSave(filename, extension);
    }
}

bool Game::GameWorldLoad(const char *extension) {
    int ext_len = strlen(extension);
    TextWindow window = TextWindow(video, input, sound);
    StrCopy(window.title, StrEquals(extension, ".ZZT") ? "ZZT Worlds" : "Saved Games");
    window.selectable = true;

    DIR *dir;
    struct dirent *entry;
    struct stat st;
    if ((dir = opendir(".")) == NULL) {
        return false;
    }

    while ((entry = readdir(dir)) != NULL) {
        stat(entry->d_name, &st);
        int name_len = strlen(entry->d_name);
        if (S_ISDIR(st.st_mode)) continue;
        if (name_len < ext_len) continue;

        bool match = true;
        for (int i = 0; i < ext_len; i++) {
            if (entry->d_name[name_len - ext_len + i] != extension[i]) {
                match = false;
                break;
            }
        }
        if (!match) continue;

        sstring<20> wname;
        StrCopy(wname, entry->d_name);
        char *extpos = strchr(wname, '.');
        if (extpos != NULL) {
            *extpos = 0;
        }
        window.Append(wname);
    }
    window.Append("Exit");

    window.DrawOpen();
    window.Select(false, false);
    window.DrawClose();

    if (window.line_pos < (window.line_count - 1) && !window.rejected) {
        const char *filename = window.lines[window.line_pos]->c_str();
        bool result = WorldLoad(filename, extension, false);
        TransitionDrawToFill(219, 0x44);
        return result;
    }

    return false;
}

void ZZT::CopyStatDataToTextWindow(const Stat &stat, TextWindow &window) {
    const char *data_ptr = stat.data;
    std::string s = "";
    int data_str_pos = 0;
    
    window.Clear();

    for (int i = 0; i < stat.data_len; i++, data_ptr++) {
        char ch = *data_ptr;
        if (ch == '\r') {
            window.Append(s);
            s = "";
        } else {
            s += ch;
        }
    }

    if (s.length() > 0) {
        window.Append(s);
    }
}

void Game::AddStat(int16_t x, int16_t y, uint8_t element, uint8_t color, int16_t cycle, Stat tpl) {
    if (board.stats.count < MAX_STAT) {
        board.stats.count++;

        Stat& stat = board.stats[board.stats.count];
        stat = tpl;
        stat.x = x;
        stat.y = y;
        stat.cycle = cycle;
        stat.under = board.tiles.get(x, y);
        stat.data_pos = 0;

        if (tpl.data != nullptr) {
            stat.data = (char*) malloc(tpl.data_len);
            memcpy(stat.data, tpl.data, stat.data_len);
        }

        board.tiles.set(x, y, {
            .element = element,
            .color = (uint8_t) (elementDefAt(x, y).placeable_on_top
                ? ((color & 0xF) | (board.tiles.get(x, y).color & 0x70))
                : color)
        });

        if (y > 0) {
            BoardDrawTile(x, y);
        }
    }
}

void Game::RemoveStat(int16_t stat_id) {
    Stat& stat = board.stats[stat_id];

    if (stat.data_len != 0) {
        bool found = false;
        for (int i = 1; i <= board.stats.count; i++) {
            if (i == stat_id) continue;
            if (board.stats[i].data == stat.data) {
                found = true;
                break;
            }
        }
        if (!found) {
            free(stat.data);
            stat.data = nullptr;
        }
    }

    if (stat_id < currentStatTicked) {
        currentStatTicked--;
    }

    board.tiles.set(stat.x, stat.y, stat.under);
    if (stat.y > 0) {
        BoardDrawTile(stat.x, stat.y);
    }

    for (int i = 1; i <= board.stats.count; i++) {
        Stat& other = board.stats[i];
        
        if (other.follower >= stat_id) {
            other.follower = (other.follower == stat_id) ? -1 : (other.follower - 1);
        }

        if (other.leader >= stat_id) {
            other.leader = (other.leader == stat_id) ? -1 : (other.leader - 1);
        }
    }

    for (int i = stat_id + 1; i <= board.stats.count; i++) {
        board.stats[i - 1] = board.stats[i];
    }
    board.stats.count--;
}

bool Game::BoardPrepareTileForPlacement(int16_t x, int16_t y) {
    int16_t stat_id = board.stats.id_at(x, y);
    bool result = true;

    if (stat_id > 0) {
        RemoveStat(stat_id);
    } else if (stat_id < 0) {
        if (!elementDefAt(x, y).placeable_on_top) {
            board.tiles.set_element(x, y, EEmpty);
        }
    } else {
        // player (stat 0) cannot be modified
        result = false;
    }

    BoardDrawTile(x, y);
    return result;
}

void Game::MoveStat(int16_t stat_id, int16_t newX, int16_t newY) {
    Stat& stat = board.stats[stat_id];
    
    Tile oldUnder = stat.under;
    stat.under = board.tiles.get(newX, newY);

    const Tile& curTile = board.tiles.get(stat.x, stat.y);
    Tile newTile = curTile;

    if (curTile.element == EPlayer) {
        // ignore color change
    } else if (stat.under.element == EEmpty) {
        newTile.color &= 0xF; // strip background color
    } else {
        // copy background color from target tile
        newTile.color = (curTile.color & 0xF) | (stat.under.color & 0x70);
    }

    board.tiles.set(newX, newY, newTile);
    board.tiles.set(stat.x, stat.y, oldUnder);

    int16_t oldX = stat.x;
    int16_t oldY = stat.y;
    stat.x = newX;
    stat.y = newY;

    BoardDrawTile(stat.x, stat.y);
    BoardDrawTile(oldX, oldY);

    if (stat_id == 0 && board.info.is_dark && world.info.torch_ticks > 0) {
        if ((Sqr(oldX - stat.x) + Sqr(oldY - stat.y)) == 1) {
            for (int ix = (stat.x - TORCH_DX - 3); ix <= (stat.x + TORCH_DX + 3); ix++) {
                if (ix < 1 || ix > board.width()) continue;
                for (int iy = (stat.y - TORCH_DY - 3); iy <= (stat.y + TORCH_DY + 3); iy++) {
                    if (iy < 1 || iy > board.height()) continue;
                    if ((((Sqr(ix-oldX))+(Sqr(iy-oldY)*2)) < TORCH_DIST_SQR) !=
                        (((Sqr(ix-newX))+(Sqr(iy-newY)*2)) < TORCH_DIST_SQR))
                    {
                        BoardDrawTile(ix, iy);
                    }
                }
            }
        } else {
            DrawPlayerSurroundings(oldX, oldY, 0);
            DrawPlayerSurroundings(stat.x, stat.y, 0);
        }
    }
}

void Game::PopupPromptString(const char *question, char *buffer, size_t buffer_len) {
    int x = 3;
    int y = 18;
    int width = 50;
    uint8_t color = 0x4F;

    VideoCopy copy = VideoCopy(width, 6);
    video->copy_chars(copy, x, y, width, 6, 0, 0);

    TextWindowDrawPattern(video, x, y, width, color, WinPatTop);
    TextWindowDrawPattern(video, x, y + 1, width, color, WinPatInner);
    TextWindowDrawPattern(video, x, y + 2, width, color, WinPatSeparator);
    TextWindowDrawPattern(video, x, y + 3, width, color, WinPatInner);
    TextWindowDrawPattern(video, x, y + 4, width, color, WinPatInner);
    TextWindowDrawPattern(video, x, y + 5, width, color, WinPatBottom);
    video->draw_string(x + 1 + ((width - strlen(question)) / 2), y + 1, color, question);

    buffer[0] = 0;
    PromptString(x + 7, y + 4, (color & 0xF0) | 0x0F, (color & 0xF0) | 0x0E, width - 16, PMAny, buffer, buffer_len);

    video->paste_chars(copy, 0, 0, width, 6, x, y);
}

void Game::GameDrawSidebar(void) {
    SidebarClear();
    SidebarClearLine(0);
    SidebarClearLine(1);
    SidebarClearLine(2);
    video->draw_string(61, 0, 0x1F, "    - - - - -      ");
    video->draw_string(62, 1, 0x70, "      ZZT      ");
    video->draw_string(61, 2, 0x1F, "    - - - - -      ");
    if (gameStateElement == EPlayer) {
        video->draw_string(64, 7, 0x1E,  " Health:");
        video->draw_string(64, 8, 0x1E,  "   Ammo:");
        video->draw_string(64, 9, 0x1E,  "Torches:");
        video->draw_string(64, 10, 0x1E, "   Gems:");
        video->draw_string(64, 11, 0x1E, "  Score:");
        video->draw_string(64, 12, 0x1E, "   Keys:");
        video->draw_char(62, 7, 0x1F, elementDefs[EPlayer].character);
        video->draw_char(62, 8, 0x1B, elementDefs[EAmmo].character);
        video->draw_char(62, 9, 0x16, elementDefs[ETorch].character);
        video->draw_char(62, 10, 0x1B, elementDefs[EGem].character);
        video->draw_char(62, 12, 0x1F, elementDefs[EKey].character);
        video->draw_string(62, 14, 0x70, " T ");
        video->draw_string(65, 14, 0x1F, " Torch");
        video->draw_string(62, 15, 0x30, " B ");
        video->draw_string(62, 16, 0x70, " H ");
        video->draw_string(65, 16, 0x1F, " Help");
        video->draw_string(67, 18, 0x30, " \x18\x19\x1A\x1B ");
        video->draw_string(72, 18, 0x1F, " Move");
        video->draw_string(61, 19, 0x70, " Shift \x18\x19\x1A\x1B ");
        video->draw_string(72, 19, 0x1F, " Shoot");
        video->draw_string(62, 21, 0x70, " S ");
        video->draw_string(65, 21, 0x1F, " Save game");
        video->draw_string(62, 22, 0x30, " P ");
        video->draw_string(65, 22, 0x1F, " Pause");
        video->draw_string(62, 23, 0x70, " Q ");
        video->draw_string(65, 23, 0x1F, " Quit");
    } else if (gameStateElement == EMonitor) {
        SidebarPromptSlider(false, 66, 21, "Game speed:;FS", tickSpeed);
        video->draw_string(62, 21, 0x70, " S ");
        video->draw_string(62, 7, 0x30, " W ");
        video->draw_string(65, 7, 0x1E, " World:");
        video->draw_string(69, 8, 0x1F, StrLength(world.info.name) != 0 ? world.info.name : "Untitled");
        video->draw_string(62, 11, 0x70, " P ");
        video->draw_string(65, 11, 0x1F, " Play");
        video->draw_string(62, 12, 0x30, " R ");
        video->draw_string(65, 12, 0x1E, " Restore game");
        video->draw_string(62, 13, 0x70, " Q ");
        video->draw_string(65, 13, 0x1E, " Quit");
        video->draw_string(62, 16, 0x30, " A ");
        video->draw_string(65, 16, 0x1F, " About ZZT!");
        video->draw_string(62, 17, 0x70, " H ");
        video->draw_string(65, 17, 0x1E, " High Scores");

        if (editorEnabled) {
            video->draw_string(62, 18, 0x30, " E ");
            video->draw_string(65, 18, 0x1E, " Board Editor");
        }
    }
}

void Game::GameUpdateSidebar(void) {
    sstring<20> s;

    if (gameStateElement == EPlayer) {
        if (board.info.time_limit_seconds > 0) {
            video->draw_string(64, 6, 0x1E, "   Time:");
            StrFormat(s, "%d ", board.info.time_limit_seconds - world.info.board_time_sec);
            video->draw_string(72, 6, 0x1E, s);
        } else {
            SidebarClearLine(6);
        }

        if (world.info.health < 0) {
            world.info.health = 0;
        }

        StrFormat(s, "%d ", world.info.health);
        video->draw_string(72, 7, 0x1E, s);
        StrFormat(s, "%d  ", world.info.ammo);
        video->draw_string(72, 8, 0x1E, s);
        StrFormat(s, "%d ", world.info.torches);
        video->draw_string(72, 9, 0x1E, s);
        StrFormat(s, "%d ", world.info.gems);
        video->draw_string(72, 10, 0x1E, s);
        StrFormat(s, "%d ", world.info.score);
        video->draw_string(72, 11, 0x1E, s);

        if (world.info.torch_ticks == 0) {
            video->draw_string(75, 9, 0x16, "    ");
        } else {
            int iLimit = ((world.info.torch_ticks * 5) / TORCH_DURATION);
            for (int i = 2; i <= 5; i++) {
                video->draw_char(73 + i, 9, 0x16, (i <= iLimit) ? 177 : 176);
            }
        }

        for (int i = 0; i < 7; i++) {
            if (world.info.keys[i]) {
                video->draw_char(72 + i, 12, 0x19 + i, elementDefs[EKey].character);
            } else {
                video->draw_char(72 + i, 12, 0x1F, ' ');
            }
        }

        video->draw_string(65, 15, 0x1F, sound->sound_is_enabled() ? " Be quiet" : " Be noisy");

        if (debugEnabled) {
            // TODO: Add something interesting.
        }
    }
}

void Game::DisplayMessage(int16_t time, const char *message) {
    int16_t timer_id = board.stats.id_at(0, 0);
    if (timer_id != -1) {
        RemoveStat(timer_id);
        BoardDrawBorder();
    }

    if (strlen(message) != 0) {
        AddStat(0, 0, EMessageTimer, 0, 1, Stat());
        board.stats[board.stats.count].p2 = time / (tickTimeDuration + 1);
        StrCopy(board.info.message, message);
    }
}

void Game::DamageStat(int16_t attacker_stat_id) {
    Stat& attacker_stat = board.stats[0];
    if (attacker_stat_id == 0) {
        if (world.info.health > 0) {
            world.info.health -= 10;

            GameUpdateSidebar();
            DisplayMessage(100, "Ouch!");

            board.tiles.set_color(attacker_stat.x, attacker_stat.y, 
                0x70 + (elementDefs[EPlayer].color & 0x0F));

            if (world.info.health > 0) {
                world.info.board_time_sec = 0;
                if (board.info.reenter_when_zapped) {
					sound->sound_queue(4, "\x20\x01\x23\x01\x27\x01\x30\x01\x10\x01");

                    // move player to start
                    board.tiles.set_element(attacker_stat.x, attacker_stat.y, EEmpty);
                    BoardDrawTile(attacker_stat.x, attacker_stat.y);
                    int old_x = attacker_stat.x;
                    int old_y = attacker_stat.y;
                    attacker_stat.x = board.info.start_player_x;
                    attacker_stat.y = board.info.start_player_y;
                    DrawPlayerSurroundings(old_x, old_y, 0);
                    DrawPlayerSurroundings(attacker_stat.x, attacker_stat.y, 0);

                    gamePaused = true;
                }

				sound->sound_queue(4, "\x10\x01\x20\x01\x13\x01\x23\x01");
            } else {
				sound->sound_queue(5,
					"\x20\x03\x23\x03\x27\x03\x30\x03"
					"\x27\x03\x2A\x03\x32\x03\x37\x03"
					"\x35\x03\x38\x03\x40\x03\x45\x03\x10\x0A"
				);
            }
        }
    } else {
        switch (board.tiles.get(attacker_stat.x, attacker_stat.y).element) {
            case EBullet:
                sound->sound_queue(3, "\x20\x01");
                break;
            case EObject:
                // pass
                break;
			default:
                sound->sound_queue(3, "\x40\x01\x10\x01\x50\x01\x30\x01");
                break;
        }
        RemoveStat(attacker_stat_id);
    }
}

void Game::BoardDamageTile(int16_t x, int16_t y) {
    int16_t stat_id = board.stats.id_at(x, y);
    if (stat_id != -1) {
        DamageStat(stat_id);
    } else {
        board.tiles.set_element(x, y, EEmpty);
        BoardDrawTile(x, y);
    }
}

void Game::BoardAttack(int16_t attacker_stat_id, int16_t x, int16_t y) {
    if (attacker_stat_id == 0 && world.info.energizer_ticks > 0) {
        world.info.score += elementDefAt(x, y).score_value;
        GameUpdateSidebar();
    } else {
        DamageStat(attacker_stat_id);
    }

    if (attacker_stat_id > 0 && attacker_stat_id <= currentStatTicked) {
        currentStatTicked--;
    }

    if (board.tiles.get(x, y).element == EPlayer && world.info.energizer_ticks > 0) {
        Stat& attacker_stat = board.stats[attacker_stat_id];
        world.info.score += elementDefAt(attacker_stat.x, attacker_stat.y).score_value;
        GameUpdateSidebar();
    } else {
        BoardDamageTile(x, y);
		sound->sound_queue(2, "\x10\x01");
    }
}

bool Game::BoardShoot(uint8_t element, int16_t x, int16_t y, int16_t dx, int16_t dy, int16_t source) {
    Tile destTile = board.tiles.get(x + dx, y + dy);
    if (elementDefs[destTile.element].walkable || destTile.element == EWater) {
        AddStat(x + dx, y + dy, element, elementDefs[element].color, 1, Stat());
        Stat& shotStat = board.stats[board.stats.count];
        shotStat.p1 = source;
        shotStat.step_x = dx;
        shotStat.step_y = dy;
        shotStat.p2 = 100;
        return true;
    } else if (destTile.element == EBreakable
        || (
            elementDefs[destTile.element].destructible
            && (destTile.element == EPlayer)
            && (world.info.energizer_ticks <= 0)
        ))
    {
        BoardDamageTile(x + dx, y + dy);
		sound->sound_queue(2, "\x10\x01");
        return true;
    } else {
        return false;
    }
}

void Game::CalcDirectionRnd(int16_t &deltaX, int16_t &deltaY) {
    deltaX = random.Next(3) - 1;
    deltaY = (deltaX == 0) ? (random.Next(2) * 2 - 1) : 0;
}

void Game::CalcDirectionSeek(int16_t x, int16_t y, int16_t &deltaX, int16_t &deltaY) {
    deltaX = 0;
    deltaY = 0;
    
    if ((random.Next(2) < 1) || (board.stats[0].y == y)) {
        deltaX = Signum(board.stats[0].x - x);
    }

    if (deltaX == 0) {
        deltaY = Signum(board.stats[0].y - y);
    }

    if (world.info.energizer_ticks > 0) {
        deltaX = -deltaX;
        deltaY = -deltaY;
    }
}

void Game::TransitionDrawBoardChange(void) {
    TransitionDrawToFill(219, 0x05);
    TransitionDrawToBoard();
}

void Game::BoardEnter(void) {
    board.info.start_player_x = board.stats[0].x;
    board.info.start_player_y = board.stats[0].y;

    if (board.info.is_dark && msgFlags.HintTorchNotShown) {
        DisplayMessage(200, "Room is dark - you need to light a torch!");
        msgFlags.HintTorchNotShown = false;
    }

    world.info.board_time_sec = 0;
    GameUpdateSidebar();
}

void Game::BoardPassageTeleport(int16_t x, int16_t y) {
    Stat& stat = board.stats.at(x, y);
    Stat& player = board.stats[0];
    uint8_t col = board.tiles.get(x, y).color;

    BoardChange(stat.p3);

    int16_t newX = 0;
    int16_t newY = 0;
    for (int16_t ix = 1; ix <= board.width(); ix++) {
        for (int16_t iy = 1; iy <= board.height(); iy++) {
            Tile tile = board.tiles.get(ix, iy);
            if (tile.element == EPassage && tile.color == col) {
                newX = ix;
                newY = iy;
            }
        }
    }

    board.tiles.set(player.x, player.y, {
        .element = EEmpty,
        .color = 0x00
    });
    if (newX != 0) {
        player.x = newX;
        player.y = newY;
    }

    gamePaused = true;
	sound->sound_queue(4,
		"\x30\x01\x34\x01\x37\x01"
		"\x31\x01\x35\x01\x38\x01"
		"\x32\x01\x36\x01\x39\x01"
		"\x33\x01\x37\x01\x3A\x01"
		"\x34\x01\x38\x01\x40\x01");
    TransitionDrawBoardChange();
    BoardEnter();
}

void Game::GameDebugPrompt(void) {
    sstring<50> input;
    input[0] = 0;
    SidebarClearLine(4);
    SidebarClearLine(5);

    PromptString(63, 5, 0x1E, 0x0F, 11, PMAny, input, StrSize(input));
    for (int i = 0; i < strlen(input); i++) {
        input[i] = UpCase(input[i]);
    }

    bool toggle = true;
    if (input[0] == '+' || input[0] == '-') {
        if (input[0] == '-') toggle = false;
        
        sstring<50> tmp;
        StrCopy(tmp, input + 1);
        StrCopy(input, tmp);

        if (toggle) WorldSetFlag(input); else WorldClearFlag(input);
    }

    debugEnabled = WorldGetFlagPosition("DEBUG") >= 0;

    if (StrEquals(input, "HEALTH")) {
        world.info.health += 50;
    } else if (StrEquals(input, "AMMO")) {
        world.info.ammo += 5;
    } else if (StrEquals(input, "KEYS")) {
        for (int i = 0; i < 7; i++) {
            world.info.keys[i] = true;
        }
    } else if (StrEquals(input, "TORCHES")) {
        world.info.torches += 3;
    } else if (StrEquals(input, "GEMS")) {
        world.info.gems += 5;
    } else if (StrEquals(input, "DARK")) {
        board.info.is_dark = toggle;
        TransitionDrawToBoard();
    } else if (StrEquals(input, "ZAP")) {
        for (int i = 0; i < 4; i++) {
            int16_t dest_x = board.stats[0].x + NeighborDeltaX[i];
            int16_t dest_y = board.stats[0].y + NeighborDeltaY[i];
            BoardDamageTile(dest_x, dest_y);
            board.tiles.set_element(dest_x, dest_y, EEmpty);
            BoardDrawTile(dest_x, dest_y);
        }
    }

	sound->sound_queue(10, "\x27\x04");
    SidebarClearLine(4);
    SidebarClearLine(5);
    GameUpdateSidebar();
}

void Game::GameAboutScreen(void) {
    TextWindowDisplayFile(video, input, sound, "ABOUT.HLP", "About ZZT...");
}

void Game::GamePlayLoop(bool boardChanged) {
    GameDrawSidebar();
    GameUpdateSidebar();

    if (justStarted) {
        GameAboutScreen();
        if (StrLength(startupWorldFileName) != 0) {
            SidebarClearLine(8);
            video->draw_string(69, 8, 0x1F, startupWorldFileName);
            if (!WorldLoad(startupWorldFileName, ".ZZT", true)) {
                WorldCreate();
            }
        }
        returnBoardId = world.info.current_board;
        BoardChange(0);
        justStarted = false;
    }

    Stat &player = board.stats[0];

    board.tiles.set(player.x, player.y, {
        .element = gameStateElement,
        .color = elementDefs[gameStateElement].color
    });

    if (gameStateElement == EMonitor) {
        DisplayMessage(0, "");
        video->draw_string(62, 5, 0x1B, "Pick a command:");
    }

    if (boardChanged) {
        TransitionDrawBoardChange();
    }

    tickTimeDuration = tickSpeed * 2;
    gamePlayExitRequested = false;
    bool exitLoop = false;
    bool pauseBlink = false;

    currentTick = random.Next(100);
    currentStatTicked = board.stats.count + 1;

    do {
        if (gamePaused) {
            if (HasTimeElapsed(tickTimeCounter, 25)) {
                pauseBlink = !pauseBlink;
            }

            if (pauseBlink) {
                video->draw_char(player.x - 1, player.y - 1, elementDefs[EPlayer].color, elementDefs[EPlayer].character);
            } else {
                if (board.tiles.get(player.x, player.y).element == EPlayer) {
                    video->draw_char(player.x - 1, player.y - 1, 0x0F, ' ');
                } else {
                    BoardDrawTile(player.x, player.y);
                }
            }

            video->draw_string(64, 5, 0x1F, "Pausing...");
            sound->idle(IMUntilFrame);
            input->update_input();

            if (input->keyPressed == KeyEscape) {
                GamePromptEndPlay();
            }

            if (input->deltaX != 0 || input->deltaY != 0) {
                int16_t dest_x = player.x + input->deltaX;
                int16_t dest_y = player.y + input->deltaY;
                elementDefAt(dest_x, dest_y).touch(*this, dest_x, dest_y, 0, input->deltaX, input->deltaY);
            }

            if (input->deltaX != 0 || input->deltaY != 0) {
                int16_t dest_x = player.x + input->deltaX;
                int16_t dest_y = player.y + input->deltaY;
                const Tile &dest_tile = board.tiles.get(dest_x, dest_y);
                if (elementDefs[dest_tile.element].walkable) {
                    const Tile &src_tile = board.tiles.get(player.x, player.y);

                    // Move player
                    if (src_tile.element == EPlayer) {
                        MoveStat(0, dest_x, dest_y);
                    } else {
                        BoardDrawTile(player.x, player.y);
                        player.x = dest_x;
                        player.y = dest_y;
                        board.tiles.set(player.x, player.y, {
                            .element = EPlayer,
                            .color = elementDefs[EPlayer].color
                        });
                        BoardDrawTile(player.x, player.y);
                        DrawPlayerSurroundings(player.x, player.y, 0);
                        DrawPlayerSurroundings(player.x - input->deltaX, player.y - input->deltaY, 0);
                    }

                    // Unpause
                    gamePaused = false;
                    SidebarClearLine(5);
                    currentTick = random.Next(100);
                    currentStatTicked = board.stats.count + 1;
                    world.info.is_save = true;
                }
            }
        } else /* not gamePaused */ {
            if (currentStatTicked <= board.stats.count) {
                Stat &stat = board.stats[currentStatTicked];
                if (stat.cycle != 0 && ((currentTick % stat.cycle) == (currentStatTicked % stat.cycle))) {
                    elementDefAt(stat.x, stat.y).tick(*this, currentStatTicked);
                }

                currentStatTicked++;
            }
        }

        if (currentStatTicked > board.stats.count && !gamePlayExitRequested) {
            // all stats ticked
            if (HasTimeElapsed(tickTimeCounter, tickTimeDuration)) {
                currentTick++;
                if (currentTick > 420) {
                    currentTick = 1;
                }
                currentStatTicked = 0;

                input->update_input();
            } else {
                sound->idle(IMUntilPit);
            }
        }
    } while (!((exitLoop || gamePlayExitRequested) && gamePlayExitRequested));

    sound->sound_clear_queue();

    if (gameStateElement == EPlayer) {
        if (world.info.health <= 0) {
            highScoreList.Add(world.info.name, world.info.score);
        }
    } else if (gameStateElement == EMonitor) {
        SidebarClearLine(5);
    }

    board.tiles.set(player.x, player.y, {
        .element = EPlayer,
        .color = elementDefs[EPlayer].color
    });

    sound->sound_set_block_queueing(false);
}

void Game::GameTitleLoop(void) {
    gameTitleExitRequested = false;
    justStarted = true;
    returnBoardId = 0;
    bool boardChanged = true;
    do {
        BoardChange(0);
        do {
            gameStateElement = EMonitor;
            bool startPlay = false;
            gamePaused = false;
            GamePlayLoop(boardChanged);
            boardChanged = false;

            switch (UpCase(input->keyPressed)) {
                case 'W': {
                    if (GameWorldLoad(".ZZT")) {
                        returnBoardId = world.info.current_board;
                        boardChanged = true;
                    }
                } break;
                case 'P': {
                    if (world.info.is_save && !debugEnabled) {
                        startPlay = WorldLoad(world.info.name, ".ZZT", false);
                        returnBoardId = world.info.current_board;
                    } else {
                        startPlay = true;
                    }
                    if (startPlay) {
                        BoardChange(returnBoardId);
                        BoardEnter();
                    }
                } break;
                case 'A': {
                    GameAboutScreen();
                } break;
                case 'E': {
                    if (editorEnabled) {
                        Editor *editor = new Editor(this);
                        editor->Loop();
                        delete editor;
                        
                        returnBoardId = world.info.current_board;
                        boardChanged = true;
                    }
                } break;
                case 'S': {
                    SidebarPromptSlider(true, 66, 21, "Game speed:;FS", tickSpeed);
                    input->keyPressed = 0;
                } break;
                case 'R': {
                    if (GameWorldLoad(".SAV")) {
                        returnBoardId = world.info.current_board;
                        BoardChange(returnBoardId);
                        startPlay = true;
                    }
                } break;
                case 'H': {
                    highScoreList.Load(world.info.name);
                    highScoreList.Display(world.info.name, 0);
                } break;
                case '|': {
                    GameDebugPrompt();
                } break;
                case KeyEscape: case 'Q': {
                    gameTitleExitRequested = SidebarPromptYesNo("Quit ZZT? ", true);
                } break;
            }

            if (startPlay) {
                gameStateElement = EPlayer;
                gamePaused = true;
                GamePlayLoop(true);
                boardChanged = true;
            }
        } while (!boardChanged && !gameTitleExitRequested);
    } while (!gameTitleExitRequested);
}
