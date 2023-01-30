#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wm_type_def.h"
#include "wm_lcd.h"


uint8_t _segs[] = {0,1,1,1,0,0,1};
uint8_t _bitmaps[] = {0x3F, 6, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 7, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x38, 0x76, 0x40, 0x6B, 0x58, 0x1C, 0x54, 0x63};
const char *_printables = "0123456789ABCDEFLH-%cun\xB0";

// 0110(6) 1011011(5B) 1001111(4F) 1100110(66) 1101101(6D) 1111101(7D) 111(7) 1111111(7f) 1101111(6f)
// 1110111(77) 1111100(7C) 111001(39) 1011110(5e) 1111001(79) 1110001(71) 111000(38) 1110110(76) 
// 1000000(40) 1101011(6B) 1011000(58) 11100(1C) 1010100(54) 1100011(63)

uint8_t com_by_seg(uint8_t seg){
	uint8_t coms[] = {0,0,2,3,2,1,1};
	if(seg > 7) return -1;		// bad segment
	return coms[seg];
}

// pos is one-based
int show_sym_by_bitmap(uint8_t bitmap, uint8_t pos){
	uint8_t run = 1;

	if(pos > 4) return -1;
	for(uint8_t i=0; i<7; ++i){
		if(bitmap & run){
			uint8_t mycom = com_by_seg(i);
			uint8_t myseg = (pos << 1) + _segs[i];

			// printf("i:%d, com:%d, seg:%d\n", i, mycom, myseg); // DEBUG line
			tls_lcd_seg_set(mycom, myseg, 1);	// zero-based!
		}
		run <<= 1;
	}
	return 1;	// ok
}

int show_sym_by_asc(char sym, uint8_t pos){
	uint8_t idx, bmp;

	char *sp = strchr(_printables, sym);
	if(sp){
		idx = sp - _printables;
		bmp = _bitmaps[idx];
		printf("char: %c, idx:%d, bmp:%x\n", sym, idx, bmp);
		show_sym_by_bitmap(bmp, pos);
	}
	else{
		printf("%c: non-printable for LCD\n", sym);
		return -1;		// non-printable symbol
	}
	return 1;
}

// erase one position
// pos must be zero-based
void clean_pos(uint8_t pos){
	for(uint8_t i =0; i<7; i++){
		tls_lcd_seg_set(com_by_seg(i), (pos<<1) + _segs[i], 0);
	}
}

// strlen, but dot position matters
uint8_t lcd_strlen(char *s){

	uint8_t	rz = 0, dotp = 0;

	for(uint8_t i=0; i< strlen(s); i++){
		if(s[i] != '.'){
			rz++;
			if(rz == 5) break;	// so long
		}
		else{	// dot found
			if(i==0){	// dot at front of string
				dotp = 1;
			} else if(i < 4){
				dotp = i;
			}
			// if i >= 4, we don't reach a dot
		}
	}
	return (dotp << 4) | rz;
}

//TODO: если точка присутствует, включить ее в правильной позиции, скорректировать strlen, вычислить позицию первого печатного знака
// startpos = 4-lcd_strlen для правого выравнивания, 1.5 это позиция 2, точка в позиции 2
