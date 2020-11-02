#include <cstddef>
#include <cstring>
#include "driver_null.h"

using namespace ZZT;

NullDriver::NullDriver(void) {

}

bool NullDriver::configure(void) {
    return true;
}

void NullDriver::install(void) {
    
}

void NullDriver::uninstall(void) {
    
}

void NullDriver::update_input(void) {
    
}

void NullDriver::read_wait_key(void) {
    
}

uint16_t NullDriver::get_hsecs(void) {
    return hsecs++;
}

void NullDriver::delay(int ms) {

}

void NullDriver::idle(IdleMode mode) {

}

void NullDriver::sound_stop(void) {

}

void NullDriver::draw_char(int16_t x, int16_t y, uint8_t col, uint8_t chr) {

}

void NullDriver::read_char(int16_t x, int16_t y, uint8_t &col, uint8_t &chr) {
    col = 0;
    chr = 0;
}

#include "gamevars.h"

int main(int argc, char** argv) {
	NullDriver driver = NullDriver();
	Game game = Game();
	
	game.input = &driver;
	game.sound = &driver;
	game.video = &driver;

	game.GameTitleLoop();

	return 0;
}