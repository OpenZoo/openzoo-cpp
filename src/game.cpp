#include <cstdlib>
#include "editor.h"
#include "file_selector.h"
#include "gamevars.h"
#include "txtwind.h"

using namespace ZZT;
using namespace ZZT::Utils;

static const uint8_t ProgressAnimColors[8] = {0x04, 0x0C, 0x05, 0x0D, 0x06, 0x0E, 0x07, 0x0F};
static const char *ProgressAnimStrings[8] = {
    " Loading....|", " Loading...*/", " Loading..*.-", " Loading.*..\\",
    " Loading*...|", " Loading..../", " Loading....-", " Loading....\\"
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

Game::Game(void): highScoreList(this) {
    boardSerializer = new SerializerFormatZZT();
    worldSerializer = new SerializerFormatZZT();

    tickSpeed = 4;
    debugEnabled = false;
#ifndef DISABLE_EDITOR
    editorEnabled = true;
#endif
    StrCopy(startupWorldFileName, "TOWN");
    StrCopy(savedGameFileName, "SAVED");
    StrCopy(savedBoardFileName, "TEMP");
    memset(world.board_len, 0, sizeof(world.board_len));
    GenerateTransitionTable();
    WorldCreate();
}

Game::~Game() {
    free(boardSerializer);
    free(worldSerializer);
}

void Game::GenerateTransitionTable(void) {
    transitionTableSize = 0;
    for (uint8_t iy = 1; iy <= board.height(); iy++) {
        for (uint8_t ix = 1; ix <= board.width(); ix++) {
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

void Game::SidebarClearLine(int y) {
    video->draw_string(60, y, 0x11, "                    ");
}

void Game::SidebarClear(void) {
    for (int i = 3; i <= 24; i++) {
        SidebarClearLine(i);
    }
}

void Game::BoardClose() {
    size_t buflen = boardSerializer->estimate_buffer_size(board);
    uint8_t *buffer = (uint8_t*) malloc(buflen);
    MemoryIOStream stream(buffer, buflen, true);

    boardSerializer->serialize(board, stream);

    for (int i = 0; i <= board.stats.count; i++) {
        board.stats.free_data_if_unused(i);
    }

    if (world.board_len[world.info.current_board] > 0) {
        free(world.board_data[world.info.current_board]);
    }
    world.board_len[world.info.current_board] = stream.tell();
    world.board_data[world.info.current_board] = (uint8_t*) realloc(buffer, stream.tell());
    if (world.board_data[world.info.current_board] == nullptr) {
        world.board_data[world.info.current_board] = buffer;
    }
}

void Game::BoardOpen(int16_t board_id) {
    if (board_id > world.board_count) {
        board_id = world.info.current_board;
    }

    MemoryIOStream stream(world.board_data[board_id], world.board_len[board_id], false);

    boardSerializer->deserialize(board, stream);

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
    StrClear(board.name);
    StrClear(board.info.message);
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
        StrClear(world.info.flags[i]);
    }
    BoardChange(0);
    StrCopy(board.name, "Title screen");
    StrClear(loadedGameFileName);
    StrClear(world.info.name);
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
            uint8_t ch = elementCharOverrides[tile.element];
            if (ch == 0) ch = elementDefs[tile.element].character;
            video->draw_char(x - 1, y - 1, tile.color, ch);
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

void Game::PauseOnError(void) {
    uint8_t error_notes[32];
    sound->sound_queue(1, error_notes, SoundParse("s004x114x9", error_notes, 32));
    sound->delay(2000);
}

void Game::DisplayIOError(IOStream &stream) {
    if (!stream.errored()) return;

    TextWindow window = TextWindow(video, input, sound, filesystem);
    StrCopy(window.title, "Error");
    window.Append("$I/O Error: ");
    window.Append("");
    window.Append("This may be caused by missing");
    window.Append("ZZT files or a bad disk.  If");
    window.Append("you are trying to save a game,");
    window.Append("your disk may be full -- try");
    window.Append("using a blank, formatted disk");
    window.Append("for saving the game!");

    window.DrawOpen();
    window.Select(false, false);
    window.DrawClose();
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
    interface->SidebarShowMessage(0x0F, " Loading.....");

    char joinedName[256];
    StrJoin(joinedName, 2, filename, extension);
    IOStream *stream = filesystem->open_file(joinedName, false);

    if (!stream->errored()) {
        WorldUnload();
        
        bool result = worldSerializer->deserialize(world, *stream, titleOnly, [this](auto bid) {
            interface->SidebarShowMessage(ProgressAnimColors[bid & 7], ProgressAnimStrings[bid & 7]);
        });

        if (result) {
            BoardOpen(world.info.current_board);
            StrCopy(loadedGameFileName, filename);
            highScoreList.Load(world.info.name);
            interface->SidebarHideMessage();
            delete stream;
            return true;
        }
    }

    if (stream->errored()) {
        DisplayIOError(*stream);
    }

    delete stream;
    return false;
}

bool Game::WorldSave(const char *filename, const char *extension) {
    BoardClose();
    interface->SidebarShowMessage(0x0F, " Saving...");

    char joinedName[256];
    StrJoin(joinedName, 2, filename, extension);
    IOStream *stream = filesystem->open_file(joinedName, true);

    if (!stream->errored()) {
        bool result = worldSerializer->serialize(world, *stream, [](auto i){});

        if (result) {
            BoardOpen(world.info.current_board);
            interface->SidebarHideMessage();
            delete stream;
            return true;
        }
    }

    if (stream->errored()) {
        delete stream;
        stream = filesystem->open_file(joinedName, true);
    }

    BoardOpen(world.info.current_board);
    interface->SidebarHideMessage();
    delete stream;
    return false;
}

void Game::GameWorldSave(const char *prompt, char* filename, size_t filename_len, const char *extension) {
    sstring<50> newFilename;
    StrCopy(newFilename, filename);
    interface->SidebarPromptString(prompt, extension, newFilename, sizeof(newFilename), PMAlphanum);
    if (input->keyPressed != KeyEscape && !StrEmpty(newFilename)) {
        strncpy(filename, newFilename, filename_len - 1);
        filename[filename_len - 1] = 0;

        if (StrEquals(extension, ".ZZT")) {
            StrCopy(world.info.name, filename);
        }

        WorldSave(filename, extension);
    }
}

bool Game::GameWorldLoad(const char *extension) {
    const char *title = StrEquals(extension, ".ZZT") ? "ZZT Worlds" : "Saved Games";
    FileSelector *selector = new FileSelector(video, input, sound, filesystem, title, extension);

    if (selector->select()) {
        bool result = WorldLoad(selector->get_filename(), extension, false);
        if (result) {
            TransitionDrawToFill(219, 0x44);
            delete selector;
            return true;
        } else {
            // FIXME: at this point, the world has a player on it - should have a monitor
            TransitionDrawBoardChange();
        }
    }
    
    delete selector;
    return false;
}

void ZZT::CopyStatDataToTextWindow(const Stat &stat, TextWindow &window) {
    DynString s = "";
    int data_str_pos = 0;
    
    window.Clear();

    for (int i = 0; i < stat.data.len; i++) {
        char ch = stat.data.data[i];
        if (ch == '\r') {
            window.Append(s);
            s = "";
        } else {
            s = s + ch;
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
        stat.data.duplicate();
        stat.data_pos = 0;

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
    board.stats.free_data_if_unused(stat_id);

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

void Game::GameDrawSidebar(void) {
    interface->SidebarGameDraw(*this, SIDEBAR_REDRAW);
}

static const char * menu_str_sound(Game *game) {
    return game->sound->sound_is_enabled() ? "Be quiet" : "Be noisy";
}

static const char * menu_str_editor(Game *game) {
    return game->editorEnabled ? "Editor" : nullptr;
}

void Game::GameUpdateSidebar(void) {
    interface->SidebarGameDraw(*this, SIDEBAR_FLAG_UPDATE);
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
    Stat& attacker_stat = board.stats[attacker_stat_id];
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
    StrClear(input);

    interface->SidebarPromptString(nullptr, nullptr, input, StrSize(input), PMAny);
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
    GameUpdateSidebar();
}

void Game::GameAboutScreen(void) {
    TextWindowDisplayFile(video, input, sound, filesystem, "ABOUT.HLP", "About ZZT...");
}

void Game::GamePlayLoop(bool boardChanged) {
    GameDrawSidebar();

    if (justStarted) {
        GameAboutScreen();
        if (StrLength(startupWorldFileName) != 0) {
            interface->SidebarGameDraw(*this, SIDEBAR_FLAG_SET_WORLD_NAME);
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
        interface->SidebarShowMessage(0x0B, "Pick a command:");
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

            interface->SidebarShowMessage(0x0F, "  Pausing...");
            sound->idle(IMUntilFrame);
            input->update_input();

            // OpenZoo: Add "Q" and START to the list of allowed keys for exiting.
            if (input->keyPressed == KeyEscape || UpCase(input->keyPressed) == 'Q' || input->joy_button_pressed(JoyButtonStart, false)) {
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
                    interface->SidebarHideMessage();
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
        interface->SidebarHideMessage();
    }

    board.tiles.set(player.x, player.y, {
        .element = EPlayer,
        .color = elementDefs[EPlayer].color
    });

    sound->sound_set_block_queueing(false);
}

const MenuEntry ZZT::TitleMenu[] = {
    {.id = 'W', .keys = {'W'}, .name = "Load world"},
    {.id = 'P', .keys = {'P'}, .name = "Play"},
    {.id = 'R', .keys = {'R'}, .name = "Restore game"},
    {.id = 'A', .keys = {'A'}, .name = "About ZZT"},
    {.id = 'E', .keys = {'E'}, .name_func = menu_str_editor},
    {.id = 'S', .keys = {'S'}, .name = "Game speed"},
    {.id = 'H', .keys = {'H'}, .name = "Help"},
    {.id = '|', .keys = {'|'}, .name = "Console command"},
    {.id = 'Q', .keys = {'Q', KeyEscape}, .name = "Quit ZZT"},
    {.id = -1}
};

const MenuEntry ZZT::PlayMenu[] = {
    {.id = 'T', .keys = {'T'}, .joy_button = JoyButtonB}, // Torch
    {.id = 'S', .keys = {'S'}, .name = "Save game"},
    {.id = 'P', .keys = {'P'}}, // Pause - hidden in menu mode
    {.id = 'B', .keys = {'B'}, .name_func = menu_str_sound},
    {.id = 'H', .keys = {'H'}, .name = "Help"},
    {.id = '?', .keys = {'?'}, .name = "Console command"},
    {.id = 'Q', .keys = {'Q', KeyEscape}, .name = "Quit game"},
    {.id = -1}
};

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

            switch (interface->HandleMenu(*this, TitleMenu, false)) {
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
                case 'Q': {
                    gameTitleExitRequested = interface->SidebarPromptYesNo("Quit ZZT? ", true);
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