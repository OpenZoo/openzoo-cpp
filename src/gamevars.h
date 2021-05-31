#ifndef __GAMEVARS_H__
#define __GAMEVARS_H__

#include <cstddef>
#include <cstdint>
#include <cstdlib> // TODO
#include "utils/iostream.h"
#include "utils/math.h"
#include "utils/quirkset.h"
#include "utils/strings.h"
#include "utils/tokenmap.h"
#include "filesystem.h"
#include "driver.h"
#include "sounds.h"
#include "txtwind.h"
#include "high_scores.h"
#include "user_interface.h"
#include "world_serializer.h"

#define MAX_ELEMENT 80
#define MAX_FLAG 16

namespace ZZT {
    typedef enum : uint8_t {
        EditorCategoryNone,
        EditorCategoryItem,
        EditorCategoryCreature,
        EditorCategoryTerrain,
        EditorCategoryUglies, // Super ZZT
        EditorCategoryTerrain2 // Super ZZT
    } EditorCategory;

    typedef enum : uint8_t {
        ShotSourcePlayer = 0,
        ShotSourceEnemy = 1
    } ShotSource;

    typedef enum : uint8_t {
        EEmpty = 0,
        EBoardEdge = 1,
        EMessageTimer = 2,
        EMonitor = 3,
        EPlayer = 4,
        EAmmo = 5,
        ETorch = 6,
        EGem = 7,
        EKey = 8,
        EDoor = 9,
        EScroll = 10,
        EPassage = 11,
        EDuplicator = 12,
        EBomb = 13,
        EEnergizer = 14,
        EStar = 15,
        EConveyorCW = 16,
        EConveyorCCW = 17,
        EBullet = 18,
        EWater = 19, // lava in Super ZZT
        EForest = 20,
        ESolid = 21,
        ENormal = 22,
        EBreakable = 23,
        EBoulder = 24,
        ESliderNS = 25,
        ESliderEW = 26,
        EFake = 27,
        EInvisible = 28,
        EBlinkWall = 29,
        ETransporter = 30,
        ELine = 31,
        ERicochet = 32,
        EBlinkRayEw = 33,
        EBear = 34,
        ERuffian = 35,
        EObject = 36,
        ESlime = 37,
        EShark = 38,
        ESpinningGun = 39,
        EPusher = 40,
        ELion = 41,
        ETiger = 42,
        EBlinkRayNs = 43,
        ECentipedeHead = 44,
        ECentipedeSegment = 45,

        EFloor = 47,
        EWaterN = 48,
        EWaterS = 49,
        EWaterW = 50,
        EWaterE = 51,
        ERoton = 59,
        EDragonPup = 60,
        EPairer = 61,
        ESpider = 62,
        EWeb = 63,
        EStone = 64,

        EBullet_SZZT = 69,
        EBlinkRayEw_SZZT = 70,
        EBlinkRayNs_SZZT = 71,
        EStar_SZZT = 72,

        ElementTypeCount
    } ElementType;

    static constexpr const uint8_t ColorSpecialMin = 0xF0;
    static constexpr const uint8_t ColorChoiceOnBlack = 0xFF;
    static constexpr const uint8_t ColorWhiteOnChoice = 0xFE;
    static constexpr const uint8_t ColorChoiceOnChoice = 0xFD;

    struct Coord {
        uint8_t x, y;
    };

    struct Tile {
        uint8_t element;
        uint8_t color;
    };

    struct RLETile {
	    uint8_t count;
        Tile tile;
    };

    class StatData {
    public:
#ifdef ROM_POINTERS
        const char *data_rom = nullptr;
#endif
        char *data = nullptr;
        int16_t len = 0;
    
        StatData() = default;

        inline bool operator==(const StatData &b) const {
            return data == b.data;
        }

        inline bool operator!=(const StatData &b) const {
            return data != b.data;
        }

        inline void duplicate() {
            if (data != nullptr && len > 0) {
                char *new_data = (char*) malloc(len);
                memcpy(new_data, data, len);
                data = new_data;
            }
        }

        inline void clear_data() {
            data = nullptr;
            len = 0;
        }

        inline void free_data() {
            if (data != nullptr && len > 0) {
                free(data);
                clear_data();
            }
        }

        inline void alloc_data(int16_t length) {
			len = length;
			if (len > 0) {
				data = (char*) malloc(len);
			}
        }
    };

    struct Stat {
        StatData data = StatData();
        uint8_t x = 0, y = 0;
        int16_t step_x = 0, step_y = 0;
        int16_t cycle = 0;
        uint8_t p1 = 0, p2 = 0, p3 = 0;
        int16_t follower = -1, leader = -1;
        Tile under = {
            .element = 0,
            .color = 0x00
        };
        int16_t data_pos = 0;
    };

    struct BoardInfo {
        uint8_t max_shots;
        bool is_dark;
        uint8_t neighbor_boards[4];
        bool reenter_when_zapped;
        sstring<58> message;
        uint8_t start_player_x;
        uint8_t start_player_y;
        int16_t time_limit_seconds;
    };

    struct WorldInfo {
        int16_t ammo;
        int16_t gems;
        bool keys[7];
        int16_t health;
        int16_t current_board;
        int16_t torches;
        int16_t torch_ticks;
        int16_t energizer_ticks;
        int16_t score;
        sstring<20> name;
        sstring<20> flags[MAX_FLAG];
        int16_t board_time_sec;
        int16_t board_time_hsec;
        bool is_save;

        // Super ZZT
        int16_t stones_of_power;
    };

    class TileMap {
        Tile *tiles;
        Tile empty = {.element = EBoardEdge, .color = 0x00};

    public:
        uint8_t width;
        uint8_t height;
        
        TileMap(uint8_t width, uint8_t height);
        TileMap(const TileMap&&);
        ~TileMap();

        bool valid(int16_t x, int16_t y) const {
            return x >= 0 && y >= 0 && x <= (width + 1) && y <= (width + 1);
        }

        const Tile& get(int16_t x, int16_t y) const {
            return valid(x, y) ? tiles[x * (height + 2) + y] : empty;
        }

        void set(int16_t x, int16_t y, Tile tile) {
            if (valid(x, y)) {
                tiles[x * (height + 2) + y] = tile;
            }
        }

        void set_element(int16_t x, int16_t y, uint8_t element) {
            if (valid(x, y)) {
                tiles[x * (height + 2) + y].element = element;
            }
        }

        void set_color(int16_t x, int16_t y, uint8_t color) {
            if (valid(x, y)) {
                tiles[x * (height + 2) + y].color = color;
            }
        }
    };

    class StatList {
        // -1 .. size + 1 => size + 3 stats
        int16_t size;
        Stat *stats;
    
    public:
        StatList(int16_t size);
        ~StatList();

        int16_t count;

        inline int16_t stat_size() const {
            return size;
        }

        Stat& operator[](int16_t idx) {
            return stats[idx + 1];
        }

        Stat& at(int16_t x, int16_t y) {
            for (int i = 1; i <= count+1; i++) {
                if (stats[i].x == x && stats[i].y == y)
                    return stats[i];
            }
            return stats[0];
        }

        int16_t id_at(int16_t x, int16_t y) {
            for (int i = 1; i <= count+1; i++) {
                if (stats[i].x == x && stats[i].y == y)
                    return i - 1;
            }
            return -1;
        }

        bool exists(int16_t value) {
            return value >= 0 && value <= count;
        }

        bool exists_non_player(int16_t value) {
            return value >= 1 && value <= count;
        }

        bool valid(int16_t value) {
            return value >= -1 && value <= (size + 1);
        }

        bool valid_input(int16_t value) {
            return value >= 0 && value <= (size + 1);
        }

        void free_data_if_unused(int16_t stat_id) {
            if (!valid_input(stat_id)) return;
            Stat &stat = stats[stat_id + 1];
            if (stat.data.len != 0) {
                bool found = false;
                for (int i = 0; i <= count; i++) {
                    if (i == stat_id) continue;
                    if (stats[i + 1].data == stat.data) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    stat.data.clear_data();
                } else {
                    stat.data.free_data();
                }
            }
        }

        void free_all_data() {
            for (int i = 0; i <= count; i++) {
                Stat &stat = stats[i + 1];
                if (stat.data.len > 0) {
                    for (int j = i + 1; j <= count; j++) {
                        Stat &stat2 = stats[j + 1];
                        if (stat.data == stat2.data) {
                            stat2.data.clear_data();
                        }
                    }
                    stat.data.free_data();
                }
            }
        }
    };

    class Board {
    public:
        Board(uint8_t width, uint8_t height, int16_t stat_size);
        ~Board();

        sstring<60> name;
        TileMap tiles;
        StatList stats;
        BoardInfo info;

        inline int width(void) const { return tiles.width; }
        inline int height(void) const { return tiles.height; }
    };

    class EngineDefinition;

    class World {
    private:
        EngineDefinition *engine;
        uint16_t *board_len;
        uint8_t *board_format;
        int16_t max_board;
        WorldFormat format;
        bool compress_eagerly;

    public:
        // TODO: move to private (Editor::GetBoardName)
        uint8_t **board_data;
        int16_t board_count;
        WorldInfo info;

        World(WorldFormat format, EngineDefinition *engine_def, int16_t max_board, bool compress_eagerly);
        ~World();

        inline int16_t max_board_count() const {
            return max_board;
        }

        inline bool can_append_board() const {
            return board_count < max_board;
        }

        bool read_board(uint8_t id, Board &board);
        bool write_board(uint8_t id, Board &board);
        void get_board(uint8_t id, uint8_t *&data, uint16_t &len, bool &temporary, WorldFormat format);
        void set_board(uint8_t id, uint8_t *data, uint16_t len, bool costlessly_loaded, WorldFormat format);
        void free_board(uint8_t id);

        inline WorldFormat get_format(void) const { return format; }
        void set_format(WorldFormat format);
    };

    class Game;

    void ElementDefaultDraw(Game &game, int16_t x, int16_t y, uint8_t &chr);
    void ElementDefaultTick(Game &game, int16_t stat_id);
    void ElementDefaultTouch(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y);

    typedef void (*ElementDrawProc)(Game &game, int16_t x, int16_t y, uint8_t &chr);
    typedef void (*ElementTickProc)(Game &game, int16_t stat_id);
    typedef void (*ElementTouchProc)(Game &game, int16_t x, int16_t y, int16_t source_stat_id, int16_t &delta_x, int16_t &delta_y);

    struct ElementDef {
        ElementType type;
        uint8_t character = ' ';
        uint8_t color = ColorChoiceOnBlack;
        bool destructible = false;
        bool pushable = false;
        bool visible_in_dark = false;
        bool placeable_on_top = false;
        bool walkable = false;
        bool has_draw_proc = false;
        int16_t score_value = 0;
        int16_t cycle = -1;
        ElementDrawProc draw = ElementDefaultDraw;
        ElementTickProc tick = ElementDefaultTick;
        ElementTouchProc touch = ElementDefaultTouch;
        const char *name = "";
#ifndef DISABLE_EDITOR
        const char *category_name = "";
        const char *p1_name = "";
        const char *p2_name = "";
        const char *param_bullet_type_name = "";
        const char *param_board_name = "";
        const char *param_direction_name = "";
        const char *param_text_name = "";
        int16_t editor_category = 0;
        char editor_shortcut = '\0';
#else
        bool param_has_text = false;
#endif

        ElementDef() : ElementDef(EEmpty, "") { }
        ElementDef(ElementType _type, const char *_name) {
            type = _type;
            name = _name;
        }

        inline bool has_text() const {
#ifndef DISABLE_EDITOR
            return param_text_name[0] != 0;
#else
            return param_has_text;
#endif
        }

        ElementDef& with_visual(uint8_t _character, uint8_t _color) {
            character = _character;
            color = _color;
            return *this;
        }

        ElementDef& with_destructible(void) {
            destructible = true;
            return *this;
        }

        ElementDef& with_destructible(int16_t _score_value) {
            destructible = true;
            score_value = _score_value;
            return *this;
        }

        ElementDef& with_pushable(void) {
            pushable = true;
            return *this;
        }

        ElementDef& with_visible_in_dark(void) {
            visible_in_dark = true;
            return *this;
        }

        ElementDef& with_placeable_on_top(void) {
            placeable_on_top = true;
            return *this;
        }

        ElementDef& with_walkable(void) {
            walkable = true;
            return *this;
        }

        ElementDef& with_drawable(ElementDrawProc _draw) {
            has_draw_proc = true;
            draw = _draw;
            return *this;
        }

        ElementDef& with_tickless_stat() {
            cycle = 0;
            return *this;
        }

        ElementDef& with_tickable_stat(int16_t _cycle, ElementTickProc _tick) {
            cycle = _cycle;
            tick = _tick;
            return *this;
        }

        ElementDef& with_touchable(ElementTouchProc _touch) {
            touch = _touch;
            return *this;
        }

        ElementDef& with_editor_category(int16_t _editor_category, char _editor_shortcut) {
#ifndef DISABLE_EDITOR
            editor_category = _editor_category;
            editor_shortcut = _editor_shortcut;
#endif
            return *this;
        }

        ElementDef& with_editor_category(int16_t _editor_category, char _editor_shortcut, const char *_category_name) {
#ifndef DISABLE_EDITOR
            editor_category = _editor_category;
            editor_shortcut = _editor_shortcut;
            category_name = _category_name;
#endif
            return *this;
        }

        ElementDef& with_p1_slider(const char *name) {
#ifndef DISABLE_EDITOR
            p1_name = name;
#endif
            return *this;
        }

        ElementDef& with_p2_slider(const char *name) {
#ifndef DISABLE_EDITOR
            p2_name = name;
#endif
            return *this;
        }

        ElementDef& with_p2_bullet_type(const char *name) {
#ifndef DISABLE_EDITOR
            param_bullet_type_name = name;
#endif
            return *this;
        }

        ElementDef& with_p3_board(const char *name) {
#ifndef DISABLE_EDITOR
            param_board_name = name;
#endif
            return *this;
        }

        ElementDef& with_step_direction(const char *name) {
#ifndef DISABLE_EDITOR
            param_direction_name = name;
#endif
            return *this;
        }

        ElementDef& with_stat_text(const char *name) {
#ifndef DISABLE_EDITOR
            param_text_name = name;
#else
            param_has_text = true;
#endif
            return *this;
        }
    };
    
    enum MessageFlag {
        MESSAGE_AMMO,
        MESSAGE_OUT_OF_AMMO,
        MESSAGE_NO_SHOOTING,
        MESSAGE_TORCH,
        MESSAGE_OUT_OF_TORCHES,
        MESSAGE_ROOM_NOT_DARK,
        MESSAGE_HINT_TORCH,
        MESSAGE_FOREST,
        MESSAGE_FAKE,
        MESSAGE_GEM,
        MESSAGE_ENERGIZER,
        MessageFlagCount
    };

    void ElementMove(Game &game, int16_t old_x, int16_t old_y, int16_t new_x, int16_t new_y);
    void ElementPushablePush(Game &game, int16_t x, int16_t y, int16_t delta_x, int16_t delta_y);

    enum EngineType {
        ENGINE_TYPE_INVALID,
        ENGINE_TYPE_ZZT,
        ENGINE_TYPE_SUPER_ZZT
    };

    enum EngineQuirk {
        QUIRK_BULLET_DRAWTILE_FIX, // Super ZZT
        QUIRK_BOARD_EDGE_TOUCH_DESINATION_FIX, // Super ZZT
        QUIRK_CENTIPEDE_EXTRA_CHECKS, // Super ZZT
        QUIRK_CONNECTION_DRAWING_CHECKS_UNDER_STAT, // Super ZZT
        QUIRK_OOP_LENIENT_COLOR_MATCHES, // Super ZZT
        QUIRK_OOP_SUPER_ZZT_MOVEMENT, // Super ZZT - also moves locking to P3
        QUIRK_BOARD_CHANGE_SENDS_ENTER, // Super ZZT
        QUIRK_PLAYER_BGCOLOR_FROM_FLOOR, // Super ZZT
        QUIRK_PLAYER_AFFECTED_BY_WATER, // Super ZZT
        QUIRK_SUPER_ZZT_FOREST_SOUND, // Super ZZT
        QUIRK_SUPER_ZZT_STONES_OF_POWER, // Super ZZT - affects OOP #GIVE/#TAKE
        QUIRK_SUPER_ZZT_COMPAT_MISC, // Super ZZT - assorted
        EngineQuirkCount  
    };

    class Viewport {
    public:
        int16_t cx_offset, cy_offset, x, y, width, height;

        Viewport(int16_t _x, int16_t _y, int16_t _width, int16_t _height);

        bool set(int16_t nx, int16_t ny);
        bool point_at(Board& board, int16_t sx, int16_t sy);
        bool point_at(Board& board, Stat& stat) {
            return point_at(board, stat.x, stat.y);
        }
    };

    class EngineDefinition {
        uint8_t elementTypeToId[ElementTypeCount];

    public:
        EngineType engineType;
        QuirkSet<EngineQuirk, EngineQuirkCount> quirks;
        ElementDef elementDefs[MAX_ELEMENT];
        uint8_t elementCount;
        uint8_t textCutoff;
        int16_t torchDuration, torchDistSqr;
        int16_t torchDx, torchDy;
        int16_t boardWidth, boardHeight, statCount;

		EngineDefinition() {
			engineType = ENGINE_TYPE_INVALID;
		}

        // Caches
        TokenMap<int16_t, -1, true> elementNameMap;

        uint8_t get_element_id(ElementType type) {
            return elementTypeToId[type];
        }

        void clear_elements(void) {
            for (int i = 0; i < MAX_ELEMENT; i++) {
                elementDefs[i] = ElementDef();
            }
            memset(elementTypeToId, 0, sizeof(elementTypeToId));
            elementCount = 1;
        }

        void mark_element_used(uint8_t id) {
            if (elementCount <= id) {
                elementCount = id + 1;
            }
        }

        void register_element(uint8_t id, ElementDef def) {
            elementTypeToId[def.type] = id;
            elementDefs[id] = def;
            mark_element_used(id);
        }

        template<EngineQuirk quirk>
        inline const bool is() const {
            return quirks.is<quirk>();
        }

        template<EngineQuirk quirk>
        inline const bool isNot() const {
            return quirks.isNot<quirk>();
        }

        inline const ElementDef& elementDef(uint8_t element) const {
            return elementDefs[element >= elementCount ? EEmpty : element];
        }
    };

    class OopState {
    public:
        Game *game;
        Stat *stat;

        TextWindow *textWindow;
        bool stopRunning;
        bool repeatInsNextTick;
        bool replaceStat;
        bool endOfProgram;
        bool lineFinished;
        int16_t insCount;
        int16_t lastPosition;
        Tile replaceTile;
    };

    class Game {
    private:
        bool initialized;

        // oop.cpp
        uint8_t GetColorForTileMatch(const Tile &tile);

    public:
        Random random;

        int16_t playerDirX;
        int16_t playerDirY;

        sstring<50> loadedGameFileName;
        sstring<50> savedGameFileName;
        sstring<50> savedBoardFileName;
        sstring<50> startupWorldFileName;

        Board board;
        World world;
        QuirkSet<MessageFlag, MessageFlagCount> msgFlags;

        bool gameTitleExitRequested;
        bool gamePlayExitRequested;
        uint8_t gameStateElement;
        int16_t returnBoardId;

        uint8_t tickSpeed;

        EngineDefinition engineDefinition;
        Viewport viewport;

        int16_t editorPatternCount;
        uint8_t editorPatterns[10];

        int16_t tickTimeDuration;
        int16_t currentTick;
        int16_t currentStatTicked;
        bool gamePaused;
        int16_t tickTimeCounter;

        bool forceDarknessOff;
        uint8_t initialTextAttr;

        char oopChar;
        sstring<20> oopWord;
        int16_t oopValue;

        bool debugEnabled;

        HighScoreList highScoreList;
        sstring<255> configRegistration;
        sstring<50> configWorldFile;
// TODO: expose as configure flag
#ifdef DISABLE_EDITOR
        constexpr static bool editorEnabled = false;
#else
        bool editorEnabled;
#endif
        sstring<50> gameVersion;
        bool parsingConfigFile;
        bool justStarted;

        Driver *driver;
        FilesystemDriver *filesystem;

        UserInterface *interface;

        inline const ElementDef& elementDef(uint8_t element) const {
            return engineDefinition.elementDef(element);
        }

        inline const ElementDef& elementDefAt(int16_t x, int16_t y) const {
            uint8_t element = board.tiles.get(x, y).element;
            return elementDef(element);
        }

        inline uint8_t elementId(ElementType type) {
            return engineDefinition.get_element_id(type);
        }

        inline ElementType elementType(uint8_t id) {
            return engineDefinition.elementDef(id).type;
        }

        Game(void);
        ~Game();

        // elements.cpp
        void DrawPlayerSurroundings(int16_t x, int16_t y, int16_t bomb_phase);
        void GamePromptEndPlay(void);
        void ResetMessageNotShownFlags(void);
        void InitEngine(EngineType type, bool is_editor);

        // game.cpp
        void Initialize(void);
        void SidebarClearLine(int y);
        void SidebarClear();
        void BoardClose();
        void BoardOpen(int16_t board_id);
        void BoardChange(int16_t board_id);
        void BoardCreate(void);
        void WorldCreate(void);
        void TransitionDrawToFill(uint8_t chr, uint8_t color);
        void BoardDrawTile(int16_t x, int16_t y);
        void BoardDrawChar(int16_t x, int16_t y, uint8_t drawn_color, uint8_t drawn_char);
        bool BoardUpdateDrawOffset(void);
        void BoardPointCameraAt(int16_t sx, int16_t sy);
        void BoardDrawBorder(void);
        void TransitionDrawToBoard(void);
        void SidebarPromptCharacter(bool editable, int16_t x, int16_t y, const char *prompt, uint8_t &value);
        void SidebarPromptSlider(bool editable, int16_t x, int16_t y,  const char *prompt, uint8_t &value);
        void SidebarPromptChoice(bool editable, int16_t y, const char *prompt, const char *choiceStr, uint8_t &result);
        void SidebarPromptDirection(bool editable, int16_t y, const char *prompt, int16_t &dx, int16_t &dy);
        void PauseOnError(void);
        void DisplayIOError(IOStream &stream);
        void WorldUnload(void);
        bool WorldLoad(const char *filename, const char *extension, bool titleOnly, bool showError = true);
        bool WorldSave(const char *filename, const char *extension);
        void GameWorldSave(const char *prompt, char* filename, size_t filename_len, const char *extension);
        bool GameWorldLoad(const char *extension);
        void AddStat(int16_t x, int16_t y, uint8_t element, uint8_t color, int16_t cycle, Stat tpl);
        void RemoveStat(int16_t stat_id);
        bool BoardPrepareTileForPlacement(int16_t x, int16_t y);
        void MoveStat(int16_t stat_id, int16_t newX, int16_t newY, bool scrollOffset = true);
        void GameDrawSidebar(void);
        void GameUpdateSidebar(void);
        void DisplayMessage(int16_t time, const char *message);
        void DamageStat(int16_t attacker_stat_id);
        void BoardDamageTile(int16_t x, int16_t y);
        void BoardAttack(int16_t attacker_stat_id, int16_t x, int16_t y);
        bool BoardShoot(ElementType element, int16_t x, int16_t y, int16_t dx, int16_t dy, int16_t source);
        void CalcDirectionRnd(int16_t &deltaX, int16_t &deltaY);
        void CalcDirectionSeek(int16_t x, int16_t y, int16_t &deltaX, int16_t &deltaY);
        void TransitionDrawBoardChange(void);
        void BoardEnter(void);
        void BoardPassageTeleport(int16_t x, int16_t y);
        void GameDebugPrompt(void);
        void GameAboutScreen(void);
        void GamePlayLoop(bool boardChanged);
        void GameTitleLoop(void);

        // oop.cpp
        void OopError(Stat& stat, const char *message);
        void OopReadChar(Stat& stat, int16_t& position);
        void OopReadWord(Stat& stat, int16_t& position);
        void OopReadValue(Stat& stat, int16_t& position);
        void OopSkipLine(Stat& stat, int16_t& position);
        bool OopParseDirection(Stat& stat, int16_t& position, int16_t& dx, int16_t& dy);
        void OopReadDirection(Stat& stat, int16_t& position, int16_t& dx, int16_t& dy);
        inline int16_t OopFindString(Stat& stat, char *str) {
            return OopFindString(stat, 0, str);
        }
        int16_t OopFindString(Stat& stat, int16_t startPos, char *str);
        bool OopIterateStat(int16_t stat_id, int16_t &i_stat, const char *lookup);
        bool OopFindLabel(int16_t stat_id, const char *sendLabel, int16_t &i_stat, int16_t &i_data_pos, const char *labelPrefix);
        int16_t WorldGetFlagPosition(const char *name);
        void WorldSetFlag(const char *name);
        void WorldClearFlag(const char *name);
        void OopStringToWord(const char *input, char *buf, size_t len);
        bool OopParseTile(Stat &stat, int16_t &position, Tile &tile);
        bool FindTileOnBoard(int16_t &x, int16_t &y, Tile tile);
        void OopPlaceTile(int16_t x, int16_t y, Tile tile);
        bool OopCheckCondition(Stat &stat, int16_t &position);
        void OopReadLineToEnd(Stat &stat, int16_t &position, char *buf, size_t len);
        bool OopSend(int16_t stat_id, const char *sendLabel, bool ignoreLock);
        bool OopExecute(int16_t stat_id, int16_t &position, const char *name);

        // sounds.cpp
        bool HasTimeElapsed(int16_t &counter, int16_t duration);
    };

    // game.cpp
    void CopyStatDataToTextWindow(const Stat &stat, TextWindow &window);

    extern const char ColorNames[7][8];
    extern const int16_t DiagonalDeltaX[8];
    extern const int16_t DiagonalDeltaY[8];
    extern const int16_t NeighborDeltaX[4];
    extern const int16_t NeighborDeltaY[4];
    extern const uint8_t LineChars[16];
    extern const uint8_t WebChars[16];

    extern const MenuEntry TitleMenu[];
    extern const MenuEntry PlayMenu[];
}

#endif
