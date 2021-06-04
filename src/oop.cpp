#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "gamevars.h"
#include "txtwind.h"
#include "platform_hacks.h"

using namespace ZZT;

static inline uint8_t& stat_lock(Stat& stat, Game& game) {
	return game.engineDefinition.is<QUIRK_OOP_SUPER_ZZT_MOVEMENT>() ? stat.p3 : stat.p2;
}

void Game::OopError(Stat& stat, const char *message) {
	char text[256];
	StrJoin(text, 2, "ERR: ", message);
	DisplayMessage(200, text);
	driver->sound_queue(5, "\x50\x0A");
	stat.data_pos = -1;
}

#define OOP_READ_CHAR_SIMPLE
void Game::OopReadChar(Stat& stat, int16_t& position) {
	if (position >= 0 && position < stat.data.len) {
		oopChar = stat.data.data[position++];
	} else {
		oopChar = 0;
	}
}

void Game::OopReadWord(Stat& stat, int16_t& position) {
	int pos = 0;
	int len = StrSize(oopWord);

	do {
		OopReadChar(stat, position);
	} while (oopChar == ' ');

	oopChar = UpCase(oopChar);
	if (oopChar < '0' || oopChar > '9') {
		while (
			(oopChar >= 'A' && oopChar <= 'Z')
			|| (oopChar == ':')
			|| (oopChar >= '0' && oopChar <= '9')
			|| (oopChar == '_')
		) {
			if (pos < len) {
				oopWord[pos++] = oopChar;
			}

			OopReadChar(stat, position);
			oopChar = UpCase(oopChar);
		}
	}
	oopWord[pos] = 0;

	if (position > 0) {
		position--;
	}
}

void Game::OopReadValue(Stat& stat, int16_t& position) {
	sstring<20> word;
	int pos = 0;
	int len = StrSize(word);

	do {
		OopReadChar(stat, position);
	} while (oopChar == ' ');

	while (oopChar >= '0' && oopChar <= '9') {
		if (pos < len) {
			word[pos++] = oopChar;
		}
		OopReadChar(stat, position);
	}
	word[pos] = 0;

	if (position > 0) {
		position--;
	}

	oopValue = (pos > 0) ? atoi(word) : -1;
}

void Game::OopSkipLine(Stat& stat, int16_t& position) {
	do {
		OopReadChar(stat, position);
	} while (oopChar != 0 && oopChar != '\r');
}

bool Game::OopParseDirection(Stat& stat, int16_t& position, int16_t& dx, int16_t& dy) {
	if (StrEquals(oopWord, "N") || StrEquals(oopWord, "NORTH")) {
		dx = 0;
		dy = -1;
	} else if (StrEquals(oopWord, "S") || StrEquals(oopWord, "SOUTH")) {
		dx = 0;
		dy = 1;
	} else if (StrEquals(oopWord, "E") || StrEquals(oopWord, "EAST")) {
		dx = 1;
		dy = 0;
	} else if (StrEquals(oopWord, "W") || StrEquals(oopWord, "WEST")) {
		dx = -1;
		dy = 0;
	} else if (StrEquals(oopWord, "I") || StrEquals(oopWord, "IDLE")) {
		dx = 0;
		dy = 0;
	} else if (StrEquals(oopWord, "SEEK")) {
		CalcDirectionSeek(stat.x, stat.y, dx, dy);
	} else if (StrEquals(oopWord, "FLOW")) {
		dx = stat.step_x;
		dy = stat.step_y;
	} else if (StrEquals(oopWord, "RND")) {
		CalcDirectionRnd(dx, dy);
	} else if (StrEquals(oopWord, "RNDNS")) {
		dx = 0;
		dy = random.Next(2) * 2 - 1;
	} else if (StrEquals(oopWord, "RNDNE")) {
		dx = random.Next(2);
		dy = (dx == 0) ? -1 : 0;
	} else if (StrEquals(oopWord, "CW")) {
		OopReadWord(stat, position);
		bool result = OopParseDirection(stat, position, dy, dx);
		dx = -dx;
		return result;
	} else if (StrEquals(oopWord, "CCW")) {
		OopReadWord(stat, position);
		bool result = OopParseDirection(stat, position, dy, dx);
		dy = -dy;
		return result;
	} else if (StrEquals(oopWord, "RNDP")) {
		OopReadWord(stat, position);
		bool result = OopParseDirection(stat, position, dy, dx);
		if (random.Next(2) == 0) dx = -dx; else dy = -dy;
		return result;
	} else if (StrEquals(oopWord, "OPP")) {
		OopReadWord(stat, position);
		bool result = OopParseDirection(stat, position, dx, dy);
		dx = -dx;
		dy = -dy;
		return result;
	} else {
		dx = 0;
		dy = 0;
		return false;
	}

	return true;
}

void Game::OopReadDirection(Stat& stat, int16_t& position, int16_t& dx, int16_t& dy) {
	OopReadWord(stat, position);
	if (!OopParseDirection(stat, position, dx, dy)) {
		OopError(stat, "Bad direction");
	}
}

GBA_CODE_IWRAM
int16_t Game::OopFindString(Stat& stat, int16_t start_pos, char *str) {
	size_t str_len = strlen(str);

	for (size_t i = 0; i < str_len; i++) {
		str[i] = UpCase(str[i]);
	}

#ifdef LABEL_CACHE
	if (str[0] == '\r' && (str[1] == '\'' || str[1] == ':')) {
		stat.data.build_label_cache();
		
		int16_t pos = 0;

		while (pos < stat.data.label_cache_len) {
			size_t word_pos = 0;
			int16_t cmp_pos = stat.data.label_cache[pos];
			if (cmp_pos < start_pos) { pos++; continue; }
			
			do {
				OopReadChar(stat, cmp_pos);
				if (str[word_pos] != UpCase(oopChar)) {
					goto NoMatchLbl;
				}
				word_pos++;
			} while (word_pos < str_len);

			// string matches
			OopReadChar(stat, cmp_pos);
			oopChar = UpCase(oopChar);

			if ((oopChar >= 'A' && oopChar <= 'Z') || (oopChar == '_')) {
				// word continues, match invalid
			} else {
				// word complete, match valid
				return stat.data.label_cache[pos];
			}
	NoMatchLbl:
			pos++;
		}

		return -1;
	}
#endif

	int16_t pos = start_pos;
	int16_t max_pos = stat.data.len - str_len;

	while (pos <= max_pos) {
		size_t word_pos = 0;
		int16_t cmp_pos = pos;
		do {
#ifdef OOP_READ_CHAR_SIMPLE
			oopChar = stat.data.data[cmp_pos++];
#else
			OopReadChar(stat, cmp_pos);
#endif
			if (str[word_pos] != UpCase(oopChar)) {
				goto NoMatch;
			}
			word_pos++;
		} while (word_pos < str_len);

		// string matches
		OopReadChar(stat, cmp_pos);
		oopChar = UpCase(oopChar);

		if ((oopChar >= 'A' && oopChar <= 'Z') || (oopChar == '_')) {
			// word continues, match invalid
		} else {
			// word complete, match valid
			return pos;
		}
NoMatch:
		pos++;
	}

	return -1;
}

bool Game::OopIterateStat(int16_t stat_id, int16_t &i_stat, const char *lookup) {
	i_stat++;

	if (StrEquals(lookup, "ALL")) {
		return i_stat <= board.stats.count;
	} else if (StrEquals(lookup, "OTHERS")) {
		if (i_stat <= board.stats.count) {
			if (i_stat != stat_id) {
				return true;
			} else {
				i_stat++;
				return i_stat <= board.stats.count;
			}
		}
	} else if (StrEquals(lookup, "SELF")) {
		if (i_stat > 0 && i_stat <= stat_id) {
			i_stat = stat_id;
			return true;
		}
	} else {
		while (i_stat <= board.stats.count) {
			Stat &stat = board.stats[i_stat];
			if (stat.data.len > 0) {
				int16_t pos = 0;
				OopReadChar(stat, pos);
				if (oopChar == '@') {
					OopReadWord(stat, pos);
					if (StrEquals(oopWord, lookup)) {
						return true;
					}
				}
			}

			i_stat++;
		}
	}

	return false;
}

bool Game::OopFindLabel(int16_t stat_id, const char *send_label, int16_t &i_stat, int16_t &i_data_pos, const char *labelPrefix) {
	size_t i;
	const char *target_split_pos;
	// NOTE: In regular ZZT, this is limited to 20 characters.
	// However, as OOP words are *also* limited to 20 characters,
	// this should hopefully not pose a practical problem.
	const char *object_message;
	char target_lookup[21];
	char prefixed_object_message[21+2];
	bool built_pom = false;
	bool found_stat = false;

	target_split_pos = strchr(send_label, ':');
	if (target_split_pos == NULL) {
		// if there is no target, we only check stat_id
		if (i_stat < stat_id) {
			object_message = send_label;
			i_stat = stat_id;
			found_stat = true;
		}
	} else {
		i = target_split_pos - send_label;
		if (i >= sizeof(target_lookup)) i = (sizeof(target_lookup) - 1);
		memcpy(target_lookup, send_label, i);
		target_lookup[i] = 0;

		object_message = target_split_pos + 1;
FindNextStat:
		found_stat = OopIterateStat(stat_id, i_stat, target_lookup);
	}

	if (found_stat) {
		if (StrEquals(object_message, "RESTART")) {
			i_data_pos = 0;
		} else {
			if (!built_pom) {
				StrJoin(prefixed_object_message, 2, labelPrefix, object_message);
				built_pom = true;
			}

			i_data_pos = OopFindString(board.stats[i_stat], prefixed_object_message);

			// if lookup target exists, there may be more stats
			if ((i_data_pos < 0) && (target_split_pos != NULL)) {
				goto FindNextStat;
			}
		}
		found_stat = i_data_pos >= 0;
	}

	return found_stat;
}

int16_t Game::WorldGetFlagPosition(const char *name) {
	for (int i = 0; i < engineDefinition.flagCount; i++) {
		if (StrEquals(world.info.flags[i], name)) {
			return i;
		}
	}
	return -1;
}

void Game::WorldSetFlag(const char *name) {
	if (WorldGetFlagPosition(name) < 0) {
		for (int i = 0; i < engineDefinition.flagCount; i++) {
			if (StrEmpty(world.info.flags[i])) {
				StrCopy(world.info.flags[i], name);
				return;
			}
		}
	}
}

void Game::WorldClearFlag(const char *name) {
	int16_t pos = WorldGetFlagPosition(name);
	if (pos >= 0) {
		StrClear(world.info.flags[pos]);
	}
}

void Game::OopStringToWord(const char *input, char *buf, size_t len) {
	size_t pos = 0;
	while (*input != 0) {
		char ch = *(input++);
		if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
			buf[pos++] = ch;
			if (pos >= (len - 1)) break;
		} else if ((ch >= 'a' && ch <= 'z')) {
			buf[pos++] = ch - 0x20;
			if (pos >= (len - 1)) break;
		}
	}
	buf[pos] = 0;
}

bool Game::OopParseTile(Stat &stat, int16_t &position, Tile &tile) {
	char compared[21];

	tile.color = 0;
	OopReadWord(stat, position);
	for (int i = 0; i < 7; i++) {
		OopStringToWord(ColorNames[i], compared, sizeof(compared));
		if (StrEquals(oopWord, compared)) {
			tile.color = 9 + i;
			OopReadWord(stat, position);
			break;
		}
	}

	auto element = engineDefinition.elementNameMap.get(oopWord);
	if (element >= 0) {
		tile.element = element;
		return true;
	}

	return false;
}

uint8_t Game::GetColorForTileMatch(const Tile &tile) {
	uint8_t def_color = elementDef(tile.element).color;
	if (def_color < ColorSpecialMin) {
		return def_color & 0x07;
	} else if (def_color == ColorWhiteOnChoice) {
		return ((tile.color >> 4) & 0x0F) + 8;
	} else {
		return tile.color & 0x0F;
	}
}

GBA_CODE_IWRAM
bool Game::FindTileOnBoard(int16_t &x, int16_t &y, Tile tile) {	
	bool lenient = engineDefinition.is<QUIRK_OOP_LENIENT_COLOR_MATCHES>();

	while (true) {
		x++;
		if (x > board.width()) {
			x = 1;
			y++;
			if (y > board.height()) {
				return false;
			}
		}

		Tile found = board.tiles.get(x, y);
		if (found.element == tile.element) {
			if (!lenient) {
				if (tile.color == 0 || GetColorForTileMatch(found) == tile.color) {
					return true;
				}
			} else {
				if (tile.color == 0 || (GetColorForTileMatch(found) & 0x07) == (tile.color & 0x07)) {
					return true;
				}
			}
		}
	}
}

GBA_CODE_IWRAM // for #CHANGE
void Game::OopPlaceTile(int16_t x, int16_t y, Tile tile) {
	Tile curTile = board.tiles.get(x, y);
	if (curTile.element != EPlayer) {
		uint8_t color = tile.color;
		if (elementDef(tile.element).color < ColorSpecialMin) {
			color = elementDef(tile.element).color;
		} else {
			if (color == 0) color = curTile.color;
			if (color == 0) color = 0x0F;
			if (elementDef(tile.element).color == ColorWhiteOnChoice)
				color = ((color - 8) << 4) + 0x0F;
		}

		if (curTile.element == tile.element) {
			board.tiles.set_color(x, y, color);
		} else {
			BoardDamageTile(x, y);
			if (elementDef(tile.element).cycle >= 0) {
				AddStat(x, y, tile.element, color, elementDef(tile.element).cycle, Stat());
			} else {
				board.tiles.set(x, y, {
					.element = tile.element,
					.color = color
				});
			}
		}

		BoardDrawTile(x, y);
	}
}

bool Game::OopCheckCondition(Stat &stat, int16_t &position) {
	Stat &player = board.stats[0];

	if (StrEquals(oopWord, "NOT")) {
		OopReadWord(stat, position);
		return !OopCheckCondition(stat, position);
	} else if (StrEquals(oopWord, "ALLIGNED")) {
		return (player.x == stat.x) || (player.y == stat.y);
	} else if (StrEquals(oopWord, "CONTACT")) {
		return (Sqr(player.x - stat.x) + Sqr(player.y - stat.y)) == 1;
	} else if (StrEquals(oopWord, "BLOCKED")) {
		int16_t delta_x, delta_y;
		OopReadDirection(stat, position, delta_x, delta_y);
		return !elementDefAt(stat.x + delta_x, stat.y + delta_y).walkable;
	} else if (StrEquals(oopWord, "ENERGIZED")) {
		return world.info.energizer_ticks > 0;
	} else if (StrEquals(oopWord, "ANY")) {
		Tile tile;
		if (!OopParseTile(stat, position, tile)) {
			OopError(stat, "Bad object kind");
		}
		
		int16_t ix = 0;
		int16_t iy = 1;
		return FindTileOnBoard(ix, iy, tile);
	} else {
		return WorldGetFlagPosition(oopWord) >= 0;
	}
}

void Game::OopReadLineToEnd(Stat &stat, int16_t &position, char *buf, size_t len) {
	size_t pos = 0;
	OopReadChar(stat, position);
	while (oopChar != 0 && oopChar != '\r') {
		if (pos < (len - 1))
			buf[pos++] = oopChar;
		OopReadChar(stat, position);
	}
	buf[pos] = 0;
}

bool Game::OopSend(int16_t stat_id, const char *sendLabel, bool ignoreLock) {
	bool ignoreSelfLock = false;
	if (stat_id < 0) {
		stat_id = -stat_id;
		ignoreSelfLock = true;
	}

	bool result = false;
	int16_t i_stat_id = 0;
	int16_t i_data_pos = 0;

	while (OopFindLabel(stat_id, sendLabel, i_stat_id, i_data_pos, "\r:")) {
		Stat &i_stat = board.stats[i_stat_id];
		if ((stat_lock(i_stat, *this) == 0) || ignoreLock || ((stat_id == i_stat_id) && !ignoreSelfLock)) {
			if (i_stat_id == stat_id) {
				result = true;
			}

			i_stat.data_pos = i_data_pos;
			if (engineDefinition.is<QUIRK_OOP_SUPER_ZZT_MOVEMENT>()) {
				i_stat.p2 = 0;
			}
		}
	}

	return result;
}

OopCommandResult ZZT::OopCommandGo(OopState &state) {
	int16_t deltaX, deltaY;
	state.game.OopReadDirection(state.stat, state.position, deltaX, deltaY);
	int16_t destX = state.stat.x + deltaX;
	int16_t destY = state.stat.y + deltaY;

	if (!state.game.elementDefAt(destX, destY).walkable) {
		ElementPushablePush(state.game, destX, destY, deltaX, deltaY);
	}

	destX = state.stat.x + deltaX;
	destY = state.stat.y + deltaY;

	if (state.game.elementDefAt(destX, destY).walkable) {
		state.game.MoveStat(state.stat_id, destX, destY);
	} else {
		state.repeatInsNextTick = true;
	}

	state.stopRunning = true;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandTry(OopState &state) {
	int16_t deltaX, deltaY;
	state.game.OopReadDirection(state.stat, state.position, deltaX, deltaY);
	int16_t destX = state.stat.x + deltaX;
	int16_t destY = state.stat.y + deltaY;

	if (!state.game.elementDefAt(destX, destY).walkable) {
		ElementPushablePush(state.game, destX, destY, deltaX, deltaY);
	}

	destX = state.stat.x + deltaX;
	destY = state.stat.y + deltaY;

	if (state.game.elementDefAt(destX, destY).walkable) {
		state.game.MoveStat(state.stat_id, destX, destY);
		state.stopRunning = true;
		return OOP_COMMAND_FINISHED;
	} else {
		return OOP_COMMAND_NEXT;
	}
}

OopCommandResult ZZT::OopCommandWalk(OopState &state) {
	int16_t newStepX = state.stat.step_x;
	int16_t newStepY = state.stat.step_y;
	state.game.OopReadDirection(state.stat, state.position, newStepX, newStepY);
	state.stat.step_x = newStepX;
	state.stat.step_y = newStepY;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandSet(OopState &state) {
	state.game.OopReadWord(state.stat, state.position);
	state.game.WorldSetFlag(state.game.oopWord);
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandClear(OopState &state) {
	state.game.OopReadWord(state.stat, state.position);
	state.game.WorldClearFlag(state.game.oopWord);
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandIf(OopState &state) {
	state.game.OopReadWord(state.stat, state.position);
	if (state.game.OopCheckCondition(state.stat, state.position)) {
		return OOP_COMMAND_NEXT;
	} else {
		return OOP_COMMAND_FINISHED;
	}
}

OopCommandResult ZZT::OopCommandShoot(OopState &state) {
	int16_t deltaX, deltaY;
	state.game.OopReadDirection(state.stat, state.position, deltaX, deltaY);
	if (state.game.BoardShoot(EBullet, state.stat.x, state.stat.y, deltaX, deltaY, ShotSourceEnemy)) {
		state.game.driver->sound_queue(2, "\x30\x01\x26\x01");
	}
	state.stopRunning = true;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandThrowstar(OopState &state) {
	int16_t deltaX, deltaY;
	state.game.OopReadDirection(state.stat, state.position, deltaX, deltaY);
	if (state.game.BoardShoot(EStar, state.stat.x, state.stat.y, deltaX, deltaY, ShotSourceEnemy)) {
		// pass
	}
	state.stopRunning = true;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandGiveTake(OopState &state) {
	bool counterSubtract = state.game.oopWord[0] == 'T';
	
	state.game.OopReadWord(state.stat, state.position);
	int16_t *counterPtr = nullptr;
	if (StrEquals(state.game.oopWord, "HEALTH")) {
		counterPtr = &state.game.world.info.health;
	} else if (StrEquals(state.game.oopWord, "AMMO")) {
		counterPtr = &state.game.world.info.ammo;
	} else if (StrEquals(state.game.oopWord, "GEMS")) {
		counterPtr = &state.game.world.info.gems;
	} else if (StrEquals(state.game.oopWord, "TORCHES")) {
		counterPtr = &state.game.world.info.torches;
	} else if (StrEquals(state.game.oopWord, "SCORE")) {
		counterPtr = &state.game.world.info.score;
	} else if (StrEquals(state.game.oopWord, "TIME")) {
		counterPtr = &state.game.world.info.board_time_sec;
	} else if (state.game.engineDefinition.is<QUIRK_SUPER_ZZT_STONES_OF_POWER>() && StrEquals(state.game.oopWord, "Z")) {
		counterPtr = &state.game.world.info.stones_of_power;
	}

	if (counterPtr != nullptr) {
		state.game.OopReadValue(state.stat, state.position);
		if (state.game.oopValue > 0) {
			if (counterSubtract) {
				state.game.oopValue = -state.game.oopValue;
			}

			if (((*counterPtr) + state.game.oopValue) >= 0) {
				*counterPtr += state.game.oopValue;
			} else {
				return OOP_COMMAND_NEXT;
			}
		}
	}

	state.game.GameUpdateSidebar();
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandEnd(OopState &state) {
	state.position = -1;
	state.game.oopChar = 0;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandEndgame(OopState &state) {
	state.game.world.info.health = 0;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandIdle(OopState &state) {
	state.stopRunning = true;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandRestart(OopState &state) {
	state.position = 0;
	state.lineFinished = false;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandZap(OopState &state) {
	state.game.OopReadWord(state.stat, state.position);
	// state->oop_word is used by OopIterateStat
	sstring<20> oopWordCopy;
	StrCopy(oopWordCopy, state.game.oopWord);

	int16_t labelStatId = 0;
	int16_t labelDataPos;
	while (state.game.OopFindLabel(state.stat_id, oopWordCopy, labelStatId, labelDataPos, "\r:")) {
		state.game.board.stats[labelStatId].data.data[labelDataPos + 1] = '\'';
	}
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandRestore(OopState &state) {
	state.game.OopReadWord(state.stat, state.position);
	// state->oop_word is used by OopIterateStat
	sstring<20> oopWordCopy;
	sstring<30> oopSearchStr;
	StrCopy(oopWordCopy, state.game.oopWord);
	StrJoin(oopSearchStr, 3, "\r'", state.game.oopWord, "\r");

	int16_t labelStatId = 0;
	int16_t labelDataPos;
	while (state.game.OopFindLabel(state.stat_id, oopWordCopy, labelStatId, labelDataPos, "\r'")) {
		Stat &labelStat = state.game.board.stats[labelStatId];

		do {
			labelStat.data.data[labelDataPos + 1] = ':';
			labelDataPos = state.game.OopFindString(labelStat, labelDataPos + 1, oopSearchStr);
		} while (labelDataPos > 0);
	}

	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandLock(OopState &state) {
	stat_lock(state.stat, state.game) = 1;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandUnlock(OopState &state) {
	stat_lock(state.stat, state.game) = 0;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandSend(OopState &state) {
	state.game.OopReadWord(state.stat, state.position);
	// state->oop_word is used by OopIterateStat
	sstring<20> oopWordCopy;
	StrCopy(oopWordCopy, state.game.oopWord);

	if (state.game.OopSend(state.stat_id, oopWordCopy, false)) {
		state.lineFinished = false;
	}
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandBecome(OopState &state) {
	Tile tile;
	if (state.game.OopParseTile(state.stat, state.position, tile)) {
		state.replaceStat = true;
		state.replaceTile = tile;
	} else {
		state.game.OopError(state.stat, "Bad #BECOME");
	}
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandPut(OopState &state) {
	int16_t deltaX, deltaY;
	Tile tile;
	state.game.OopReadDirection(state.stat, state.position, deltaX, deltaY);
	
	if (deltaX == 0 && deltaY == 0) {
		state.game.OopError(state.stat, "Bad #PUT");
	} else if (!state.game.OopParseTile(state.stat, state.position, tile)) {
		state.game.OopError(state.stat, "Bad #PUT");
	} else {
		int16_t destX = state.stat.x + deltaX;
		int16_t destY = state.stat.y + deltaY;
		if (destX > 0 && destX <= state.game.board.width() && destY > 0 && destY < state.game.board.height()) {
			if (!state.game.elementDefAt(destX, destY).walkable) {
				ElementPushablePush(state.game, destX, destY, deltaX, deltaY);
			}

			destX = state.stat.x + deltaX;
			destY = state.stat.y + deltaY;

			state.game.OopPlaceTile(destX, destY, tile);
		}
	}

	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandChange(OopState &state) {
	Tile fromTile, toTile;
	if (!state.game.OopParseTile(state.stat, state.position, fromTile)) {
		state.game.OopError(state.stat, "Bad #CHANGE");
	}
	if (!state.game.OopParseTile(state.stat, state.position, toTile)) {
		state.game.OopError(state.stat, "Bad #CHANGE");
	}

	int16_t ix = 0;
	int16_t iy = 1;
	if (toTile.color == 0 && state.game.elementDef(toTile.element).color < ColorSpecialMin) {
		toTile.color = state.game.elementDef(toTile.element).color;
	}

	while (state.game.FindTileOnBoard(ix, iy, fromTile)) {
		state.game.OopPlaceTile(ix, iy, toTile);
	}

	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandPlay(OopState &state) {
	char textLine[256];
	state.game.OopReadLineToEnd(state.stat, state.position, textLine, sizeof(textLine));
	if (!StrEmpty(textLine)) {
		uint8_t buf[255];
		int buflen = SoundParse(textLine, buf, sizeof(buf));
		if (buflen > 0) {
			state.game.driver->sound_queue(-1, buf, buflen);
		}
	}
	state.lineFinished = false;
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandCycle(OopState &state) {
	state.game.OopReadValue(state.stat, state.position);
	if (state.game.oopValue > 0) {
		state.stat.cycle = state.game.oopValue;
	}
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandChar(OopState &state) {
	state.game.OopReadValue(state.stat, state.position);
	if (state.game.oopValue > 0 && state.game.oopValue <= 255) {
		state.stat.p1 = (uint8_t) state.game.oopValue;
		state.game.BoardDrawTile(state.stat.x, state.stat.y);
	}
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandDie(OopState &state) {
	state.replaceStat = true;
	state.replaceTile = {
		.element = EEmpty,
		.color = 0x0F
	};
	return OOP_COMMAND_FINISHED;
}

OopCommandResult ZZT::OopCommandBind(OopState &state) {
	state.game.OopReadWord(state.stat, state.position);
	// state->oop_word is used by OopIterateStat
	sstring<20> oopWordCopy;
	StrCopy(oopWordCopy, state.game.oopWord);
	int16_t bindStatId = 0;
	if (state.game.OopIterateStat(state.stat_id, bindStatId, oopWordCopy)) {
		state.game.board.stats.free_data_if_unused(state.stat_id);
		state.stat.data = state.game.board.stats[bindStatId].data;
		state.position = 0;
	}
	return OOP_COMMAND_FINISHED;
}

bool Game::OopExecute(int16_t stat_id, int16_t &position, const char *default_name) {
StartParsing:
	Stat &stat = board.stats[stat_id];

	OopState state = {
		.game = *this,
		.stat = stat,
		.stat_id = stat_id,
		.position = position,
		.textWindow = nullptr,
		.stopRunning = false,
		.repeatInsNextTick = false,
		.replaceStat = false,
		.endOfProgram = false,
		.lineFinished = false,
		.insCount = 0,
		.replaceTile = {EEmpty, 0x00}
	};

	int16_t lastPosition;

	do {
ReadInstruction:
		state.lineFinished = true;
		lastPosition = position;
		OopReadChar(stat, position);

		// skip labels
		while (oopChar == ':') {
			do {
				OopReadChar(stat, position);
			} while (oopChar != 0 && oopChar != '\r');
			OopReadChar(stat, position);
		}

		switch (oopChar) {
			case '\'':
			case '@': {
				OopSkipLine(stat, position);
			} break;
			case '/':
			case '?': {
				int16_t deltaX, deltaY;
				bool is_try = (oopChar == '?');
				if (engineDefinition.isNot<QUIRK_OOP_SUPER_ZZT_MOVEMENT>()) {
					state.repeatInsNextTick = !is_try;
				}

				OopReadWord(stat, position);
				if (OopParseDirection(stat, position, deltaX, deltaY)) {
					if (engineDefinition.is<QUIRK_OOP_SUPER_ZZT_MOVEMENT>()) {
						// Super ZZT movement logic
						OopReadValue(stat, position);
						if (oopValue < 0) oopValue = 1;

						stat.step_x = deltaX;
						stat.step_y = deltaY;
						stat.p2 = is_try ? -oopValue : oopValue;
					} else {
						// ZZT movement logic
						if (deltaX != 0 || deltaY != 0) {
							int16_t destX = stat.x + deltaX;
							int16_t destY = stat.y + deltaY;
							if (!elementDefAt(destX, destY).walkable) {
								ElementPushablePush(*this, destX, destY, deltaX, deltaY);
							}

							destX = stat.x + deltaX;
							destY = stat.y + deltaY;

							if (elementDefAt(destX, destY).walkable) {
								MoveStat(stat_id, destX, destY);
								state.repeatInsNextTick = false;
							}
						} else {
							state.repeatInsNextTick = false;
						}
					}

					OopReadChar(stat, position);
					if (oopChar != '\r') position--;

					state.stopRunning = true;
				} else {
					OopError(stat, "Bad direction");
				}
			} break;
			case '#': {
ReadCommand:
				OopReadWord(stat, position);
				if (StrEquals(oopWord, "THEN")) {
					OopReadWord(stat, position);
				}
				if (StrEmpty(oopWord)) {
					goto ReadInstruction;
				} else {
					state.insCount++;
					OopCommandProc proc = engineDefinition.oopCommandMap.get(oopWord);
					
					if (proc != nullptr) {
						OopCommandResult result = proc(state);
						if (result == OOP_COMMAND_NEXT) goto ReadCommand;
					} else {
						sstring<20> oopWordCopy;
						StrCopy(oopWordCopy, oopWord);

						if (OopSend(stat_id, oopWordCopy, false)) {
							state.lineFinished = false;
						} else {
							if (strchr(oopWordCopy, ':') == NULL) {
								char textLine[61];
								StrJoin(textLine, 2, "Bad command ", oopWordCopy);
								OopError(stat, textLine);
							}
						}
					}
				}

				if (state.lineFinished) {
					OopSkipLine(stat, position);
				}
			} break;
			case '\r': {
				// OpenZoo: implies textWindow.lineCount > 0
				if (state.textWindow != nullptr) {
					state.textWindow->Append("");
				}
			} break;
			case 0: {
				state.endOfProgram = true;
			} break;
			default: {
				char textLine[66]; // should be enough?
				textLine[0] = oopChar;
				OopReadLineToEnd(stat, position, textLine + 1, sizeof(textLine) - 1);
				if (state.textWindow == nullptr) {
					state.textWindow = interface->CreateTextWindow(filesystem);
					state.textWindow->selectable = false;
				}
				state.textWindow->Append(textLine);
			} break;
		}
	} while (!state.endOfProgram && !state.stopRunning && !state.repeatInsNextTick && !state.replaceStat && state.insCount <= 32);

	if (state.repeatInsNextTick) {
		position = lastPosition;
	}

	if (oopChar == 0) {
		position = -1;
	}

	if (state.textWindow == nullptr) {
		// implies 0 lines
	} else if (state.textWindow->line_count > engineDefinition.messageLines) {
		char name[256];
		StrCopy(name, default_name);

		int16_t namePosition = 0;
		OopReadChar(stat, namePosition);
		if (oopChar == '@') {
			OopReadLineToEnd(stat, namePosition, name, sizeof(name));
		}

		if (StrEmpty(name)) {
			StrCopy(name, "Interaction");
		}

		StrCopy(state.textWindow->title, name);
		state.textWindow->DrawOpen();
		state.textWindow->Select(true, false);
		state.textWindow->DrawClose();
		state.textWindow->Clear();

		if (!StrEmpty(state.textWindow->hyperlink)) {
			if (OopSend(stat_id, state.textWindow->hyperlink, false)) {
				delete state.textWindow;
				goto StartParsing;
			}
		}

		delete state.textWindow;
	} else if (state.textWindow->line_count >= 2) {
		DisplayMessage(200, state.textWindow->lines[0]->c_str(), state.textWindow->lines[1]->c_str());
		state.textWindow->Clear();
		delete state.textWindow;
	} else if (state.textWindow->line_count >= 1) {
		DisplayMessage(200, state.textWindow->lines[0]->c_str());
		state.textWindow->Clear();
		delete state.textWindow;
	}

	if (state.replaceStat) {
		int16_t ix = stat.x;
		int16_t iy = stat.y;
		if (engineDefinition.isNot<QUIRK_SUPER_ZZT_COMPAT_MISC>()) {
			DamageStat(stat_id);
		}
		OopPlaceTile(ix, iy, state.replaceTile);
	}

	return state.replaceStat;
}
