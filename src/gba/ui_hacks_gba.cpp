#include "ui_hacks_gba.h"
#include "driver_gba.h"

using namespace ZZT;

void TextWindowSuperZZTGBA::DrawOpen(void) {
	zoo_set_force_4x8_mode(FORCE_4X8_MODE_ALWAYS);
	TextWindow::DrawOpen();
}

void TextWindowSuperZZTGBA::DrawClose(void) {
	zoo_set_force_4x8_mode(FORCE_4X8_MODE_READ);
	TextWindow::DrawClose();
	zoo_set_force_4x8_mode(FORCE_4X8_MODE_NONE);
}

void UserInterfaceSuperZZTGBA::PopupPromptString(const char *question, char *buffer, size_t buffer_len) {
	zoo_set_force_4x8_mode(FORCE_4X8_MODE_ALWAYS);
    UserInterface::PopupPromptString(4, 18, 50, 0x4F, question, buffer, buffer_len);
	zoo_set_force_4x8_mode(FORCE_4X8_MODE_NONE);
}

UserInterfaceSuperZZTGBA::UserInterfaceSuperZZTGBA(Driver *driver) : UserInterfaceSuperZZT(driver, 36, 20) { }

TextWindow *UserInterfaceSuperZZTGBA::CreateTextWindow(FilesystemDriver *fsDriver) {
	return new TextWindowSuperZZTGBA(driver, fsDriver, 12, 1, 38, 17);
}