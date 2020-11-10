#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "editor.h"
#include "file_selector.h"
#include "txtwind.h"
#include "utils.h"

using namespace ZZT;
using namespace ZZT::Utils;

static const char EditorColorNames[16][4] = {
    "Blk", "DBl", "DGn", "DCy", "DRe", "DMa", "Bro", "LGr",
    "DGr", "LBl", "LGn", "LCy", "LRe", "LMa", "Yel", "Wht"
};

static const uint8_t EditorPatterns[] = {
    ESolid, ENormal, EBreakable, EWater, EEmpty, ELine
};
static constexpr size_t EditorPatternCount = sizeof(EditorPatterns) / sizeof(uint8_t);

static const char NeighborBoardStrs[4][8] = {
    "Board \x18",
    "Board \x19",
    "Board \x1B",
    "Board \x1A"
};

EditorCopiedTile::EditorCopiedTile() {
    stat.data.free_data();
    Clear();
}

EditorCopiedTile::~EditorCopiedTile() {
    stat.data.free_data();
}

void EditorCopiedTile::Clear(void) {
    has_stat = false;
    tile.element = 0;
    tile.color = 0x0F;
    preview_char = 0;
    preview_color = 0x0F;
    stat.data.free_data();
}

Editor::Editor(Game *game) {
    this->game = game;

    // InitEditorStatSettings
    for (int i = 0; i < ElementCount; i++) {
        this->stat_settings[i] = {
            .p1 = 4,
            .p2 = 4,
            .p3 = 0,
            .step_x = 0,
            .step_y = -1
        };
    }

    this->stat_settings[EObject].p1 = 1;
    this->stat_settings[EBear].p1 = 8;
}

Editor::~Editor() {

}

uint8_t Editor::GetDrawingColor(uint8_t element) {
    const ElementDef &def = game->elementDefs[element];

    if (element == EPlayer) {
        return def.color;
    } else if (color_ignore_defaults || (def.color == ColorChoiceOnBlack)) {
        return cursor_color;
    } else if (def.color == ColorWhiteOnChoice) {
        return ((cursor_color & 0x07) << 4) | 0x0F;
    } else if (def.color == ColorChoiceOnChoice) {
        return ((cursor_color & 0x07) * 0x11) | 0x08;
    } else {
        return game->elementDefs[element].color;
    }
}

void Editor::AppendBoard(void) {
    // TODO: add bool - to fix P3 values if too many boards
    if (game->world.board_count < MAX_BOARD) {
        game->BoardClose();

        game->world.board_count++;
        game->world.info.current_board = game->world.board_count;
        game->world.board_len[game->world.board_count] = 0;
        game->BoardCreate();

        game->TransitionDrawToBoard();

        do {
            game->interface->PopupPromptString("Room's Title:", game->board.name, StrSize(game->board.name));
        } while (StrEmpty(game->board.name));

        game->TransitionDrawToBoard();
    }
}

void Editor::UpdateCursorColor(void) {
    uint8_t color_bg = cursor_color >> 4;
    uint8_t color_fg = cursor_color & 0x0F;

    game->driver->draw_string(72, 19, 0x1E, EditorColorNames[color_fg]);
    game->driver->draw_string(76, 19, 0x1E, EditorColorNames[color_bg]);

    game->driver->draw_string(61, 24, 0x1F, "                ");
    if (color_bg == color_fg) {
        game->driver->draw_char(61 + color_bg, 24, 0x9F, 30);
    } else {
        game->driver->draw_char(61 + color_fg, 24, 0x1F, 24);
        game->driver->draw_char(61 + color_bg, 24, 0x1F, '^');
    }
}

void Editor::UpdateCursorPattern(void) {
    game->driver->draw_string(61, 22, 0x1F, "                ");
    game->driver->draw_char(61 + cursor_pattern, 22, 0x1F, 30);
}

void Editor::UpdateDrawMode(void) {
    const char *desc;
    uint8_t color = 0x9E;

    switch (draw_mode) {
        case EDMDrawingOn: desc = "Draw"; break;
        case EDMTextEntry: desc = "Text"; break;
        case EDMDrawingOff: desc = "None"; color = 0x1E; break;
        default: return;
    }

    game->driver->draw_string(75, 16, color, desc);
}

void Editor::UpdateColorIgnoreDefaults(void) {
    game->driver->draw_char(78, 23, 0x1F, color_ignore_defaults ? 'd' : 'D');
}

void Editor::UpdateCopiedPatterns(void) {
    for (int i = 0; i < COPIED_TILES_COUNT; i++) {
        game->driver->draw_char(
            61 + EditorPatternCount + i,
            21,
            copied_tiles[i].preview_color,
            copied_tiles[i].preview_char
        );
    }
}

void Editor::CopyPattern(int16_t x, int16_t y, EditorCopiedTile &copied) {
    const Tile &tile = game->board.tiles.get(x, y);
    if (tile.element == EPlayer) {
        return;
    }

    int16_t stat_id = game->board.stats.id_at(x, y);
    
    copied.Clear();
    copied.tile = tile;
    copied.has_stat = stat_id > 0;
    if (copied.has_stat) {
        const Stat &stat = game->board.stats[stat_id];
        copied.stat = stat;
        copied.stat.data.duplicate();
    }
    
    // generate preview
    // TODO: unify with BoardDrawTile
    const ElementDef &def = game->elementDefs[tile.element];
    if (def.has_draw_proc) {
        def.draw(*game, x, y, copied.preview_char);
    } else if (tile.element < ETextBlue) {
        uint8_t elem_ch = game->elementCharOverrides[tile.element];
        if (elem_ch == 0) elem_ch = def.character;

        copied.preview_char = elem_ch;
    } else {
        copied.preview_char = def.color;
    }

    if (tile.element < ETextBlue) {
        copied.preview_color = tile.color;
    } else if (tile.element == ETextWhite) {
        copied.preview_color = 0x0F;
    } else {
        copied.preview_color = ((tile.element - ETextBlue + 1) << 4) | 0x0F;
    }
}

void Editor::CopyPatternToCurrent(int16_t x, int16_t y) {
    if (cursor_pattern >= EditorPatternCount) {
        CopyPattern(x, y, copied_tiles[cursor_pattern - EditorPatternCount]);
    }
}

void Editor::DrawSidebar(void) {
    game->SidebarClear();
    game->SidebarClearLine(0);
    game->SidebarClearLine(1);
    game->SidebarClearLine(2);

    game->driver->draw_string(61, 0, 0x1F, "     - - - -       ");
    game->driver->draw_string(62, 1, 0x70, "  ZZT Editor   ");
    game->driver->draw_string(61, 2, 0x1F, "     - - - -       ");
    game->driver->draw_string(61, 4, 0x70, " L ");
    game->driver->draw_string(64, 4, 0x1F, " Load");
    game->driver->draw_string(61, 5, 0x30, " S ");
    game->driver->draw_string(64, 5, 0x1F, " Save");
    game->driver->draw_string(70, 4, 0x70, " H ");
    game->driver->draw_string(73, 4, 0x1F, " Help");
    game->driver->draw_string(70, 5, 0x30, " Q ");
    game->driver->draw_string(73, 5, 0x1F, " Quit");
    game->driver->draw_string(61, 7, 0x70, " B ");
    game->driver->draw_string(64, 7, 0x1F, " Switch boards");
    game->driver->draw_string(61, 8, 0x30, " I ");
    game->driver->draw_string(64, 8, 0x1F, " Board Info");
    game->driver->draw_string(61, 10, 0x70, "  f1   ");
    game->driver->draw_string(68, 10, 0x1F, " Item");
    game->driver->draw_string(61, 11, 0x30, "  f2   ");
    game->driver->draw_string(68, 11, 0x1F, " Creature");
    game->driver->draw_string(61, 12, 0x70, "  f3   ");
    game->driver->draw_string(68, 12, 0x1F, " Terrain");
    game->driver->draw_string(61, 13, 0x30, "  f4   ");
    game->driver->draw_string(68, 13, 0x1F, " Enter text");
    game->driver->draw_string(61, 15, 0x70, " Space ");
    game->driver->draw_string(68, 15, 0x1F, " Plot");
    game->driver->draw_string(61, 16, 0x30, "  Tab  ");
    game->driver->draw_string(68, 16, 0x1F, " Mode:");
    game->driver->draw_string(61, 18, 0x70, " P ");
    game->driver->draw_string(64, 18, 0x1F, " Pattern");
    game->driver->draw_string(61, 19, 0x30, " C ");
    game->driver->draw_string(64, 19, 0x1F, " Color:");
    game->driver->draw_char(75, 19, 0x1F, 26);

    // Colors
    for (int i = 0; i < 16; i++) {
        game->driver->draw_char(61 + i, 23, 0x10 | i, 219);
    }
    UpdateCursorColor();
    UpdateColorIgnoreDefaults();

    // patterns
    for (int i = 0; i < EditorPatternCount; i++) {
        game->driver->draw_char(61 + i, 21, 0x0F, game->elementDefs[EditorPatterns[i]].character);
    }
    UpdateCopiedPatterns();
    UpdateCursorPattern();

    UpdateDrawMode();
}

void Editor::DrawTileAndNeighhborsAt(int16_t x, int16_t y) {
    game->BoardDrawTile(x, y);
    for (int i = 0; i < 4; i++) {
        int16_t ix = x + NeighborDeltaX[i];
        int16_t iy = y + NeighborDeltaY[i];
        if (ix >= 1 && ix <= game->board.width() && iy >= 1 && iy <= game->board.height()) {
            game->BoardDrawTile(ix, iy);
        }
    }
}

void Editor::DrawRefresh(void) {
    sstring<128> name;

    game->BoardDrawBorder();
    DrawSidebar();
    game->TransitionDrawToBoard();

    if (!StrEmpty(game->board.name)) {
        StrJoin(name, 3, " ", game->board.name, " ");
    } else {
        StrCopy(name, " Untitled ");
    }
    game->driver->draw_string((59 - strlen(name)) / 2, 0, 0x70, name);
}

void Editor::SetTile(int16_t x, int16_t y, Tile tile) {
    game->board.tiles.set(x, y, tile);
    DrawTileAndNeighhborsAt(x, y);
}

void Editor::SetAndCopyTile(int16_t x, int16_t y, Tile tile) {
    SetTile(x, y, tile);

    CopyPatternToCurrent(x, y);
    UpdateCopiedPatterns();
}

void Editor::AskSaveChanged(void) {
    game->driver->keyPressed = 0;
    if (was_modified) {
        if (game->interface->SidebarPromptYesNo("Save first? ", true)) {
            if (game->driver->keyPressed != KeyEscape) {
                game->GameWorldSave("Save world", game->loadedGameFileName, sizeof(game->loadedGameFileName), ".ZZT");
            }
        }
    }
    StrCopy(game->world.info.name, game->loadedGameFileName);
}

bool Editor::PrepareModifyTile(int16_t x, int16_t y) {
    was_modified = true;
    bool result = game->BoardPrepareTileForPlacement(x, y);
    DrawTileAndNeighhborsAt(x, y);
    return result;
}

bool Editor::PrepareModifyStatAtCursor(void) {
    if (game->board.stats.count >= MAX_STAT) {
        return false;
    }
    return PrepareModifyTile(cursor_x, cursor_y);
}

void Editor::PlaceTile(int16_t x, int16_t y) {
    const Tile &tile = game->board.tiles.get(x, y);

    if (cursor_pattern < EditorPatternCount) {
        if (PrepareModifyTile(x, y)) {
            game->board.tiles.set(x, y, {
                .element = EditorPatterns[cursor_pattern],
                .color = cursor_color
            });
        }
    } else {
        EditorCopiedTile &copied = copied_tiles[cursor_pattern - EditorPatternCount];
        if (copied.has_stat) {
            if (PrepareModifyStatAtCursor()) {
                game->AddStat(x, y, copied.tile.element, copied.tile.color, copied.stat.cycle, copied.stat);
            }
        } else {
            if (PrepareModifyTile(x, y)) {
                game->board.tiles.set(x, y, copied.tile);
            }
        }
    }

    DrawTileAndNeighhborsAt(x, y);
}

void Editor::RemoveTile(int16_t x, int16_t y) {
    int16_t stat_id = game->board.stats.id_at(x, y);
    if (stat_id > 0) {
        game->RemoveStat(stat_id);
    } else if (stat_id < 0) {
        game->board.tiles.set_element(x, y, EEmpty);
    } else {
        // stat_id = 0 (player) cannot be modified
        return;
    }
    game->BoardDrawTile(x, y);
}

static const char *boolToString(bool value) {
    return value ? "Yes" : "No ";
}

void Editor::EditBoardInfo(void) {
    sstring<50> num_str;

    TextWindow window = TextWindow(game->driver, game->filesystem);
    window.DrawOpen();
    bool exit_requested = false;
    
    do {
        window.Clear();

        window.selectable = true;
        StrCopy(window.title, "Board Information");

        window.Append(DynString("         Title: ") + game->board.name);
        StrFromInt(num_str, game->board.info.max_shots);
        window.Append(DynString("      Can fire: ") + num_str + " shots.");
        window.Append(DynString(" Board is dark: ") + boolToString(game->board.info.is_dark));
        
        for (int i = 0; i < 4; i++) {
            GetBoardName(game->board.info.neighbor_boards[i], true, num_str, sizeof(num_str));
            window.Append(DynString("       ") + NeighborBoardStrs[i] + ": " + num_str);
        }

        window.Append(DynString("Re-enter when zapped: ") + boolToString(game->board.info.reenter_when_zapped));
        StrFromInt(num_str, game->board.info.time_limit_seconds);
        window.Append(DynString("  Time limit, 0=None: ") + num_str + " sec.");
        window.Append("          Quit!");

        window.Select(false, false);
        was_modified |= (game->driver->keyPressed == KeyEnter && window.line_pos != (window.line_count - 1));
        if (game->driver->keyPressed == KeyEnter) {
            switch (window.line_pos) {
                case 0: { // title
                    game->interface->PopupPromptString("New title for board:", game->board.name, sizeof(game->board.name));
                } break;
                case 1: { // max shots
                    StrFromInt(num_str, game->board.info.max_shots);
                    game->interface->SidebarPromptString("Maximum shots?", "", num_str, sizeof(num_str), PMNumeric);
                    if (!StrEmpty(num_str)) {
                        game->board.info.max_shots = atoi(num_str);
                    }
                    DrawSidebar();
                } break;
                case 2: { // is dark
                    game->board.info.is_dark = !game->board.info.is_dark;
                } break;
                case 3:
                case 4:
                case 5:
                case 6: { // neighbor boards
                    int id = window.line_pos - 3;
                    game->board.info.neighbor_boards[id] = SelectBoard(
                        NeighborBoardStrs[id],
                        game->board.info.neighbor_boards[id],
                        true
                    );
                    if (game->board.info.neighbor_boards[id] > game->world.board_count) {
                        AppendBoard();
                    }
                } break;
                case 7: { // re-enter when zapped
                    game->board.info.reenter_when_zapped = !game->board.info.reenter_when_zapped;
                } break;
                case 8: { // time limit
                    StrFromInt(num_str, game->board.info.time_limit_seconds);
                    game->interface->SidebarPromptString("Time limit?", " Sec", num_str, sizeof(num_str), PMNumeric);
                    if (!StrEmpty(num_str)) {
                        game->board.info.time_limit_seconds = atoi(num_str);
                    }
                    DrawSidebar();
                } break;
                default: {
                    exit_requested = true;
                } break;
            }
        } else {
            exit_requested = true;
        }
    } while (!exit_requested);
    
    window.DrawClose();
}

void Editor::DrawTextEditSidebar(void) {
    game->SidebarClear();
    game->driver->draw_string(61, 4, 0x30, " Return ");
    game->driver->draw_string(64, 5, 0x1F, " Insert line");
    game->driver->draw_string(61, 7, 0x70, " Ctrl-Y ");
    game->driver->draw_string(64, 8, 0x1F, " Delete line");
    game->driver->draw_string(61, 10, 0x30, " Cursor keys ");
    game->driver->draw_string(64, 11, 0x1F, " Move cursor");
    game->driver->draw_string(61, 13, 0x70, " Insert ");
    game->driver->draw_string(64, 14, 0x1F, " Insert mode: ");
    game->driver->draw_string(61, 16, 0x30, " Delete ");
    game->driver->draw_string(64, 17, 0x1F, " Delete char");
    game->driver->draw_string(61, 19, 0x70, " Escape ");
    game->driver->draw_string(64, 20, 0x1F, " Exit editor");
}

void Editor::EditStatText(int16_t stat_id, const char *prompt) {
    bool affected_stats[MAX_STAT + 2];
    Stat &stat = game->board.stats[stat_id];
    TextWindow window = TextWindow(game->driver, game->filesystem);
    StrCopy(window.title, prompt);
    window.DrawOpen();
    window.selectable = false;
    CopyStatDataToTextWindow(stat, window);

    if (stat.data.len > 0) {
        for (int i = 0; i <= game->board.stats.count; i++) {
            affected_stats[i] = game->board.stats[i].data == stat.data;
        }
        stat.data.free_data();
    } else {
        memset(affected_stats, 0, game->board.stats.count + 2);
    }

    DrawTextEditSidebar();
    window.Edit();

    int16_t len = 0;
    for (int i = 0; i < window.line_count; i++) {
        len += window.lines[i]->length() + 1;
    }
    stat.data.alloc_data(len);

    for (int i = 0; i <= game->board.stats.count; i++) {
        if (i == stat_id) continue;
        if (affected_stats[i]) {
            game->board.stats[i].data = stat.data;
        }
    }

    char *data_ptr = stat.data.data;
    for (int i = 0; i < window.line_count; i++) {
        int len = window.lines[i]->length();
        memcpy(data_ptr, window.lines[i]->c_str(), len);
        data_ptr[len] = '\r';
        data_ptr += len + 1;
    }

    window.DrawClose();
    game->driver->keyPressed = 0;
}

void Editor::EditStat(int16_t stat_id) {
    Stat &stat = game->board.stats[stat_id];
    const Tile &tile = game->board.tiles.get(stat.x, stat.y);
    const ElementDef &def = game->elementDefs[tile.element];
    
    game->SidebarClear();
    was_modified = true;
    
    const char *category_name = "";
    for (int i = 0; i <= tile.element; i++) {
        const ElementDef &i_def = game->elementDefs[i];
        if (i_def.editor_category == def.editor_category && !StrEmpty(i_def.category_name)) {
            category_name = i_def.category_name;
        }
    }

    game->driver->draw_string(64, 6, 0x1E, category_name);
    game->driver->draw_string(64, 7, 0x1F, def.name);

    sstring<50> text;
    StrClear(text);

    for (int i = 0; i < 2; i++) {
        bool selected = i == 1;
        game->driver->keyPressed = 0;
        int iy = 9;

        if (!StrEmpty(def.p1_name)) {
            if (StrEmpty(def.param_text_name)) {
                game->SidebarPromptSlider(selected, 63, iy, def.p1_name, stat.p1);
            } else {
                if (stat.p1 == 0) {
                    stat.p1 = stat_settings[tile.element].p1;
                }
                game->BoardDrawTile(stat.x, stat.y);
                game->SidebarPromptCharacter(selected, 63, iy, def.p1_name, stat.p1);
                game->BoardDrawTile(stat.x, stat.y);
            }

            if (selected) {
                stat_settings[tile.element].p1 = stat.p1;
            }

            iy += 4;
        }

        if (game->driver->keyPressed == KeyEscape) continue;
        if (!StrEmpty(def.param_text_name)) {
            if (selected) {
                EditStatText(stat_id, def.param_text_name);
            }
        }

        if (game->driver->keyPressed == KeyEscape) continue;
        if (!StrEmpty(def.p2_name)) {
            uint8_t prompt_byte = stat.p2 & 0x7F;
            game->SidebarPromptSlider(selected, 63, iy, def.p2_name, prompt_byte);
            
            if (selected) {
                stat.p2 = (stat.p2 & 0x80) + prompt_byte;
                stat_settings[tile.element].p2 = stat.p2;
            }

            iy += 4;
        }

        if (game->driver->keyPressed == KeyEscape) continue;
        if (!StrEmpty(def.param_bullet_type_name)) {
            uint8_t prompt_byte = stat.p2 >> 7;
            game->SidebarPromptChoice(selected, iy, def.param_bullet_type_name, "Bullets Stars", prompt_byte);
            
            if (selected) {
                stat.p2 = (stat.p2 & 0x7F) | (prompt_byte << 7);
                stat_settings[tile.element].p2 = stat.p2;
            }

            iy += 4;
        }

        if (game->driver->keyPressed == KeyEscape) continue;
        if (!StrEmpty(def.param_direction_name)) {
            game->SidebarPromptDirection(selected, iy, def.param_direction_name,
                stat.step_x, stat.step_y);

            if (selected) {
                stat_settings[tile.element].step_x = stat.step_x;
                stat_settings[tile.element].step_y = stat.step_y;
            }

            iy += 4;
        }

        if (game->driver->keyPressed == KeyEscape) continue;
        if (!StrEmpty(def.param_board_name)) {
            if (selected) {
                int16_t selected_board = SelectBoard(def.param_board_name, stat.p3, true);
                if (selected_board != 0) {
                    stat.p3 = (uint8_t) selected_board;
                    if (stat.p3 > game->world.board_count) {
                        AppendBoard();
                    }
                    stat_settings[tile.element].p3 = stat.p3;
                } else {
                    game->driver->keyPressed = KeyEscape;
                }
            } else {
                GetBoardName(stat.p3, true, text, (80 - (63 + 6)) + 1);
                game->driver->draw_string(63, iy, 0x1F, "Room: ");
                game->driver->draw_string(63 + 6, iy, 0x1F, text);
            }
        }
    }

    if (game->driver->keyPressed != KeyEscape) {
        CopyPatternToCurrent(stat.x, stat.y);
    }
}

void Editor::TransferBoard(void) {
    sstring<255> filename_joined;
    uint8_t i = 1;
    game->SidebarPromptChoice(true, 3, "Transfer board:", "Import Export", i);
    if (game->driver->keyPressed != KeyEscape) {
        if (i == 0) {
            FileSelector *selector = new FileSelector(game->driver, game->filesystem, "Import board", ".BRD");

            if (selector->select()) {
                StrCopy(game->savedBoardFileName, selector->get_filename());
                delete selector;

                StrJoin(filename_joined, 2, game->savedBoardFileName, ".BRD");
                IOStream *stream = game->filesystem->open_file(filename_joined, false);

                if (!stream->errored()) {
                    int board = game->world.info.current_board;

                    game->BoardClose();
                    free(game->world.board_data[board]);
                    game->world.board_len[board] = stream->read16();
                    if (!stream->errored()) {
                        game->world.board_data[board] = (uint8_t*) malloc(game->world.board_len[board]);
                        stream->read(game->world.board_data[board], game->world.board_len[board]);
                    }

                    if (stream->errored()) {
                        game->world.board_len[board] = 0;
                        game->DisplayIOError(*stream);
                        game->BoardCreate();
                        DrawRefresh();
                    } else {
                        game->BoardOpen(game->world.info.current_board);
                        DrawRefresh();
                        for (int i = 0; i < 4; i++) {
                            game->board.info.neighbor_boards[i] = 0;
                        }
                    }
                } else {
                    game->DisplayIOError(*stream);
                }

                delete stream;
            } else {
                delete selector;
            }
        } else if (i == 1) {
            // export
            game->interface->SidebarPromptString("Export board", ".BRD", game->savedBoardFileName, sizeof(game->savedBoardFileName), PMAlphanum);
            if (game->driver->keyPressed != KeyEscape && !StrEmpty(game->savedBoardFileName)) {
                StrJoin(filename_joined, 2, game->savedBoardFileName, ".BRD");
                IOStream *stream = game->filesystem->open_file(filename_joined, false);

                if (!stream->errored()) {
                    int board = game->world.info.current_board;

                    game->BoardClose();
                    stream->write16(game->world.board_len[board]);
                    stream->write(game->world.board_data[board], game->world.board_len[board]);
                    game->BoardOpen(board);
                }

                game->DisplayIOError(*stream);
            }
        }
    }
    DrawSidebar();
}

void Editor::FloodFill(int16_t x, int16_t y, Tile from) {
    int16_t x_pos[256];
    int16_t y_pos[256];

    uint8_t to_fill = 1;
    uint8_t filled = 0;

    while (to_fill != filled) {
        Tile tile_old = game->board.tiles.get(x, y);
        PlaceTile(x, y);
        const Tile &tile_new = game->board.tiles.get(x, y);
        if (tile_old.element != tile_new.element || tile_old.color != tile_new.color) {
            for (int i = 0; i < 4; i++) {
                int16_t nx = x + NeighborDeltaX[i];
                int16_t ny = y + NeighborDeltaY[i];
                const Tile &ntile = game->board.tiles.get(nx, ny); 

                if (ntile.element == from.element && (from.element == 0 || ntile.color == from.color)) {
                    x_pos[to_fill] = nx;
                    y_pos[to_fill] = ny;
                    to_fill++;
                }
            }
        }

        filled++;
        x = x_pos[filled];
        y = y_pos[filled];
    }
}

void Editor::EditHelpFile() {
    sstring<50> filename;
    filename[0] = '*';
    game->interface->SidebarPromptString("File to edit", ".HLP", filename + 1, sizeof(filename) - 5, PMAlphanum);
    if (filename[1] != 0) {
        TextWindow window = TextWindow(game->driver, game->filesystem);
        strcat(filename, ".HLP");
        window.OpenFile(filename, false);
        StrJoin(window.title, 2, "Editing ", filename);
        window.DrawOpen();
        DrawTextEditSidebar();
        window.Edit();
        window.SaveFile(filename);
        window.DrawClose();
    }
}

void Editor::GetBoardName(int16_t board_id, bool title_screen_is_none, char *buffer, size_t buf_len) {
    buffer[buf_len - 1] = 0;

    if (board_id == 0 && title_screen_is_none) {
        strncpy(buffer, "None", buf_len - 1);
    } else if (board_id == game->world.info.current_board) {
        strncpy(buffer, game->board.name, buf_len - 1);
    } else {
        size_t size = game->world.board_data[board_id][0];
        if (size > (buf_len - 1)) size = buf_len - 1;
        memcpy(buffer, game->world.board_data[board_id] + 1, size);
        buffer[size] = 0;
    }
}

int Editor::SelectBoard(const char *title, int16_t current_board, bool title_screen_is_none) {
    sstring<50> boardName;
    TextWindow window = TextWindow(game->driver, game->filesystem);
    StrCopy(window.title, title);
    window.line_pos = current_board;
    window.selectable = true;
    for (int i = 0; i <= game->world.board_count; i++) {
        GetBoardName(i, title_screen_is_none, boardName, sizeof(boardName));
        window.Append(boardName);
    }
    window.Append("Add new board");
    window.DrawOpen();
    window.Select(false, false);
    window.DrawClose();
    return (game->driver->keyPressed == KeyEscape) ? 0 : window.line_pos;
}

void Editor::Loop(void) {
    if (game->world.info.is_save || (game->WorldGetFlagPosition("SECRET") >= 0)) {
        game->WorldUnload();
        game->WorldCreate();
    }

    game->InitElementsEditor();
    game->currentTick = 0;
    was_modified = false;
    cursor_x = 30;
    cursor_y = 12;
    draw_mode = EDMDrawingOff;
    cursor_pattern = 0;
    cursor_color = 0x0E;
    color_ignore_defaults = false;
    cursor_blinker = 0;

    for (int i = 0; i < COPIED_TILES_COUNT; i++) {
        copied_tiles[i].Clear();
    }

    if (game->world.info.current_board != 0) {
        game->BoardChange(game->world.info.current_board);
    }

    DrawRefresh();
    if (game->world.board_count == 0) {
        AppendBoard();
    }

    editor_exit_requested = false;
    do {
        if (draw_mode == EDMDrawingOn) {
            PlaceTile(cursor_x, cursor_y);
        }

        game->driver->idle(IMUntilFrame);
        game->driver->update_input();

        if (game->driver->keyPressed == 0 && game->driver->deltaX == 0
            && game->driver->deltaY == 0 && !game->driver->shiftPressed
        ) {
            if (game->HasTimeElapsed(game->tickTimeCounter, 15)) {
                cursor_blinker = (cursor_blinker + 1) % 3;
            }
            if (cursor_blinker == 0) {
                game->BoardDrawTile(cursor_x, cursor_y);
            } else {
                game->driver->draw_char(cursor_x - 1, cursor_y - 1, 0x0F, 197);
            }
        } else {
            game->BoardDrawTile(cursor_x, cursor_y);
        }

        if (draw_mode == EDMTextEntry) {
            auto &ch = game->driver->keyPressed;
            if (ch >= 32 && ch < 128) {
                if (PrepareModifyTile(cursor_x, cursor_y)) {
                    i = (cursor_color & 0x07) + ETextBlue - 1;
                    if (i < ETextBlue) i = ETextWhite;

                    game->board.tiles.set(cursor_x, cursor_y, {
                        .element = i,
                        .color = (uint8_t) ch
                    });
                    DrawTileAndNeighhborsAt(cursor_x, cursor_y);
                    game->driver->deltaX = 1;
                    game->driver->deltaY = 0;
                }
                ch = 0;
            } else if (ch == KeyBackspace && cursor_x > 1 && PrepareModifyTile(cursor_x - 1, cursor_y)) {
                cursor_x--;
            } else if (ch == KeyEnter || ch == KeyEscape) {
                draw_mode = EDMDrawingOff;
                ch = 0;
                UpdateDrawMode();
            }
        }

        const Tile &cursor_tile = game->board.tiles.get(cursor_x, cursor_y);

        if (game->driver->shiftPressed || game->driver->keyPressed == ' ') {
            game->driver->shiftAccepted = true;

            can_modify = cursor_tile.element == 0 || game->driver->deltaX != 0 || game->driver->deltaY != 0;
            if (!can_modify && game->elementDefs[cursor_tile.element].placeable_on_top && cursor_pattern >= EditorPatternCount) {
                // Place stat "under"
                can_modify = copied_tiles[cursor_pattern - EditorPatternCount].has_stat;
            }

            if (can_modify) {
                PlaceTile(cursor_x, cursor_y);
            } else {
                can_modify = PrepareModifyTile(cursor_x, cursor_y);
                if (can_modify) {
                    game->board.tiles.set_element(cursor_x, cursor_y, EEmpty);
                }
            }
        } else if (game->driver->keyPressed == KeyDelete) {
            RemoveTile(cursor_x, cursor_y);
        }

        if (game->driver->deltaX != 0 || game->driver->deltaY != 0) {
            cursor_x += game->driver->deltaX;
            if (cursor_x < 1) cursor_x = 1;
            if (cursor_x > game->board.width()) cursor_x = game->board.width();

            cursor_y += game->driver->deltaY;
            if (cursor_y < 1) cursor_y = 1;
            if (cursor_y > game->board.height()) cursor_y = game->board.height();

            game->driver->draw_char(cursor_x - 1, cursor_y - 1, 0x0F, 197);

            if (game->driver->keyPressed == 0 && game->driver->joystickEnabled) {
                game->driver->delay(70);
            }
            game->driver->shiftAccepted = true;
        }

        if (game->driver->keyPressed >= '0' && game->driver->keyPressed <= '9') {
            int i = (game->driver->keyPressed == '0') ? 9 : (game->driver->keyPressed - '1');
            if (i < COPIED_TILES_COUNT) {
                cursor_pattern = EditorPatternCount + i;
                UpdateCursorPattern();
            }
        } else switch(UpCase(game->driver->keyPressed)) {
            case '`': {
                DrawRefresh();
            } break;
            case 'P': {
                if (game->driver->keyPressed == 'P') {
                    if (cursor_pattern > 1) {
                        cursor_pattern--;
                    } else {
                        cursor_pattern = EditorPatternCount + COPIED_TILES_COUNT - 1;
                    }
                } else {
                    if (cursor_pattern < (EditorPatternCount + COPIED_TILES_COUNT - 1)) {
                        cursor_pattern++;
                    } else {
                        cursor_pattern = 1;
                    }
                }
                UpdateCursorPattern();
            } break;
            case 'C': {
                if (game->driver->keyPressed == 'C') {
                    cursor_color = (cursor_color & 0x8F) | ((cursor_color + 16) & 0x70);
                } else {
                    cursor_color = (cursor_color & 0xF0) | ((cursor_color + 1) & 0x0F);
                }
                UpdateCursorColor();
            } break;
            case 'D': {
                color_ignore_defaults = !color_ignore_defaults;
                UpdateColorIgnoreDefaults();
            } break;
            case 'L': {
                AskSaveChanged();
                if (game->driver->keyPressed != KeyEscape && game->GameWorldLoad(".ZZT")) {
                    bool is_secret = game->WorldGetFlagPosition("SECRET") >= 0;
                    if (game->world.info.is_save || is_secret) {
                        if (!game->debugEnabled) {
                            game->SidebarClearLine(3);
                            game->SidebarClearLine(4);
                            game->SidebarClearLine(5);

                            sstring<20> sidebar_text;
                            game->driver->draw_string(63, 4, 0x1E, "Can not edit");
                            if (game->world.info.is_save) {
                                StrCopy(sidebar_text, "a saved game!");
                            } else {
                                StrJoin(sidebar_text, 3, "  ", game->world.info.name, "!");
                            }
                            game->driver->draw_string(63, 5, 0x1E, sidebar_text);

                            game->PauseOnError();
                            game->WorldUnload();
                            game->WorldCreate();
                        }
                    }
                    was_modified = false;
                    DrawRefresh();
                }
                DrawSidebar();
            } break;
            case 'S': {
                game->GameWorldSave("Save world:", game->loadedGameFileName, sizeof(game->loadedGameFileName), ".ZZT");
                if (game->driver->keyPressed != KeyEscape) {
                    was_modified = false;
                }
                DrawSidebar();
            } break;
            case 'Z': {
                if (game->interface->SidebarPromptYesNo("Clear board? ", false)) {
                    for (int i = game->board.stats.count; i >= 1; i--) {
                        game->RemoveStat(i);
                    }
                    game->BoardCreate();
                    DrawRefresh();
                } else {
                    DrawSidebar();
                }
            } break;
            case 'N': {
                if (game->interface->SidebarPromptYesNo("Make new world? ", false) && (game->driver->keyPressed != KeyEscape)) {
                    AskSaveChanged();
                    if (game->driver->keyPressed != KeyEscape) {
                        game->WorldUnload();
                        game->WorldCreate();
                        DrawRefresh();
                        was_modified = false;
                    }
                }
                DrawSidebar();
            } break;
            case 'Q':
            case KeyEscape: {
                editor_exit_requested = true;
            } break;
            case 'B': {
                i = SelectBoard("Switch boards", game->world.info.current_board, false);
                if (game->driver->keyPressed != KeyEscape) {
                    if (i > game->world.board_count) {
                        if (game->interface->SidebarPromptYesNo("Add new board? ", false)) {
                            AppendBoard();
                        }
                    }
                    game->BoardChange(i);
                    DrawRefresh();
                }
                DrawSidebar();
            } break;
            case '?': {
                game->GameDebugPrompt();
                DrawSidebar();
            } break;
            case KeyTab: {
                draw_mode = (draw_mode == EDMDrawingOff) ? EDMDrawingOn : EDMDrawingOff;
                UpdateDrawMode();
            } break;
            case KeyF5: {
                game->driver->draw_char(cursor_x - 1, cursor_y - 1, 0x0F, 197);
                for (i = 3; i <= 20; i++) {
                    game->SidebarClearLine(i);
                }
                game->driver->draw_string(65, 4, 0x1E, "Advanced:");
                game->driver->draw_string(61, 5, 0x70, " E ");
                game->driver->draw_string(65, 5, 0x1F, "Board edge");
                game->driver->draw_char(78, 5, cursor_color, 'E');
                game->driver->read_wait_key();
                switch (UpCase(game->driver->keyPressed)) {
                    case 'E': {
                        if (PrepareModifyTile(cursor_x, cursor_y)) {
                            SetAndCopyTile(cursor_x, cursor_y, {
                                .element = EBoardEdge,
                                .color = cursor_color
                            });
                        }
                    } break;
                }
                DrawSidebar();
            } break;
            case KeyF1:
            case KeyF2:
            case KeyF3: {
                game->driver->draw_char(cursor_x - 1, cursor_y - 1, 0x0F, 197);
                for (i = 3; i <= 20; i++) {
                    game->SidebarClearLine(i);
                }
                switch (game->driver->keyPressed) {
                    case KeyF1: selected_category = EditorCategoryItem; break;
                    case KeyF2: selected_category = EditorCategoryCreature; break;
                    case KeyF3: selected_category = EditorCategoryTerrain; break;
                }
                i = 3;

                sstring<3> hotkey;
                hotkey[0] = hotkey[2] = ' ';
                hotkey[3] = 0;

                for (i_elem = 0; i_elem <= ElementCount; i_elem++) {
                    const ElementDef &def = game->elementDefs[i_elem];
                    if (def.editor_category == selected_category) {
                        if (!StrEmpty(def.category_name)) {
                            i++;
                            game->driver->draw_string(65, i, 0x1E, def.category_name);
                            i++;
                        }

                        uint8_t elem_ch = game->elementCharOverrides[i_elem];
                        if (elem_ch == 0) elem_ch = def.character;

                        hotkey[1] = def.editor_shortcut;
                        game->driver->draw_string(61, i, ((i & 1) << 6) + 0x30, hotkey);
                        game->driver->draw_string(65, i, 0x1F, def.name);
                        game->driver->draw_char(78, i, GetDrawingColor(i_elem), elem_ch);

                        i++;
                    }
                }

                game->driver->read_wait_key();

                for (i_elem = 1; i_elem <= ElementCount; i_elem++) {
                    const ElementDef &def = game->elementDefs[i_elem];
                    if (def.editor_category == selected_category
                        && def.editor_shortcut == UpCase(game->driver->keyPressed)
                    ) {
                        if (i_elem == EPlayer) {
                            if (PrepareModifyTile(cursor_x, cursor_y)) {
                                game->MoveStat(0, cursor_x, cursor_y);
                            }
                            continue;
                        }

                        elem_menu_color = GetDrawingColor(i_elem);
                        if (def.cycle == -1) {
                            if (PrepareModifyTile(cursor_x, cursor_y)) {
                                SetAndCopyTile(cursor_x, cursor_y, {
                                    .element = i_elem,
                                    .color = elem_menu_color
                                });
                            }
                        } else {
                            if (PrepareModifyStatAtCursor()) {
                                game->AddStat(cursor_x, cursor_y, i_elem, elem_menu_color, def.cycle, Stat());

                                Stat &stat = game->board.stats[game->board.stats.count];
                                if (!StrEmpty(def.p1_name)) stat.p1 = stat_settings[i_elem].p1;
                                if (!StrEmpty(def.p2_name)) stat.p2 = stat_settings[i_elem].p2;
                                if (!StrEmpty(def.param_direction_name)) {
                                    stat.step_x = stat_settings[i_elem].step_x;
                                    stat.step_y = stat_settings[i_elem].step_y;
                                }
                                if (!StrEmpty(def.param_board_name)) stat.p3 = stat_settings[i_elem].p3;
                                EditStat(game->board.stats.count);
                                if (game->driver->keyPressed == KeyEscape) {
                                    game->RemoveStat(game->board.stats.count);
                                }
                            }
                        }
                    }
                }
                DrawSidebar();
            } break;
            case KeyF4: {
                draw_mode = (draw_mode != EDMTextEntry) ? EDMTextEntry : EDMDrawingOff;
                UpdateDrawMode();
            } break;
            case 'H': {
                TextWindowDisplayFile(game->driver, game->filesystem, "editor.hlp", "World editor help");
            } break;
            case 'X': {
                FloodFill(cursor_x, cursor_y, game->board.tiles.get(cursor_x, cursor_y));
            } break;
            case '!': {
                EditHelpFile();
                DrawSidebar();
            } break;
            case 'T': {
                TransferBoard();
            } break;
            case KeyEnter: {
                i = game->board.stats.id_at(cursor_x, cursor_y);
                if (i >= 0) {
                    EditStat(i);
                    DrawSidebar();
                } else {
                    CopyPatternToCurrent(cursor_x, cursor_y);
                    UpdateCopiedPatterns();
                }
            } break;
            case 'I': {
                EditBoardInfo();
                game->TransitionDrawToBoard();
            } break;
        }

        if (editor_exit_requested) {
            AskSaveChanged();
            if (game->driver->keyPressed == KeyEscape) {
                editor_exit_requested = false;
                DrawSidebar();
            }
        }
    } while (!editor_exit_requested);

    for (int i = 0; i < COPIED_TILES_COUNT; i++) {
        copied_tiles[i].Clear();
    }

    game->driver->keyPressed = 0;
    game->InitElementsGame();
}