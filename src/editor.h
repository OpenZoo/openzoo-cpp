#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <cstdint>
#include "gamevars.h"

#define COPIED_TILES_COUNT 10

namespace ZZT {
    typedef enum {
        EDMDrawingOff,
        EDMDrawingOn,
        EDMTextEntry
    } EditorDrawMode;

    struct EditorStatSetting {
        uint8_t p1, p2, p3;
        int16_t step_x, step_y;
    };

    class EditorCopiedTile {
    public:
        Stat stat;
        bool has_stat;
        Tile tile;
        uint8_t preview_char;
        uint8_t preview_color;

        EditorCopiedTile();
        ~EditorCopiedTile();
        void Clear(void);
    };

    class Editor {
    protected:
        Game *game;

        int16_t selected_category;
        uint8_t elem_menu_color;
        bool was_modified;
        bool editor_exit_requested;
        EditorDrawMode draw_mode;
        int16_t cursor_x, cursor_y;
        uint8_t cursor_pattern, cursor_color;
        bool color_ignore_defaults;
        int16_t i, i_elem;
        bool can_modify;
        EditorCopiedTile copied_tiles[COPIED_TILES_COUNT];
        EditorStatSetting stat_settings[ElementCount];
        int16_t cursor_blinker;

        void AppendBoard(void);
        uint8_t GetDrawingColor(uint8_t element);
        void UpdateCursorColor(void);
        void UpdateCursorPattern(void);
        void UpdateDrawMode(void);
        void UpdateColorIgnoreDefaults(void);
        void UpdateCopiedPatterns(void);
        void CopyPattern(int16_t x, int16_t y, EditorCopiedTile &copied);
        void CopyPatternToCurrent(int16_t x, int16_t y);
        void DrawSidebar(void);
        void DrawTileAndNeighhborsAt(int16_t x, int16_t y);
        void DrawRefresh(void);
        void SetTile(int16_t x, int16_t y, Tile tile);
        void SetAndCopyTile(int16_t x, int16_t y, Tile tile);
        void AskSaveChanged(void);
        bool PrepareModifyTile(int16_t x, int16_t y);
        bool PrepareModifyStatAtCursor(void);
        void PlaceTile(int16_t x, int16_t y);
        void RemoveTile(int16_t x, int16_t y);
        void EditBoardInfo(void);
        void DrawTextEditSidebar(void);
        void EditStatText(int16_t stat_id, const char *prompt);
        void EditStat(int16_t stat_id);
        void TransferBoard(void);
        void FloodFill(int16_t x, int16_t y, Tile from);
        void EditHelpFile();
        void GetBoardName(int16_t board_id, bool title_screen_is_none, char *buffer, size_t buf_len);
        int SelectBoard(const char *title, int16_t current_board, bool title_screen_is_none);

    public:
        Editor(Game *game);
        ~Editor();

        void Loop(void);
    };
}

#endif