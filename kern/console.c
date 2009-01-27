/* 
 * @file console.c
 * @brief Console driver
 * @author Anshuman P. Kanetkar (apk)
 *
 */
//#include <p1kern.h>
#include <simics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86/asm.h>
#include <x86/seg.h>
#include <x86/interrupt_defines.h>
#include <x86/timer_defines.h>
#include <x86/keyhelp.h>
#include <x86/video_defines.h>
#include <interrupt_wrappers.h>
#include <handler_install.h>
#include <console.h>

void xen_console_print(const char *data, int length);

/* @brief The software cursor location */
static unsigned char cons_curr_row;
static unsigned char cons_curr_col;
/* @brief Current fg/bg color */
static unsigned char cons_curr_color = CONS_DEFAULT_COLOR;
/* @brief Current cursor state (visible/hidden) */
static unsigned char cons_cursor_state = CONS_CURSOR_VISIBLE;

/**
 * @brief Initialize the console, clear it, and set default color.
 */
void
console_init()
{
  // Initialize console
  set_term_color(CONS_DEFAULT_COLOR);
  clear_console();
  show_cursor();
}


/**
 * @brief Scrolls the screen, called when current row is the bottom-most 
 * row of the screen.
 */
static inline void 
cons_scroll(void)
{
  // Scroll the display one line upwards
  memcpy(FB_LOC_CHAR(0,0), 
	 FB_LOC_CHAR(1, 0), 
	 2 * 
	 (CONSOLE_HEIGHT - 1) *
	 CONSOLE_WIDTH);
  
  int i = 0;
  unsigned short * loc = (unsigned short *)FB_LOC_CHAR(cons_curr_row,0);
  
  // Clear the last line of the screen
  for (i = 0; i < CONSOLE_WIDTH; i++) {
    //lprintf("%p: ll: 0x%04x", loc + i, *(loc + i));
    *(loc + i) = ((unsigned short)(CONS_DEFAULT_COLOR) << 8) | 
                  CONS_CHAR_SPC;
  }
  
}

/**
 * @brief clear the screen, setting the attributes to default.
 */
static inline void
cons_clear(void)
{
  int i = 0;
  unsigned short * loc = (unsigned short *)FB_LOC_CHAR(0, 0);
  
  // Clear the last line of the screent
  for (i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
    //lprintf("ll: 0x%04x", *(loc + i));
    *(loc + i) = ((unsigned short)(CONS_DEFAULT_COLOR) << 8) | 
      CONS_CHAR_SPC;
  }
  
}

/**
 * @brief Draw the character with specified attributes at the given
 * coordinates.
 */
static inline void
cons_draw_char(unsigned int row, 
	   unsigned int col,
	   char ch, 
	   unsigned char color)
{
  char * cursor = FB_LOC_CHAR(row, col);

  *(cursor + 1) = color;

  *cursor = ch;
  //lprintf("Printed: [%p] '%c'", cursor, *cursor);  
  //lprintf("Color  : [%p] 0x%02x", cursor + 1, *(cursor + 1));
}

/**
 * @brief Writes a character to the display, performing
 * special processing for CR,LF and  BKSPC, and word wrap.
 *
 * Backspace deletes the previous character (including attributes)
 * upto the beginning of the line.
 * CR/LF behaves as defined in the spec, screen scrolls if at the bottom-
 * most line.
 *
 */
static inline void 
cons_putbyte(char ch)
{
  
  /* Print the character */
  if (ch != CONS_CHAR_BKSPC && 
      ch != CONS_CHAR_CR &&
      ch != CONS_CHAR_LF) {
    cons_draw_char(cons_curr_row, 
	       cons_curr_col, 
	       ch, 
	       cons_curr_color);
  }

  /* If backspace, remove the previous character */
  if (ch == CONS_CHAR_BKSPC) {

    if (cons_curr_col > 0) {
      cons_curr_col--;
      cons_draw_char(cons_curr_row, 
		     cons_curr_col,
		     CONS_CHAR_SPC,
		     CONS_DEFAULT_COLOR);
    } 
    
  } else if (ch == CONS_CHAR_CR) {
    /* for CR, move to beginning of this line */
    cons_curr_col = 0;
  } else if (ch != CONS_CHAR_LF &&
	     cons_curr_col < (CONSOLE_WIDTH - 1)) {
    /* If not LF, and no word wrap, move to next column */
    cons_curr_col++;
  } else { 
    /* LF encountered, or we are at the last column of the row */
    cons_curr_col = 0;
    if (cons_curr_row < (CONSOLE_HEIGHT - 1)) {
      cons_curr_row++;
    } else {
      /* Scroll the screen up if this is the last row */
      cons_scroll();
    }
  }

}

/**
 * @brief programs the CRTC to hide the cursor, and marks it
 * as hidden
 */
static inline void
cons_hide_cursor(void)
{

  cons_cursor_state = CONS_CURSOR_HIDDEN;

  /* Write out-of-range offsets into LSB and MSB */
  outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
  outb(CRTC_DATA_REG, 0xff);
  outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
  outb(CRTC_DATA_REG, 0xff);
    
}

/**
 * @brief program the CRTC to display the cursor.
 */
static inline void
cons_display_cursor(void)
{

  /* calculate cursor offset */
  unsigned short curpos = cons_curr_row * CONSOLE_WIDTH + cons_curr_col;
  unsigned char * pcurpos = (unsigned char *)&curpos;

  //lprintf("Displcur: %d", curpos);

  /* Instructions to show the cursor */

  /* Write cursor offset LSB */
  outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
  outb(CRTC_DATA_REG, *pcurpos);

  /* Write cursor offset MSB */
  pcurpos++;
  outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
  outb(CRTC_DATA_REG, *pcurpos);

  //MAGIC_BREAK;
}

/**
 * @brief Display the cursor on the screen, and
 * mark it visible.
 */
static inline void
cons_show_cursor(void)
{
  cons_cursor_state = CONS_CURSOR_VISIBLE;
  cons_display_cursor();
}

/**
 * @brief Update the hardware cursor, displaying it if
 * it is marked visible.
 */
static inline void
cons_update_cursor(void)
{
  if (cons_cursor_state == CONS_CURSOR_VISIBLE) {
    cons_display_cursor();
  }
}

void 
draw_char(int row,
	  int col,
	  int ch,
	  int color)
{
  cons_draw_char((unsigned int)row,
		 (unsigned int)col,
		 (unsigned char)ch,
		 (unsigned char)color);
}

int
putbyte(char ch)
{
  char buf[1];
  buf[0] = ch;
  xen_console_print(buf, 1);
  return ch;
#if 0
  cons_putbyte(ch);
  //MAGIC_BREAK;
  /* Update hw cursor */
  cons_update_cursor();
#endif
  return ch;
}

/**
 * @brief writes the given string to current cursor position,
 * with current color settings, updating the cursor position 
 *
 */
void 
putbytes(const char * str, int length)
{
  

  xen_console_print(str, length);
#if 0
  int i = 0;
  for (i = 0; i < length; i++) {
    cons_putbyte(*(str + i));
  } 
  /* Update hw cursor */
  cons_update_cursor();
#endif
}

char
get_char(int row,
	 int col)
{
  return *((char *)FB_LOC_CHAR(row, col));
}

int
set_term_color(int color)
{
  // Check if the color is valid
  if (CONS_IS_VALID_COLOR(color)) {
    cons_curr_color = (unsigned char)color;
    return 0;
  }
  //lprintf("Invalid Color : 0x%08x", color);
  return -1;
}

void
get_term_color(int * color)
{
  *color = (((int)cons_curr_color) & 0xffUL);
}

int
set_cursor(int row, 
	   int col)
{ 
 
  if (row < 0 ||
      row >= CONSOLE_HEIGHT ||
      col < 0 ||
      col >= CONSOLE_WIDTH) {
    return -1;
  }

  cons_curr_row = (unsigned char)row;
  cons_curr_col = (unsigned char)col;

  /* Update hw cursor */
  cons_update_cursor();

  return 0;
}

void
get_cursor(int * row, 
	   int * col)
{
  *row = ((int)cons_curr_row & 0xffUL);
  *col = ((int)cons_curr_col & 0xffUL);
}

/**
 * @brief Clear console, and set cursor location
 * to top-left corner of the screen.
 */
void
clear_console()
{
  cons_clear();
  
  cons_curr_row = 0;
  cons_curr_col = 0;
  
  /* Update hw cursor */
  cons_update_cursor();
}

void
show_cursor()
{
  cons_show_cursor();
}

void
hide_cursor()
{
  cons_hide_cursor();
}

/**
 * @brief A test routine for the console. Tests
 * scrolling, rapping, cursor control and colors. 
 */
void
test_console()
{
  
  
  putbyte('A');
  putbyte('B');
  putbyte('c');
  putbyte('d');

  putbyte(' ');
  
  putbytes("0123456789", 10);
  
  putbyte('\b');
  
  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);

  putbyte('\n');

  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);

  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);
  
  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);
  putbytes("0123456789 ", 11);
  
  
  int i = 0;
  set_term_color(FGND_GREEN|BGND_RED); 
  for (i = 0; i < CONSOLE_HEIGHT/2 + 2; i++) {
    putbytes("ababababababababababababababa\n", 30);
    putbytes("zzzzzzzzzzzzzzzzzzzzzzzzzzzzz\n", 30);
  }
  putbytes("33333333333333333333333333333\n", 30);
  //set_cursor(10, 10);
  
}
