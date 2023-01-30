#include <stdio.h>
#include <string.h>
#include <widemath.h>
#include <ctype.h>
#include <stdbool.h>
#include "lcd4x8lib.h"

const char *_printables = "0123456789ABCDEFLH-%cun\xB0";
uint8_t _lcd_align = LCD_ALIGN_RIGHT;

bool is_printable(char c) {
	return (strchr(_printables, c) != NULL) ? true : false;
}
// strlen, but dot position matters
// dot is available after pos 0, 1 and 2
// dot as first symbol is not correct
uint8_t lcd_strlen(const char* s) {

	uint8_t	rz = 0, dotp = 0, count = 0;

	for (uint8_t i = 0; i < strlen(s); i++) {
		if (s[i] != '.') {
			if(is_printable(s[i]))	// is_printable
				rz++;
			if (rz == LCD_MAX_POS+2) break;	// so long
		}
		else {	// dot found
			if (i == 0) {	// dot at front of string
				dotp = 1;
			}
			else if (i < LCD_MAX_POS+1) {
				if(!dotp) dotp = i;
			}
			// if i >= 4, we don't reach a dot
		}
	}
	return (dotp << 4) | rz;
}

//TODO: если точка присутствует, включить ее в правильной позиции, скорректировать strlen, вычислить позицию первого печатного знака
// startpos = 4-lcd_strlen для правого выравнивания, 1.5 это позиция 2, точка в позиции 2

// clear all dot segments (exclude colon sign)
void lcd_clear_dots() {
	for (uint8_t i = 2; i < LCD_MAX_POS * 2 + 1; i += 2) {
		//tls_lcd_seg_set(3, i, 0);	// clear dots at segs 2, 4 and 6
	}
}

// dp = 1, значит включить после 0 позиции, перед 1-й, segm=2
int lcd_show_dot(uint8_t dp) {
	printf("show segm at %d\n", dp << 1);
	//tls_lcd_seg_set(3, dp << 1, 1);
}

char get_next_prn_sym(const char *s, uint8_t *pos) {
	uint8_t ch = s[*pos];
	(*pos)++;

	while (!is_printable(ch) && ch != 0) {
		ch = s[*pos];
		(*pos)++;
		if (ch == 0) return ch;
	}
	
	return ch;
}

int lcd_show_str(const char *s) {

	uint8_t curpos = 0;
	uint8_t n = lcd_strlen(s);
	if (n > LCD_MAX_POS + 2) {	// dot present
		lcd_clear_dots();
		lcd_show_dot(n >> 4);	// точка отображается, но без правого выравнивания
	}
	if ((n & 0xF) > LCD_MAX_POS + 1)
		printf("Warning: string will be truncated!\n");

	// if align right, calculate start_pos
	for (uint8_t i = 0; i < LCD_MAX_POS + 1; i++) {
		char sym = get_next_prn_sym(s, &curpos);		// get next printable sym from s
		if (sym == 0) return 1;
		//show_sym_by_asc(sym, i);
		printf("show %c at pos %d\n", sym, i);
	}
	return 1;
}
