#include <cstddef>
#include <cstdlib>
#include <cstring>
#include "txtwind.h"

using namespace ZZT;
using namespace ZZT::Utils;

static int16_t window_x = 5, window_y = 3, window_width = 50, window_height = 18;

static const char draw_patterns[4][5] = {
	{'\xC6', '\xD1', '\xCD', '\xD1', '\xB5'}, // top
	{'\xC6', '\xCF', '\xCD', '\xCF', '\xB5'}, // bottom
	{' ', '\xB3', ' ', '\xB3', ' '}, // inner
	{' ', '\xC6', '\xCD', '\xB5', ' '} // separator
};

void ZZT::TextWindowDrawPattern(VideoDriver *video, int16_t x, int16_t y, int16_t width, uint8_t color, WindowPatternType ptype) {
	int16_t ix;
	const char *pattern = draw_patterns[ptype];

	video->draw_char(x, y, color, pattern[0]);
	video->draw_char(x + 1, y, color, pattern[1]);
	for (ix = 2; ix < width - 2; ix++) {
		video->draw_char(x + ix, y, color, pattern[2]);
	}
	video->draw_char(x + width - 2, y, color, pattern[3]);
	video->draw_char(x + width - 1, y, color, pattern[4]);
}

void TextWindow::DrawBorderLine(int16_t y, WindowPatternType ptype) {
	TextWindowDrawPattern(video, window_x, y, window_width, 0x0F, ptype);
}

TextWindow::TextWindow(VideoDriver *video, InputDriver *input, SoundDriver *sound, FilesystemDriver *filesystem) {
    this->video = video;
    this->input = input;
    this->sound = sound;
    this->filesystem = filesystem;
    this->line_count = 0;
    this->line_pos = 0;
    StrClear(this->loaded_filename);
    this->screenCopy = nullptr;
}

TextWindow::~TextWindow() {
    Clear();
    if (this->screenCopy != nullptr) {
        delete this->screenCopy;
    }
}

void TextWindow::Clear(void) {
    for (int i = 0; i < this->line_count; i++)
        delete this->lines[i];
    this->line_pos = 0;
    this->line_count = 0;
}

void TextWindow::DrawTitle(uint8_t color, const char *title) {
	int16_t i;
	int16_t il = strlen(title);
	int16_t is = window_x + ((window_width + 1 - il) >> 1);

	for (i = window_x + 2; i < (window_x + window_width - 2); i++) {
		if (i >= is && i < (is+il)) {
			video->draw_char(i, window_y + 1, color, title[i - is]);
		} else {
			video->draw_char(i, window_y + 1, color, ' ');
		}
	}
}

void TextWindow::DrawOpen(void) {
    screenCopy = new VideoCopy((uint8_t) window_width, (uint8_t) (window_height + 1));
    video->copy_chars(*screenCopy, window_x, window_y, window_width, window_height + 1, 0, 0);

    for (int iy = window_height / 2; iy >= 0; iy--) {
        DrawBorderLine(window_y + iy + 1, WinPatInner);
        DrawBorderLine(window_y + window_height - iy - 1, WinPatInner);
        DrawBorderLine(window_y + iy, WinPatTop);
        DrawBorderLine(window_y + window_height - iy, WinPatBottom);
        sound->delay(25);
    }

    DrawBorderLine(window_y + 2, WinPatSeparator);
    DrawTitle(0x1E, title);
}

void TextWindow::DrawClose(void) {
    for (int iy = 0; iy <= window_height / 2; iy++) {
        DrawBorderLine(window_y + iy, WinPatTop);
        DrawBorderLine(window_y + window_height - iy, WinPatBottom);
        sound->delay(18);
        video->paste_chars(*screenCopy, 0, iy, window_width, 1, window_x, window_y + iy);
        video->paste_chars(*screenCopy, 0, window_height - iy, window_width, 1, window_x, window_y + window_height - iy);
    }

    delete screenCopy;
    screenCopy = nullptr;
}

void TextWindow::Append(const char *line) {
    /* this->lines[this->line_count] = (char*) malloc(str_width + 1);
    strncpy(this->lines[this->line_count], line, str_width);
    this->lines[this->line_count][str_width] = 0; */

    if (this->line_count >= MAX_TEXT_WINDOW_LINES) return;
    this->lines[this->line_count++] = new DynString(line);
}

void TextWindow::Append(const DynString line) {
    if (this->line_count >= MAX_TEXT_WINDOW_LINES) return;
    this->lines[this->line_count++] = new DynString(line);
}

void TextWindow::DrawLine(int16_t lpos, bool withoutFormatting, bool viewingFile) {
	int line_y;
	int text_x, text_width;
	int text_color;
	bool same_line;

	line_y = (window_y + lpos - line_pos) + (window_height >> 1) + 1;
	same_line = lpos == line_pos;
	video->draw_char(window_x + 2, line_y, 0x1C, same_line ? '\xAF' : ' ');
	video->draw_char(window_x + window_width - 3, line_y, 0x1C, same_line ? '\xAE' : ' ');

	text_color = 0x1E;
	text_x = 0;
	text_width = (window_width - 7);

    bool is_boundary, draw_arrow = false;
    const char *str = NULL;

	if (lpos >= 0 && lpos < line_count) {
        str = lines[lpos]->c_str();
        const char *tmp = NULL;

		if (!withoutFormatting) switch (str[0]) {
			case '!':
				tmp = strchr(str, ';');
				if (tmp != NULL) str = tmp + 1;
				draw_arrow = true;
				text_x += 5;
				text_color = 0x1F;
				break;
			case ':':
				tmp = strchr(str, ';');
				if (tmp != NULL) str = tmp + 1;
				text_color = 0x1F;
				break;
			case '$':
				str++;
				text_color = 0x1F;
				// (window_width - 8 - strlen(str)) / 2
				text_x = (text_width - 1 - strlen(str)) >> 1;
				break;
		}
    }

    if (str != NULL) {
        for (int i = -text_x - 1; i < (text_width - text_x); i++) {
            if (draw_arrow && i == -3) {
                video->draw_char(window_x + 4 + i + text_x, line_y, 0x1D, '\x10');
            } else {
                video->draw_char(window_x + 4 + i + text_x, line_y, text_color,
                    (i >= 0 && i < strlen(str)) ? str[i] : ' ');
            }
        }
    } else {
        is_boundary = lpos == -1 || lpos == line_count;
        for (int i = 0; i < (window_width - 4); i++) {
            video->draw_char(window_x + 2 + i, line_y, text_color,
                (is_boundary && ((i % 5) == 4)) ? '\x07' : ' ');
        }
    }
    
    if (viewingFile) {
        if (lpos == -4) {
            video->draw_string(window_x + 2, line_y, 0x1A, "   Use            to view text,");
            video->draw_string(window_x + 2 + 7, line_y, 0x1F, "\x18 \x19, Enter");
        } else if (lpos == -3) {
            video->draw_string(window_x + 2, line_y, 0x1A, "                  to print.");
            video->draw_string(window_x + 2 + 12, line_y, 0x1F, "Alt-P");
        }
	}
}

void TextWindow::Draw(bool withoutFormatting, bool viewingFile) {
    for (int i = 0; i <= window_height - 4; i++) {
        DrawLine(line_pos - (window_height / 2) + i + 2, withoutFormatting, viewingFile);
    }
    DrawTitle(0x1E, title);
}

void TextWindow::Select(bool hyperlinkAsSelect, bool viewingFile) {
    rejected = false;
    StrClear(hyperlink);
    Draw(false, viewingFile);
    do {
        sound->idle(IMUntilFrame);
        input->update_input();
        int16_t new_line_pos = line_pos;
        if (input->deltaY != 0) {
            new_line_pos += input->deltaY;
        } else if (input->shiftPressed || input->keyPressed == KeyEnter) {
            input->shiftAccepted = true;
            if (lines[line_pos]->length() > 0 && (*lines[line_pos])[0] == '!') {
                sstring<20> pointerStr;
                StrCopy(pointerStr, (lines[line_pos]->c_str()) + 1);

                char *divPos = strchr(pointerStr, ';');
                if (divPos != NULL) {
                    *divPos = 0;
                }

                if (pointerStr[0] == '-') {
                    Clear();
                    OpenFile(pointerStr + 1, false);
                    if (line_count == 0) {
                        return;
                    } else {
                        viewingFile = true;
                        new_line_pos = line_pos;
                        Draw(false, viewingFile);
                        input->keyPressed = 0;
                        input->shiftPressed = false;
                    }
                } else {
                    if (hyperlinkAsSelect) {
                        StrCopy(hyperlink, pointerStr);
                    } else {
                        sstring<21> label;
                        StrJoin(label, 2, ":", pointerStr);
                        int label_len = StrLength(label);
                        for (int i = 0; i < line_count; i++) {
                            if (label_len == lines[i]->length()) {
                                bool match = true;
                                for (int ic = 0; ic < label_len; ic++) {
                                    if (UpCase(label[ic]) != UpCase((*lines[i])[ic])) {
                                        match = false;
                                        break;
                                    }
                                }
                                if (match) {
                                    new_line_pos = i;
                                    input->keyPressed = 0;
                                    input->shiftPressed = false;
                                    goto LabelMatched;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            switch (input->keyPressed) {
                case KeyPageUp: {
                    new_line_pos = line_pos - window_height + 4;
                } break;
                case KeyPageDown: {
                    new_line_pos = line_pos + window_height - 4;
                } break;
                case KeyAltP: {
                    // TODO: Printing (in ZZT but I don't really care)
                } break;
            }
        }

    LabelMatched:
        if (new_line_pos < 0) new_line_pos = 0;
        else if (new_line_pos >= line_count) new_line_pos = line_count - 1;

        if (new_line_pos != line_pos) {
            line_pos = new_line_pos;
            Draw(false, viewingFile);

            if (lines[line_pos]->length() > 0 && (*lines[line_pos])[0] == '!') {
                DrawTitle(0x1E, hyperlinkAsSelect
                    ? "\xAEPress ENTER to select this\xAF"
                    : "\xAEPress ENTER for more info\xAF"
                );
            }
        }

        if (input->joystickMoved) {
            sound->delay(35);
        }
    } while (input->keyPressed != KeyEscape && input->keyPressed != KeyEnter && !input->shiftPressed);

    if (input->keyPressed == KeyEscape) {
        input->keyPressed = 0;
        rejected = true;
    }
}

int16_t TextWindow::Edit_DeleteCurrLine(void) {
    if (line_count > 1) {
        delete lines[line_pos];
        for (int i = line_pos + 1; i < line_count; i++) {
            lines[i - 1] = lines[i];
        }
        line_count--;
        if (line_pos > line_count) {
            return line_count;
        } else {
            Draw(true, false);
            return line_pos;
        }
    } else {
        delete lines[0];
        lines[0] = new DynString();
        return line_pos;
    }
}

void TextWindow::Edit(void) {
    if (line_count <= 0) {
        Append("");
    }
    bool insert_mode = false;
    line_pos = 0;
    int16_t char_pos = 0;
    Draw(true, false);
    do {
        video->draw_string(77, 14, 0x1E, insert_mode ? "on " : "off");

        if (char_pos >= lines[line_pos]->length()) {
            char_pos = lines[line_pos]->length();
            video->draw_char(char_pos + window_x + 4,
                window_y + (window_height / 2) + 1,
                0x70, ' ');
        } else {
            video->draw_char(char_pos + window_x + 4,
                window_y + (window_height / 2) + 1,
                0x70, (*lines[line_pos])[char_pos]);
        }

        input->read_wait_key();
        int16_t new_line_pos = line_pos;
        switch (input->keyPressed) {
            case KeyUp:
                new_line_pos = line_pos - 1;
                break;
            case KeyDown:
                new_line_pos = line_pos + 1;
                break;
            case KeyPageUp:
                new_line_pos = line_pos - window_height + 4;
                break;
            case KeyPageDown:
                new_line_pos = line_pos + window_height - 4;
                break;
            case KeyRight: {
                char_pos++;
                if (char_pos > lines[line_pos]->length()) {
                    char_pos = 0;
                    new_line_pos = line_pos + 1;
                }
            } break;
            case KeyLeft: {
                char_pos--;
                if (char_pos < 0) {
                    char_pos = window_width;
                    new_line_pos = line_pos - 1;
                }
            } break;
            case KeyEnter: {
                if (line_count < MAX_TEXT_WINDOW_LINES) {
                    for (int i = line_count - 1; i >= line_pos + 1; i--) {
                        lines[i + 1] = lines[i];
                    }

                    DynString *s = lines[line_pos];
                    lines[line_pos + 1] = new DynString(char_pos >= s->length() ? "" : s->substr(char_pos, lines[line_pos]->length() - char_pos));
                    lines[line_pos] = new DynString(s->substr(0, char_pos));
                    delete s;

                    new_line_pos = line_pos + 1;
                    char_pos = 0;
                    line_count++;
                }
            } break;
            case KeyBackspace: {
                if (char_pos > 0) {
                    DynString *s = lines[line_pos];
                    lines[line_pos] = new DynString(
                        s->substr(0, char_pos - 1) + s->substr(char_pos, s->length() - char_pos)
                    );
                    delete s;
                    char_pos--;
                } else if (lines[line_pos]->length() == 0) {
                    Edit_DeleteCurrLine();
                    new_line_pos = line_pos - 1;
                    char_pos = window_width;
                }
            } break;
            case KeyInsert:
                insert_mode = !insert_mode;
                break;
            case KeyDelete: {
                if (lines[line_pos]->length() > 0) {
                    DynString *s = lines[line_pos];
                    if (char_pos < lines[line_pos]->length()) {
                        lines[line_pos] = new DynString(
                            s->substr(0, char_pos) + s->substr(char_pos + 1, s->length() - char_pos - 1)
                        );
                    } else {
                        lines[line_pos] = new DynString(
                            s->substr(0, char_pos) 
                        );
                    }
                    delete s;
                }
            } break;
            case KeyCtrlY: {
                Edit_DeleteCurrLine();
            } break;
            default: {
                if (input->keyPressed >= 32 && input->keyPressed < 127 && char_pos < (window_width - 7)) {
                    if (!insert_mode) {
                        DynString *s = lines[line_pos];
                        if (char_pos < s->length()) {
                            lines[line_pos] = new DynString(
                                s->substr(0, char_pos) + ((char) input->keyPressed) + s->substr(char_pos, s->length() - char_pos)
                            );
                        } else {
                            lines[line_pos] = new DynString(
                                s->substr(0, char_pos) + ((char) input->keyPressed)
                            );
                        }
                        delete s;

                        char_pos++;
                    } else {
                        if (lines[line_pos]->length() < (window_width - 8)) {
                            DynString *s = lines[line_pos];
                            if ((char_pos + 1) < s->length()) {
                                lines[line_pos] = new DynString(
                                    s->substr(0, char_pos) + ((char) input->keyPressed) + s->substr(char_pos + 1, s->length() - char_pos - 1)
                                );
                            } else {
                                lines[line_pos] = new DynString(
                                    s->substr(0, char_pos) + ((char) input->keyPressed)
                                );
                            }
                            delete s;

                            char_pos++;
                        }
                    }
                }
            } break;
        }

        if (new_line_pos < 0) new_line_pos = 0;
        else if (new_line_pos >= line_count) new_line_pos = line_count - 1;

        if (new_line_pos != line_pos) {
            line_pos = new_line_pos;
            Draw(true, false);
        } else {
            DrawLine(line_pos, true, false);
        }
    } while (input->keyPressed != KeyEscape);

    if (lines[line_count - 1]->length() == 0) {
        delete lines[line_count - 1];
        line_count--;
    }
}

void TextWindow::OpenFile(const char *filename, bool errorIfMissing) {
    sstring<255> filename_joined;

    if (filename[0] == '*') {
        filename++;
    }
    if (strchr(filename, '.') == NULL) {
        StrJoin(filename_joined, 2, filename, ".HLP");
    } else {
        StrCopy(filename_joined, filename);
    }

    Clear();
    {
        sstring<255> line;
        int lpos = 0;

        StrClear(line);
        IOStream *stream = filesystem->open_file(filename_joined, false);
        while (!stream->eof() && !stream->errored()) {
            char c = stream->read8();
            if (c == '\r') {
                line[lpos] = 0;
                Append(line);
                lpos = 0;
                StrClear(line);
            } else {
                if (lpos < StrSize(line)) {
                    line[lpos++] = c;
                }
            }
        }

        if (stream->errored() && !stream->eof()) {
            Clear();
            if (errorIfMissing) {
                StrJoin(line, 2, "Error reading ", filename_joined);
                Append(line);
            }
        } else {
            // Finish reading.
            if (lpos > 0) {
                line[lpos] = 0;
                Append(line);
            }
        }

        delete stream;
    }
}

void TextWindow::SaveFile(const char *filename) {
    IOStream *stream = filesystem->open_file(filename, true);
    for (int i = 0; i < line_count; i++) {
        stream->write_cstring((const char *) lines[i], false);
        stream->write8('\r');
        stream->write8('\n');
    }
    delete stream;
}

static int cmp_dynstring(const void *a, const void *b) {
    const DynString *as = *((const DynString **) a);
    const DynString *bs = *((const DynString **) b);
    return strcmp(as->c_str(), bs->c_str());
}

void TextWindow::Sort(int16_t start, int16_t count) {
#ifdef HAVE_QSORT
    if (start < 0) start = 0;
    if ((start + count) >= line_count) count = line_count - start;

    if (count > 0) {
        qsort(lines + start, count, sizeof(DynString*), cmp_dynstring);
    }
#endif
}

void ZZT::TextWindowDisplayFile(VideoDriver *video, InputDriver *input, SoundDriver *sound, FilesystemDriver *filesystem, const char *filename, const char *title) {
    TextWindow window = TextWindow(video, input, sound, filesystem);
    StrCopy(window.title, title);
    window.OpenFile(filename, false);
    window.selectable = false;
    if (window.line_count > 0) {
        window.DrawOpen();
        window.Select(false, true);
        window.DrawClose();
    }
}

void ZZT::TextWindowInit(int16_t x, int16_t y, int16_t width, int16_t height) {
    window_x = x;
    window_y = y;
    window_width = width;
    window_height = height;
}
