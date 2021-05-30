#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "gamevars.h"
#include "txtwind.h"
#include "platform_hacks.h"

using namespace ZZT;
using namespace ZZT::Utils;

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
	int16_t pos = start_pos;
	size_t str_len = strlen(str);
	int16_t max_pos = stat.data.len - str_len;

	for (int i = 0; i < str_len; i++) {
		str[i] = UpCase(str[i]);
	}

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
	for (int i = 0; i < MAX_FLAG; i++) {
		if (StrEquals(world.info.flags[i], name)) {
			return i;
		}
	}
	return -1;
}

void Game::WorldSetFlag(const char *name) {
	if (WorldGetFlagPosition(name) < 0) {
		for (int i = 0; i < MAX_FLAG; i++) {
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

	for (int i = 0; i < ElementCount; i++) {
		OopStringToWord(elementDef(i).name, compared, sizeof(compared));
		if (StrEquals(oopWord, compared)) {
			tile.element = i;
			return true;
		}
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
			if (tile.color == 0 || GetColorForTileMatch(found) == tile.color) {
				return true;
			}
		}
	}
}

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
		if ((i_stat.p2 == 0) || ignoreLock || ((stat_id == i_stat_id) && !ignoreSelfLock)) {
			if (i_stat_id == stat_id) {
				result = true;
			}

			i_stat.data_pos = i_data_pos;
		}
	}

	return result;
}

void Game::OopExecute(int16_t stat_id, int16_t &position, const char *default_name) {
StartParsing:
	TextWindow *textWindow = nullptr;
	bool stopRunning = false;
	bool repeatInsNextTick = false;
	bool replaceStat = false;
	bool endOfProgram = false;
	bool lineFinished;
	int16_t insCount = 0;
	int16_t lastPosition;
	Tile replaceTile;

	Stat &stat = board.stats[stat_id];

	do {
ReadInstruction:
		lineFinished = true;
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
				repeatInsNextTick = true;
			case '?': {
				int16_t deltaX, deltaY;

				OopReadWord(stat, position);
				if (OopParseDirection(stat, position, deltaX, deltaY)) {
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
							repeatInsNextTick = false;
						}
					} else {
						repeatInsNextTick = false;
					}

					OopReadChar(stat, position);
					if (oopChar != '\r') position--;

					stopRunning = true;
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
					insCount++;
					if (StrEquals(oopWord, "GO")) {
						int16_t deltaX, deltaY;
						OopReadDirection(stat, position, deltaX, deltaY);
						int16_t destX = stat.x + deltaX;
						int16_t destY = stat.y + deltaY;

						if (!elementDefAt(destX, destY).walkable) {
							ElementPushablePush(*this, destX, destY, deltaX, deltaY);
						}

						destX = stat.x + deltaX;
						destY = stat.y + deltaY;

						if (elementDefAt(destX, destY).walkable) {
							MoveStat(stat_id, destX, destY);
						} else {
							repeatInsNextTick = true;
						}

						stopRunning = true;
					} else if (StrEquals(oopWord, "TRY")) {
						int16_t deltaX, deltaY;
						OopReadDirection(stat, position, deltaX, deltaY);
						int16_t destX = stat.x + deltaX;
						int16_t destY = stat.y + deltaY;

						if (!elementDefAt(destX, destY).walkable) {
							ElementPushablePush(*this, destX, destY, deltaX, deltaY);
						}

						destX = stat.x + deltaX;
						destY = stat.y + deltaY;

						if (elementDefAt(destX, destY).walkable) {
							MoveStat(stat_id, destX, destY);
							stopRunning = true;
						} else {
							goto ReadCommand;
						}
					} else if (StrEquals(oopWord, "WALK")) {
						int16_t newStepX = stat.step_x;
						int16_t newStepY = stat.step_y;
						OopReadDirection(stat, position, newStepX, newStepY);
						stat.step_x = newStepX;
						stat.step_y = newStepY;
					} else if (StrEquals(oopWord, "SET")) {
						OopReadWord(stat, position);
						WorldSetFlag(oopWord);
					} else if (StrEquals(oopWord, "CLEAR")) {
						OopReadWord(stat, position);
						WorldClearFlag(oopWord);
					} else if (StrEquals(oopWord, "IF")) {
						OopReadWord(stat, position);
						if (OopCheckCondition(stat, position)) {
							goto ReadCommand;
						}
					} else if (StrEquals(oopWord, "SHOOT")) {
						int16_t deltaX, deltaY;
						OopReadDirection(stat, position, deltaX, deltaY);
						if (BoardShoot(EBullet, stat.x, stat.y, deltaX, deltaY, ShotSourceEnemy)) {
							driver->sound_queue(2, "\x30\x01\x26\x01");
						}
						stopRunning = true;
					} else if (StrEquals(oopWord, "THROWSTAR")) {
						int16_t deltaX, deltaY;
						OopReadDirection(stat, position, deltaX, deltaY);
						if (BoardShoot(EStar, stat.x, stat.y, deltaX, deltaY, ShotSourceEnemy)) {
							// pass
						}
						stopRunning = true;
					} else if (StrEquals(oopWord, "GIVE") || StrEquals(oopWord, "TAKE")) {
						bool counterSubtract = StrEquals(oopWord, "TAKE");

						OopReadWord(stat, position);
						int16_t *counterPtr = nullptr;
						if (StrEquals(oopWord, "HEALTH")) {
							counterPtr = &world.info.health;
						} else if (StrEquals(oopWord, "AMMO")) {
							counterPtr = &world.info.ammo;
						} else if (StrEquals(oopWord, "GEMS")) {
							counterPtr = &world.info.gems;
						} else if (StrEquals(oopWord, "TORCHES")) {
							counterPtr = &world.info.torches;
						} else if (StrEquals(oopWord, "SCORE")) {
							counterPtr = &world.info.score;
						} else if (StrEquals(oopWord, "TIME")) {
							counterPtr = &world.info.board_time_sec;
						}

						if (counterPtr != nullptr) {
							OopReadValue(stat, position);
							if (oopValue > 0) {
								if (counterSubtract) {
									oopValue = -oopValue;
								}

								if (((*counterPtr) + oopValue) >= 0) {
									*counterPtr += oopValue;
								} else {
									goto ReadCommand;
								}
							}
						}

						GameUpdateSidebar();
					} else if (StrEquals(oopWord, "END")) {
						position = -1;
						oopChar = 0;
					} else if (StrEquals(oopWord, "ENDGAME")) {
						world.info.health = 0;
					} else if (StrEquals(oopWord, "IDLE")) {
						stopRunning = true;
					} else if (StrEquals(oopWord, "RESTART")) {
						position = 0;
						lineFinished = false;
					} else if (StrEquals(oopWord, "ZAP")) {
						OopReadWord(stat, position);
						// state->oop_word is used by OopIterateStat
						sstring<20> oopWordCopy;
						StrCopy(oopWordCopy, oopWord);

						int16_t labelStatId = 0;
						int16_t labelDataPos;
						while (OopFindLabel(stat_id, oopWordCopy, labelStatId, labelDataPos, "\r:")) {
							board.stats[labelStatId].data.data[labelDataPos + 1] = '\'';
						}
					} else if (StrEquals(oopWord, "RESTORE")) {
						OopReadWord(stat, position);
						// state->oop_word is used by OopIterateStat
						sstring<20> oopWordCopy;
						sstring<30> oopSearchStr;
						StrCopy(oopWordCopy, oopWord);
						StrJoin(oopSearchStr, 3, "\r'", oopWord, "\r");

						int16_t labelStatId = 0;
						int16_t labelDataPos;
						while (OopFindLabel(stat_id, oopWordCopy, labelStatId, labelDataPos, "\r'")) {
							Stat &labelStat = board.stats[labelStatId];

							do {
								labelStat.data.data[labelDataPos + 1] = ':';
								labelDataPos = OopFindString(labelStat, labelDataPos + 1, oopSearchStr);
							} while (labelDataPos > 0);
						}
					} else if (StrEquals(oopWord, "LOCK")) {
						stat.p2 = 1;
					} else if (StrEquals(oopWord, "UNLOCK")) {
						stat.p2 = 0;
					} else if (StrEquals(oopWord, "SEND")) {
						OopReadWord(stat, position);
						// state->oop_word is used by OopIterateStat
						sstring<20> oopWordCopy;
						StrCopy(oopWordCopy, oopWord);

						if (OopSend(stat_id, oopWordCopy, false)) {
							lineFinished = false;
						}
					} else if (StrEquals(oopWord, "BECOME")) {
						Tile tile;
						if (OopParseTile(stat, position, tile)) {
							replaceStat = true;
							replaceTile = tile;
						} else {
							OopError(stat, "Bad #BECOME");
						}
					} else if (StrEquals(oopWord, "PUT")) {
						int16_t deltaX, deltaY;
						Tile tile;
						OopReadDirection(stat, position, deltaX, deltaY);
						
						if (deltaX == 0 && deltaY == 0) {
							OopError(stat, "Bad #PUT");
						} else if (!OopParseTile(stat, position, tile)) {
							OopError(stat, "Bad #PUT");
						} else {
							int16_t destX = stat.x + deltaX;
							int16_t destY = stat.y + deltaY;
							if (destX > 0 && destX <= board.width() && destY > 0 && destY < board.height()) {
								if (!elementDefAt(destX, destY).walkable) {
									ElementPushablePush(*this, destX, destY, deltaX, deltaY);
								}

								destX = stat.x + deltaX;
								destY = stat.y + deltaY;

								OopPlaceTile(destX, destY, tile);
							}
						}
					} else if (StrEquals(oopWord, "CHANGE")) {
						Tile fromTile, toTile;
						if (!OopParseTile(stat, position, fromTile)) {
							OopError(stat, "Bad #CHANGE");
						}
						if (!OopParseTile(stat, position, toTile)) {
							OopError(stat, "Bad #CHANGE");
						}

						int16_t ix = 0;
						int16_t iy = 1;
						if (toTile.color == 0 && elementDef(toTile.element).color < ColorSpecialMin) {
							toTile.color = elementDef(toTile.element).color;
						}

						while (FindTileOnBoard(ix, iy, fromTile)) {
							OopPlaceTile(ix, iy, toTile);
						}
					} else if (StrEquals(oopWord, "PLAY")) {
						char textLine[256];
						OopReadLineToEnd(stat, position, textLine, sizeof(textLine));
						if (!StrEmpty(textLine)) {
							uint8_t buf[255];
							int buflen = SoundParse(textLine, buf, sizeof(buf));
							if (buflen > 0) {
								driver->sound_queue(-1, buf, buflen);
							}
						}
						lineFinished = false;
					} else if (StrEquals(oopWord, "CYCLE")) {
						OopReadValue(stat, position);
						if (oopValue > 0) {
							stat.cycle = oopValue;
						}
					} else if (StrEquals(oopWord, "CHAR")) {
						OopReadValue(stat, position);
						if (oopValue > 0 && oopValue <= 255) {
							stat.p1 = (uint8_t) oopValue;
							BoardDrawTile(stat.x, stat.y);
						}
					} else if (StrEquals(oopWord, "DIE")) {
						replaceStat = true;
						replaceTile = {
							.element = EEmpty,
							.color = 0x0F
						};
					} else if (StrEquals(oopWord, "BIND")) {
						OopReadWord(stat, position);
						// state->oop_word is used by OopIterateStat
						sstring<20> oopWordCopy;
						StrCopy(oopWordCopy, oopWord);
						int16_t bindStatId = 0;
						if (OopIterateStat(stat_id, bindStatId, oopWordCopy)) {
							board.stats.free_data_if_unused(stat_id);
							stat.data = board.stats[bindStatId].data;
							position = 0;
						}
					} else {
						sstring<20> oopWordCopy;
						StrCopy(oopWordCopy, oopWord);

						if (OopSend(stat_id, oopWordCopy, false)) {
							lineFinished = false;
						} else {
							if (strchr(oopWordCopy, ':') == NULL) {
								char textLine[61];
								StrJoin(textLine, 2, "Bad command ", oopWordCopy);
								OopError(stat, textLine);
							}
						}
					}
				}

				if (lineFinished) {
					OopSkipLine(stat, position);
				}
			} break;
			case '\r': {
				// OpenZoo: implies textWindow.lineCount > 0
				if (textWindow != nullptr) {
					textWindow->Append("");
				}
			} break;
			case 0: {
				endOfProgram = true;
			} break;
			default: {
				char textLine[66]; // should be enough?
				textLine[0] = oopChar;
				OopReadLineToEnd(stat, position, textLine + 1, sizeof(textLine) - 1);
				if (textWindow == nullptr) {
					textWindow = new TextWindow(driver, filesystem);
					textWindow->selectable = false;
				}
				textWindow->Append(textLine);
			} break;
		}
	} while (!endOfProgram && !stopRunning && !repeatInsNextTick && !replaceStat && insCount <= 32);

	if (repeatInsNextTick) {
		position = lastPosition;
	}

	if (oopChar == 0) {
		position = -1;
	}

	if (textWindow == nullptr) {
		// implies 0 lines
	} else if (textWindow->line_count > 1) {
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

		StrCopy(textWindow->title, name);
		textWindow->DrawOpen();
		textWindow->Select(true, false);
		textWindow->DrawClose();
		textWindow->Clear();

		if (!StrEmpty(textWindow->hyperlink)) {
			if (OopSend(stat_id, textWindow->hyperlink, false)) {
				delete textWindow;
				goto StartParsing;
			}
		}

		delete textWindow;
	} else if (textWindow->line_count == 1) {
		DisplayMessage(200, textWindow->lines[0]->c_str());
		textWindow->Clear();
		delete textWindow;
	}

	if (replaceStat) {
		int16_t ix = stat.x;
		int16_t iy = stat.y;
		DamageStat(stat_id);
		OopPlaceTile(ix, iy, replaceTile);
	}
}
