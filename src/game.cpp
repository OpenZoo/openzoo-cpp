#include <cstdlib>
#include "editor.h"
#include "file_selector.h"
#include "gamevars.h"
#include "platform_hacks.h"
#include "txtwind.h"

using namespace ZZT;

static const uint8_t ProgressAnimColors[8] = {0x07, 0x0C, 0x09, 0x0D, 0x0A, 0x0E, 0x0B, 0x0F};
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

const uint8_t ZZT::WebChars[16] = {
    250, 179, 179, 179, 196, 217, 191, 180, 196, 192, 218, 195, 196, 193, 194, 197
};

#if defined(__GBA__) || defined(__NDS__)
extern uint8_t ext_tile_memory[];
extern uint8_t ext_stat_memory[];
#endif

// LFSR11 for transition table

static uint16_t transition_table_start = 42;

bool transition_table_next(uint16_t &seed, uint8_t &tx, uint8_t &ty) {
	seed = (seed >> 1) ^ ((-(seed & 1)) & 0x740);
	tx = (seed - 1) & 0x3F;
	ty = ((seed - 1) >> 6) & 0x3F;
	return (seed == transition_table_start);
}

// TileMap

TileMap::TileMap(uint8_t _width, uint8_t _height)
    : width(_width), height(_height) {
#if defined(__GBA__) || defined(__NDS__)
    if (width <= 60 && height <= 25) {
        this->tiles = (Tile*) ext_tile_memory;
    } else
#endif
    this->tiles = (Tile*) malloc((width + 2) * (height + 2) * sizeof(Tile));
    memset(this->tiles, 0, (width + 2) * (height + 2) * sizeof(Tile));
}

TileMap::TileMap(const TileMap&& other)
    : width(other.width), height(other.height) {
    tiles = other.tiles;
}

TileMap::~TileMap() {
#if defined(__GBA__) || defined(__NDS__)
    if (width <= 60 && height <= 25) {} else
#endif
    free(this->tiles);
}


// StatList

StatList::StatList(int16_t _size)
    : size(_size) {
#if defined(__GBA__) || defined(__NDS__)
    if (size <= 150) {
        this->stats = (Stat*) ext_stat_memory;
    } else
#endif
    this->stats = (Stat*) malloc((size + 3) * sizeof(Stat));
	this->count = 0;

    // set stat -1 to out of bounds values
    this->stats[0] = {
        .data = StatData(),
        .x = 0,
        .y = 1,
        .step_x = 256,
        .step_y = 256,
        .cycle = 256,
        .p1 = 0,
        .p2 = 1,
        .p3 = 0,
        .follower = 1,
        .leader = 1,
        .under = {
            .element = 1,
            .color = 0x00
        },
        .data_pos = 1
    };
	this->stats[0].data.len = 1;
	this->stats[1] = {
		.data = StatData(),
		.follower = -1,
		.leader = -1
	};
	this->stats[1].data.len = 0;
    for (int i = 2; i < (size + 3); i++) {
		this->stats[i] = this->stats[1];
    }
}

StatList::~StatList() {
    free_all_data();
#if defined(__GBA__) || defined(__NDS__)
    if (size <= 150) {} else
#endif
    free(this->stats);
}

// Board

Board::Board(uint8_t width, uint8_t height, int16_t stat_size)
    : tiles(TileMap(width, height)), stats(StatList(stat_size)) {

}

Board::~Board() {

}

// World

#define SERIALIZERS_COUNT 2
static Serializer *serializers[SERIALIZERS_COUNT] = {
    new SerializerFormatZZT(WorldFormatZZT),
    new SerializerFormatZZT(WorldFormatSuperZZT)
};
static EngineType engine_types[SERIALIZERS_COUNT] = {
    ENGINE_TYPE_ZZT,
    ENGINE_TYPE_SUPER_ZZT
};

static Serializer *get_serializer(WorldFormat format) {
    for (int i = 0; i < SERIALIZERS_COUNT; i++) {
        if (serializers[i]->get_format() == format) {
            return serializers[i];
        }
    }
    return nullptr;
}

static void convert_board(EngineDefinition &def, const uint8_t *data, size_t data_len, uint8_t from, uint8_t to, bool destroyDataUponCompletion, uint8_t *&out_data, size_t &out_data_len) {
    Board *board = new Board(def.boardWidth, def.boardHeight, def.statCount);
    Serializer *fromS = get_serializer((WorldFormat) (from & 0x7F));
    Serializer *toS = get_serializer((WorldFormat) (to & 0x7F));

    MemoryIOStream inputStream(data, data_len);
    fromS->deserialize_board(*board, inputStream, (from & 0x80) != 0);

    if (destroyDataUponCompletion) {
        free((void*) data);
    }

    size_t buflen = toS->estimate_board_size(*board);
    uint8_t *buffer = (uint8_t*) malloc(buflen);
    MemoryIOStream stream(buffer, buflen, true);

    toS->serialize_board(*board, stream, (to & 0x80) != 0);
    delete board;

    uint8_t *buffer_ra = (uint8_t*) realloc(buffer, stream.tell());
    out_data = buffer_ra != nullptr ? buffer_ra : buffer;
    out_data_len = stream.tell();
}

World::World(WorldFormat _format, EngineDefinition *engine_def, int16_t _max_board, bool compress_eagerly) {
    this->format = _format;
    this->engine = engine_def;
    this->max_board = _max_board;
    this->compress_eagerly = compress_eagerly;

    this->board_data = (uint8_t**) malloc(sizeof(uint8_t*) * (_max_board + 1));
    this->board_format = (uint8_t*) malloc(sizeof(uint8_t) * (_max_board + 1));
    this->board_len = (uint16_t*) malloc(sizeof(uint16_t) * (_max_board + 1));
    memset(board_len, 0, sizeof(uint16_t) * (_max_board + 1));
}

World::~World() {
    for (int i = 0; i <= board_count; i++) {
        free_board(i);
    }

    free(this->board_len);
    free(this->board_format);
    free(this->board_data);
}

void World::set_format(WorldFormat format) {
    this->format = format;
}

bool World::read_board(uint8_t id, Board &board) {
    uint8_t format = this->board_format[id];
    Serializer *fromS = get_serializer((WorldFormat) (format & 0x7F));
    MemoryIOStream inputStream(this->board_data[id], this->board_len[id]);
    return fromS->deserialize_board(board, inputStream, (format & 0x80) != 0);
}

bool World::write_board(uint8_t id, Board &board) {
    Serializer *toS = get_serializer(format);
    size_t buflen = toS->estimate_board_size(board);
    uint8_t *buffer = (uint8_t*) malloc(buflen);
    MemoryIOStream stream(buffer, buflen, true);
    if (!toS->serialize_board(board, stream, true)) {
        free(buffer);
        return false;
    }

    free_board(id);

    board_len[id] = stream.tell();
    board_data[id] = (uint8_t*) realloc(buffer, stream.tell());
    if (board_data[id] == nullptr) {
        board_data[id] = buffer;
    }
    board_format[id] = (uint8_t) format | 0x80;

    return true;
}

void World::get_board(uint8_t id, uint8_t *&data, uint16_t &len, bool &temporary, WorldFormat format) {
    if (this->board_format[id] == format) {
        data = this->board_data[id];
        len = this->board_len[id];
        temporary = false;
    } else {
        uint8_t *out_data;
        size_t out_len;

        convert_board(*engine,
            this->board_data[id], this->board_len[id],
            this->board_format[id], format,
            false, out_data, out_len);

        data = out_data;
        len = out_len;
        temporary = true;
    }
}

void World::set_board(uint8_t id, uint8_t *data, uint16_t len, bool costlessly_loaded, WorldFormat format) {
    free_board(id);

    if (costlessly_loaded) {
        this->board_data[id] = data;
        this->board_len[id] = len;
        this->board_format[id] = format;
    } else if (!compress_eagerly) {
        this->board_data[id] = (uint8_t *) malloc(len);
        memcpy(this->board_data[id], data, len);
        this->board_len[id] = len;
        this->board_format[id] = format;
    } else {
        uint8_t *out_data;
        size_t out_len;

        convert_board(*engine,
            data, len,
            format, (uint8_t) format | 0x80,
            false, out_data, out_len);

        this->board_data[id] = out_data;
        this->board_len[id] = out_len;
        this->board_format[id] = (uint8_t) format | 0x80;
    }
}

void World::free_board(uint8_t bid) {
    if (this->board_len[bid] > 0) {
        this->board_len[bid] = 0;
#ifdef ROM_POINTERS
        // check writeability
        uint8_t val = this->board_data[bid][0] ^ 0xFF;
        this->board_data[bid][0] = val;
        if (this->board_data[bid][0] != val) {
            return;
        }
#endif
        free(this->board_data[bid]);
    }
}

// Viewport

Viewport::Viewport(int16_t _x, int16_t _y, int16_t _width, int16_t _height)
	: x(_x), y(_y), width(_width), height(_height) {
	cx_offset = 0;
	cy_offset = 0;
}



bool Viewport::set(int16_t nx, int16_t ny) {
    if (cx_offset != nx || cy_offset != ny) {
        cx_offset = nx;
        cy_offset = ny;
        return true;
    } else {
        return false;
    }
}

bool Viewport::point_at(Board &board, int16_t sx, int16_t sy) {
    int16_t nx = (sx - 1) - (width >> 1);
    int16_t ny = (sy - 1) - (height >> 1);

    if (nx < 0) nx = 0;
    else if (nx > (board.width() - width)) nx = board.width() - width;
    if (ny < 0) ny = 0;
    else if (ny > (board.height() - height)) ny = board.height() - height;

    return set(nx, ny);
}

// Game

Game::Game(void):
    board(60, 25, 150),
    viewport(0, 0, 60, 25),
    world(WorldFormatZZT, &engineDefinition, 255, true), /* TODO: dynamic board list scaling? */
    highScoreList(this)
{
    tickSpeed = 4;
    debugEnabled = false;
#ifndef DISABLE_EDITOR
    editorEnabled = true;
#endif
    StrCopy(startupWorldFileName, "TOWN");
    StrCopy(savedGameFileName, "SAVED");
    StrCopy(savedBoardFileName, "TEMP");
    initialized = false;
}

Game::~Game() {
}

void Game::Initialize() {
    if (!initialized) {
        WorldCreate(ENGINE_TYPE_ZZT);
        initialized = true;
    }
}

void Game::SidebarClearLine(int y) {
    driver->draw_string(60, y, 0x11, "                    ");
}

void Game::SidebarClear(void) {
    for (int i = 3; i <= 24; i++) {
        SidebarClearLine(i);
    }
}

void Game::BoardClose() {
    world.write_board(world.info.current_board, board); 

    board.stats.free_all_data();
}

void Game::BoardOpen(int16_t board_id) {
    if (board_id > world.board_count) {
        board_id = world.info.current_board;
    }

    world.read_board(board_id, board);
    world.info.current_board = board_id;
}

void Game::BoardChange(int16_t board_id) {
    board.tiles.set(board.stats[0].x, board.stats[0].y, {
        .element = EPlayer,
        .color = elementDef(EPlayer).color
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

    board.tiles.set(playerX, playerY, {.element = EPlayer, .color = elementDef(EPlayer).color});

    board.stats.count = 0;
    board.stats[0] = Stat();
    board.stats[0].x = playerX;
    board.stats[0].y = playerY;
    board.stats[0].cycle = 1;

    BoardUpdateDrawOffset();
}

void Game::WorldCreate(EngineType type) {
    InitEngine(type, false);
    world.board_count = 0;
    world.free_board(0);
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
	uint16_t seed = transition_table_start;
	uint8_t tx = transition_table_start - 1, ty = 0;

	do {
		if (tx < viewport.width && ty < viewport.height) {
			BoardDrawChar(
				viewport.cx_offset + 1 + tx,
				viewport.cy_offset + 1 + ty,
				color, chr);
		}
	} while (!transition_table_next(seed, tx, ty));

}

GBA_CODE_IWRAM
void Game::BoardDrawTile(int16_t x, int16_t y) {
    Tile tile = board.tiles.get(x, y);
    uint8_t drawn_char, drawn_color;

    if (!board.info.is_dark
        || elementDef(tile.element).visible_in_dark
        || (
            (world.info.torch_ticks > 0)
            // TODO: torchYMul not implemented here as codepath not used in SZZT
            && ((Sqr(board.stats[0].x - x) + Sqr(board.stats[0].y - y) * 2) < engineDefinition.torchDistSqr)
        ) || forceDarknessOff
    ) {
        if (tile.element == EEmpty) {
            drawn_color = 0x0F;
            drawn_char = ' ';
        } else if (elementDef(tile.element).has_draw_proc) {
            drawn_color = tile.color;
            elementDef(tile.element).draw(*this, x, y, drawn_char);
        } else if (tile.element < engineDefinition.textCutoff) {
            drawn_color = tile.color;
            drawn_char = elementDef(tile.element).character;
        } else {
            drawn_char = tile.color;
            if (tile.element == (engineDefinition.textCutoff + 6)) {
                drawn_color = 0x0F;
            } else if (driver->is_monochrome()) {
                drawn_color = ((tile.element - engineDefinition.textCutoff + 1) << 4);
            } else {
                drawn_color = ((tile.element - engineDefinition.textCutoff + 1) << 4) | 0x0F;
            }
        }
    } else {
        // darkness
        drawn_color = 0x07;
        drawn_char = 176;
    }

    BoardDrawChar(x, y, drawn_color, drawn_char);
}

GBA_CODE_IWRAM
void Game::BoardDrawChar(int16_t x, int16_t y, uint8_t drawn_color, uint8_t drawn_char) {
    int16_t x_pos = x - 1 - viewport.cx_offset;
    int16_t y_pos = y - 1 - viewport.cy_offset;
    if (x_pos >= 0 && y_pos >= 0 && x_pos < viewport.width && y_pos < viewport.height) {
        driver->draw_char(x_pos + viewport.x, y_pos + viewport.y, drawn_color, drawn_char);
    }
}

bool Game::BoardUpdateDrawOffset(void) {
    return viewport.point_at(board, board.stats[0]);
}

void Game::BoardPointCameraAt(int16_t sx, int16_t sy) {
    int16_t old_cx_offset = viewport.cx_offset;
    int16_t old_cy_offset = viewport.cy_offset;
    if (viewport.point_at(board, sx, sy)) {
        int deltaX = old_cx_offset - viewport.cx_offset;
        int deltaY = old_cy_offset - viewport.cy_offset;
        if ((Abs(deltaX) + Abs(deltaY)) == 1) {
            driver->scroll_chars(viewport.x, viewport.y, viewport.width, viewport.height, deltaX, deltaY);
            if (deltaX == 0) {
                int y_pos = ((deltaY > 0) ? viewport.cy_offset : (viewport.cy_offset + viewport.height - 1)) + 1;
                for (int i = 0; i < viewport.width; i++) {
                    BoardDrawTile(viewport.cx_offset + i + 1, y_pos);
                }
            } else {
                int x_pos = ((deltaX > 0) ? viewport.cx_offset : (viewport.cx_offset + viewport.width - 1)) + 1;
                for (int i = 0; i < viewport.height; i++) {
                    BoardDrawTile(x_pos, viewport.cy_offset + i + 1);
                }
            }
        } else {
            TransitionDrawToBoard();
        }
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
	uint16_t seed = transition_table_start;
	uint8_t tx = transition_table_start - 1, ty = 0;

	do {
		if (tx < viewport.width && ty < viewport.height) {
			BoardDrawTile(
				viewport.cx_offset + 1 + tx,
				viewport.cy_offset + 1 + ty);
		}
	} while (!transition_table_next(seed, tx, ty));
}

void Game::SidebarPromptCharacter(bool editable, int16_t x, int16_t y, const char *prompt, uint8_t &value) {
    SidebarClearLine(y);
    driver->draw_string(x, y, editable ? 0x1F : 0x1E, prompt);
    SidebarClearLine(y + 1);
    driver->draw_char(x + 5, y + 1, 0x9F, 31);
    SidebarClearLine(y + 2);

    do {
        for (int i = value - 4; i <= value + 4; i++) {
            driver->draw_char(x + i - value + 5, y + 2, 0x1E, (uint8_t) (i & 0xFF));
        }

        if (editable) {
            driver->delay(25);
            driver->update_input();
            if (driver->keyPressed == KeyTab) {
                driver->deltaX = 9;
            }

            uint8_t new_value = (uint8_t) (value + driver->deltaX);
            if (value != new_value) {
                value = new_value;
                SidebarClearLine(y + 2);
            }
        }
    } while (driver->keyPressed != KeyEnter && driver->keyPressed != KeyEscape && editable && !driver->shiftPressed);

    driver->draw_char(x + 5, y + 1, 0x1F, 31);
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
    driver->draw_string(x, y, editable ? 0x1F : 0x1E, str);
    SidebarClearLine(y + 1);
    SidebarClearLine(y + 2);
    StrCopy(str, "?....:....?");
    str[0] = startChar;
    str[10] = endChar;
    driver->draw_string(x, y + 2, 0x1E, str);

    do {
        if (editable) {
            driver->idle(IMUntilFrame);
            driver->draw_char(x + value + 1, y + 1, 0x9F, 31);

            driver->update_input();
            if (driver->keyPressed >= '1' && driver->keyPressed <= '9') {
                value = driver->keyPressed - 49;
                SidebarClearLine(y + 1);
            } else {
                uint8_t new_value = (uint8_t) (value + driver->deltaX);
                if (value != new_value && new_value >= 0 && new_value <= 8) {
                    value = new_value;
                    SidebarClearLine(y + 1);
                }
            }
        }
    } while (driver->keyPressed != KeyEnter && driver->keyPressed != KeyEscape && editable && !driver->shiftPressed);

    driver->draw_char(x + value + 1, y + 1, 0x1F, 31);
}

void Game::SidebarPromptChoice(bool editable, int16_t y, const char *prompt, const char *choiceStr, uint8_t &result) {
    SidebarClearLine(y);
    SidebarClearLine(y + 1);
    SidebarClearLine(y + 2);
    driver->draw_string(63, y, editable ? 0x1F : 0x1E, prompt);
    driver->draw_string(63, y + 2, 0x1E, choiceStr);

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
            driver->draw_char(63 + i, y + 1, 0x9F, 31);
            driver->delay(35);
            driver->update_input();

            uint8_t new_result = (uint8_t) (result + driver->deltaX);
            if (new_result != result && new_result >= 0 && new_result < choice_count) {
                result = new_result;
                SidebarClearLine(y + 1);
            }
        }
    } while (driver->keyPressed != KeyEnter && driver->keyPressed != KeyEscape && editable && !driver->shiftPressed);

    driver->draw_char(63 + i, y + 1, 0x1F, 31);
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
    driver->sound_queue(1, error_notes, SoundParse("s004x114x9", error_notes, 32));
    driver->delay(2000);
}

void Game::DisplayIOError(IOStream &stream) {
    if (!stream.errored()) return;

    TextWindow window = TextWindow(driver, filesystem);
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
        world.free_board(i);
    }
}

bool Game::WorldLoad(const char *filename, const char *extension, bool titleOnly, bool showError) {
    interface->SidebarShowMessage(0x0F, " Loading.....", true);

    sstring<31> ext_tokens;
    StrCopy(ext_tokens, extension);
    char *ext_token;
    char *ext_save = ext_tokens;
    char joinedName[256];
    IOStream *stream = NULL;

    while ((ext_token = strtok_r(ext_save, ";", &ext_save))) {
        StrJoin(joinedName, 2, filename, ext_token);
        if (stream != NULL) delete stream;
        stream = filesystem->open_file(joinedName, false);
        if (!stream->errored()) break;
    }

    if (!stream->errored()) {
        WorldUnload();
        bool result = false;
        
        for (int i = 0; i < SERIALIZERS_COUNT; i++) {
            WorldCreate(engine_types[i]);
            result = get_serializer(world.get_format())->deserialize_world(world, *stream, titleOnly, [this](auto bid) {
                interface->SidebarShowMessage(ProgressAnimColors[bid & 7], ProgressAnimStrings[bid & 7], true);
            });
            if (result) break;
        }

        if (result) {
            BoardOpen(world.info.current_board);
            StrCopy(loadedGameFileName, filename);
            highScoreList.Load(world.info.name);
            interface->SidebarHideMessage();
            delete stream;
            return true;
        }
    }

    if (stream->errored() && showError) {
        DisplayIOError(*stream);
    }

    interface->SidebarHideMessage();
    delete stream;
    return false;
}

bool Game::WorldSave(const char *filename, const char *extension) {
    BoardClose();
    interface->SidebarShowMessage(0x0F, " Saving...", true);

    char joinedName[256];
    StrJoin(joinedName, 2, filename, extension);
    IOStream *stream = filesystem->open_file(joinedName, true);

    if (!stream->errored()) {
        bool result = get_serializer(world.get_format())->serialize_world(world, *stream, [](auto i){});

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
    interface->SidebarPromptString(prompt, extension, newFilename, sizeof(newFilename), InputPMAlphanumeric);
    if (driver->keyPressed != KeyEscape && !StrEmpty(newFilename)) {
        strncpy(filename, newFilename, filename_len - 1);
        filename[filename_len - 1] = 0;

        if (StrEquals(extension, ".ZZT")) {
            StrCopy(world.info.name, filename);
        }

        WorldSave(filename, extension);
    }
}

bool Game::GameWorldLoad(const char *extension) {
    const char *title = StrEquals(extension, ".SAV") ? "Saved Games" : "ZZT Worlds";
    FileSelector *selector = new FileSelector(driver, filesystem, title, extension);

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
    if (board.stats.count < board.stats.stat_size()) {
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

    if (curTile.element == EPlayer && engineDefinition.isNot<QUIRK_PLAYER_BGCOLOR_FROM_FLOOR>()) {
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

    if (stat_id == 0) {
        // TODO: More accurate Super ZZT behaviour
        int16_t old_cx_offset = viewport.cx_offset;
        int16_t old_cy_offset = viewport.cy_offset;
        if (BoardUpdateDrawOffset()) {
            int deltaX = old_cx_offset - viewport.cx_offset;
            int deltaY = old_cy_offset - viewport.cy_offset;
            if ((Abs(deltaX) + Abs(deltaY)) == 1) {
                driver->scroll_chars(viewport.x, viewport.y, viewport.width, viewport.height, deltaX, deltaY);
                if (deltaX == 0) {
                    int y_pos = ((deltaY > 0) ? viewport.cy_offset : (viewport.cy_offset + viewport.height - 1)) + 1;
                    for (int i = 0; i < viewport.width; i++) {
                        BoardDrawTile(viewport.cx_offset + i + 1, y_pos);
                    }
                } else {
                    int x_pos = ((deltaX > 0) ? viewport.cx_offset : (viewport.cx_offset + viewport.width - 1)) + 1;
                    for (int i = 0; i < viewport.height; i++) {
                        BoardDrawTile(x_pos, viewport.cy_offset + i + 1);
                    }
                }
            } else {
                TransitionDrawToBoard();
            }
        }
    }

    BoardDrawTile(stat.x, stat.y);
    BoardDrawTile(oldX, oldY);

    if (stat_id == 0 && board.info.is_dark && world.info.torch_ticks > 0) {
        if ((Sqr(oldX - stat.x) + Sqr(oldY - stat.y)) == 1) {
            int16_t torchDx = engineDefinition.torchDx;
            int16_t torchDy = engineDefinition.torchDy;
            int16_t torchDistSqr = engineDefinition.torchDistSqr;
            int16_t torchYMul = engineDefinition.is<QUIRK_SUPER_ZZT_COMPAT_MISC>() ? 1 : 2;

            for (int ix = (stat.x - torchDx - 3); ix <= (stat.x + torchDx + 3); ix++) {
                if (ix < 1 || ix > board.width()) continue;
                for (int iy = (stat.y - torchDy - 3); iy <= (stat.y + torchDy + 3); iy++) {
                    if (iy < 1 || iy > board.height()) continue;
                    if ((((Sqr(ix-oldX))+(Sqr(iy-oldY)*torchYMul)) < torchDistSqr) !=
                        (((Sqr(ix-newX))+(Sqr(iy-newY)*torchYMul)) < torchDistSqr))
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

static const char * menu_str_restore(Game *game) {
    return game->filesystem->is_read_only() ? nullptr : "Restore game";
}

static const char * menu_str_save(Game *game) {
    return game->filesystem->is_read_only() ? nullptr : "Save game";
}

static const char * menu_str_sound(Game *game) {
    return game->driver->sound_is_enabled() ? "Be quiet" : "Be noisy";
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
                0x70 + (elementDef(EPlayer).color & 0x0F));

            if (world.info.health > 0) {
                world.info.board_time_sec = 0;
                if (board.info.reenter_when_zapped) {
					driver->sound_queue(4, "\x20\x01\x23\x01\x27\x01\x30\x01\x10\x01");

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

				driver->sound_queue(4, "\x10\x01\x20\x01\x13\x01\x23\x01");
            } else {
				driver->sound_queue(5,
					"\x20\x03\x23\x03\x27\x03\x30\x03"
					"\x27\x03\x2A\x03\x32\x03\x37\x03"
					"\x35\x03\x38\x03\x40\x03\x45\x03\x10\x0A"
				);
            }
        }
    } else {
        switch (board.tiles.get(attacker_stat.x, attacker_stat.y).element) {
            case EBullet:
                driver->sound_queue(3, "\x20\x01");
                break;
            case EObject:
                // pass
                break;
			default:
                driver->sound_queue(3, "\x40\x01\x10\x01\x50\x01\x30\x01");
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
		driver->sound_queue(2, "\x10\x01");
    }
}

bool Game::BoardShoot(ElementType element, int16_t x, int16_t y, int16_t dx, int16_t dy, int16_t source) {
    Tile destTile = board.tiles.get(x + dx, y + dy);
    if (elementDef(destTile.element).walkable || destTile.element == EWater) {
        AddStat(x + dx, y + dy, elementId(element), elementDef(element).color, 1, Stat());
        Stat& shotStat = board.stats[board.stats.count];
        shotStat.p1 = source;
        shotStat.step_x = dx;
        shotStat.step_y = dy;
        shotStat.p2 = 100;
        return true;
    } else if (destTile.element == EBreakable
        || (
            elementDef(destTile.element).destructible
            && ((destTile.element == EPlayer) == (source != 0))
            && (world.info.energizer_ticks <= 0)
        ))
    {
        BoardDamageTile(x + dx, y + dy);
		driver->sound_queue(2, "\x10\x01");
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

    if (board.info.is_dark && msgFlags.first<MESSAGE_HINT_TORCH>()) {
        DisplayMessage(200, "Room is dark - you need to light a torch!");
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
	driver->sound_queue(4,
		"\x30\x01\x34\x01\x37\x01"
		"\x31\x01\x35\x01\x38\x01"
		"\x32\x01\x36\x01\x39\x01"
		"\x33\x01\x37\x01\x3A\x01"
		"\x34\x01\x38\x01\x40\x01");
    BoardUpdateDrawOffset();
    TransitionDrawBoardChange();
    BoardEnter();
}

void Game::GameDebugPrompt(void) {
    sstring<50> input;
    StrClear(input);

    interface->SidebarPromptString(nullptr, nullptr, input, StrSize(input), InputPMAnyText);
    for (size_t i = 0; i < strlen(input); i++) {
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

	driver->sound_queue(10, "\x27\x04");
    GameUpdateSidebar();
}

void Game::GameAboutScreen(void) {
    TextWindowDisplayFile(driver, filesystem, "ABOUT.HLP", "About ZZT...");
}

void Game::GamePlayLoop(bool boardChanged) {
    GameDrawSidebar();

    if (justStarted) {
        GameAboutScreen();
        if (StrLength(startupWorldFileName) != 0) {
            if (!WorldLoad(startupWorldFileName, ".ZZT", true, false)) {
                if (!WorldLoad(startupWorldFileName, ".SZT", true, false)) {
                    WorldCreate(ENGINE_TYPE_ZZT);
                }
            }
            interface->SidebarGameDraw(*this, SIDEBAR_FLAG_SET_WORLD_NAME);
        }
        returnBoardId = world.info.current_board;
        BoardChange(0);
        justStarted = false;
    }

    Stat &player = board.stats[0];

    board.tiles.set(player.x, player.y, {
        .element = gameStateElement,
        .color = elementDef(gameStateElement).color
    });

    if (gameStateElement == EMonitor) {
        DisplayMessage(0, "");
        interface->SidebarShowMessage(0x0B, "Pick a command:", false);
    }

    BoardUpdateDrawOffset();

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
                BoardDrawChar(player.x, player.y, elementDef(EPlayer).color, elementDef(EPlayer).character);
            } else {
                if (board.tiles.get(player.x, player.y).element == EPlayer) {
                    BoardDrawChar(player.x, player.y, 0x0F, ' ');
                } else {
                    BoardDrawTile(player.x, player.y);
                }
            }

            interface->SidebarShowMessage(0x0F, "  Pausing...", true);
            driver->idle(IMUntilFrame);
            driver->update_input();

            // OpenZoo: Add "Q" and START to the list of allowed keys for exiting.
            if (driver->keyPressed == KeyEscape || UpCase(driver->keyPressed) == 'Q' || driver->joy_button_pressed(JoyButtonStart, false)) {
                GamePromptEndPlay();
            }

            if (driver->deltaX != 0 || driver->deltaY != 0) {
                int16_t dest_x = player.x + driver->deltaX;
                int16_t dest_y = player.y + driver->deltaY;
                elementDefAt(dest_x, dest_y).touch(*this, dest_x, dest_y, 0, driver->deltaX, driver->deltaY);
            }

            if (driver->deltaX != 0 || driver->deltaY != 0) {
                int16_t dest_x = player.x + driver->deltaX;
                int16_t dest_y = player.y + driver->deltaY;
                const Tile &dest_tile = board.tiles.get(dest_x, dest_y);
                if (elementDef(dest_tile.element).walkable) {
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
                            .color = elementDef(EPlayer).color
                        });
                        BoardDrawTile(player.x, player.y);
                        DrawPlayerSurroundings(player.x, player.y, 0);
                        DrawPlayerSurroundings(player.x - driver->deltaX, player.y - driver->deltaY, 0);
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

                // OpenZoo: On some platforms, it is necessary to occasionally yield,
                // which will not happen with a zero tick time duration otherwise.
                if (tickTimeDuration == 0) {
                    if (world.info.health <= 0) {
                        // The game over state certainly shouldn't consume 100% CPU.
                        // Pinning to VBlank should be sufficient to give the necessary "feel"
                        // that the DOS version would, though.
                        driver->idle(IMUntilFrame);
                    } else {
                        // If an user explicitly selects "fastest speed", just yield for as little
                        // as possible.
                        driver->idle(IMYield);
                    }
                }

                driver->update_input();
            } else {
                driver->idle(IMUntilPit);
            }
        }
    } while (!((exitLoop || gamePlayExitRequested) && gamePlayExitRequested));

    driver->sound_clear_queue();

    if (gameStateElement == EPlayer) {
        if (world.info.health <= 0) {
            highScoreList.Add(world.info.name, world.info.score);
        }
    } else if (gameStateElement == EMonitor) {
        interface->SidebarHideMessage();
    }

    board.tiles.set(player.x, player.y, {
        .element = EPlayer,
        .color = elementDef(EPlayer).color
    });

    driver->sound_set_block_queueing(false);
}

const MenuEntry ZZT::TitleMenu[] = {
    {.id = 'W', .keys = {'W'}, .name = "Load world"},
    {.id = 'P', .keys = {'P'}, .name = "Play"},
    {.id = 'R', .keys = {'R'}, .name_func = menu_str_restore},
    {.id = 'A', .keys = {'A'}, .name = "About ZZT"},
    {.id = 'E', .keys = {'E'}, .name_func = menu_str_editor},
    {.id = 'S', .keys = {'S'}, .name = "Game speed"},
    {.id = 'H', .keys = {'H'}, .name = "High scores"},
    {.id = '|', .keys = {'|'}, .name = "Console command"},
    {.id = 'Q', .keys = {'Q', KeyEscape}, .name = "Quit ZZT"},
    {.id = -1}
};

const MenuEntry ZZT::PlayMenu[] = {
    {.id = 'T', .keys = {'T'}, .joy_button = JoyButtonB}, // Torch
    {.id = 'S', .keys = {'S'}, .name_func = menu_str_save},
    {.id = 'P', .keys = {'P'}}, // Pause - hidden in menu mode
    {.id = 'H', .keys = {'H'}, .name = "Help"},
    {.id = '?', .keys = {'?'}, .name = "Console command"},
    {.id = 'B', .keys = {'B'}, .name_func = menu_str_sound},
    {.id = 'Q', .keys = {'Q', KeyEscape}, .name = "Quit game"},
    {.id = -1}
};

void Game::GameTitleLoop(void) {
    Initialize();

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
                    if (GameWorldLoad(".ZZT;.SZT")) {
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
                    driver->keyPressed = 0;
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
