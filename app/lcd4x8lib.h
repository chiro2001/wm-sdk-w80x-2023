// lcd4x8lib.h
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_MAX_POS	3	// maximal symbol index for our LCD

#define LCD_ALIGN_RIGHT 1
#define LCD_ALIGN_LEFT 0
	
	const char* _printables;
	uint8_t _lcd_align;

	int show_sym_by_bitmap(uint8_t bitmap, uint8_t pos);	// pos is zero-based
	int show_sym_by_asc(char sym, uint8_t pos);
	void clean_pos(uint8_t pos);
	uint8_t lcd_strlen(const char* s); // char or const char?
	bool is_printable(char c);
	int lcd_show_str(const char* s);
	int lcd_show_dot(uint8_t dp);

#ifdef __cplusplus
}
#endif
