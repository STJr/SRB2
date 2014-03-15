/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Some definitions for internal use by the library code.
 *      This should not be included by user programs.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef INTERNAL_H
#define INTERNAL_H

#include "allegro.h"

/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Some definitions for internal use by the library code.
 *      This should not be included by user programs.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef INTERNDJ_H
#define INTERNDJ_H

#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif


#include <dos.h>


/* file access macros */
#define FILE_OPEN(filename, handle)             handle = open(filename, O_RDONLY | O_BINARY, S_IRUSR | S_IWUSR)
#define FILE_CREATE(filename, handle)           handle = open(filename, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)
#define FILE_CLOSE(handle)                      close(handle)
#define FILE_READ(handle, buf, size, sz)        sz = read(handle, buf, size)
#define FILE_WRITE(handle, buf, size, sz)       sz = write(handle, buf, size) 
#define FILE_SEARCH_STRUCT                      struct ffblk
#define FILE_FINDFIRST(filename, attrib, dta)   findfirst(filename, dta, attrib)
#define FILE_FINDNEXT(dta)                      findnext(dta)
#define FILE_ATTRIB                             ff_attrib
#define FILE_SIZE                               ff_fsize
#define FILE_NAME                               ff_name
#define FILE_TIME                               ff_ftime
#define FILE_DATE                               ff_fdate


/* macros to enable and disable interrupts */
#define DISABLE()   asm volatile ("cli")
#define ENABLE()    asm volatile ("sti")


__INLINE__ void enter_critical(void) 
{
   if (windows_version >= 3) {
      __dpmi_regs r;
      r.x.ax = 0x1681; 
      __dpmi_int(0x2F, &r);
   }

   DISABLE();
}


__INLINE__ void exit_critical(void) 
{
   if (windows_version >= 3) {
      __dpmi_regs r;
      r.x.ax = 0x1682; 
      __dpmi_int(0x2F, &r);
   }

   ENABLE();
}


/* interrupt hander stuff */
#define _map_irq(irq)   (((irq)>7) ? ((irq)+104) : ((irq)+8))

int _install_irq(int num, int (*handler)(void));
void _remove_irq(int num);
void _restore_irq(int irq);
void _enable_irq(int irq);
void _disable_irq(int irq);

#define _eoi(irq) { outportb(0x20, 0x20); if ((irq)>7) outportb(0xA0, 0x20); }

typedef struct _IRQ_HANDLER
{
   int (*handler)(void);         /* our C handler */
   int number;                   /* irq number */
   __dpmi_paddr old_vector;      /* original protected mode vector */
} _IRQ_HANDLER;


/* DPMI memory mapping routines */
int _create_physical_mapping(unsigned long *linear, int *segment, unsigned long physaddr, int size);
void _remove_physical_mapping(unsigned long *linear, int *segment);
int _create_linear_mapping(unsigned long *linear, unsigned long physaddr, int size);
void _remove_linear_mapping(unsigned long *linear);
int _create_selector(int *segment, unsigned long linear, int size);
void _remove_selector(int *segment);
void _unlock_dpmi_data(void *addr, int size);


/* bank switching routines */
void _accel_bank_stub(void);
void _accel_bank_stub_end(void);

void _accel_bank_switch (void);
void _accel_bank_switch_end(void);

void _vesa_window_1(void);
void _vesa_window_1_end(void);
void _vesa_window_2(void);
void _vesa_window_2_end(void);

void _vesa_pm_window_1(void);
void _vesa_pm_window_1_end(void);
void _vesa_pm_window_2(void);
void _vesa_pm_window_2_end(void);

void _vesa_pm_es_window_1(void);
void _vesa_pm_es_window_1_end(void);
void _vesa_pm_es_window_2(void);
void _vesa_pm_es_window_2_end(void);


/* stuff for the VESA and VBE/AF drivers */
extern __dpmi_regs _dpmi_reg;

extern int _window_2_offset;

extern void (*_pm_vesa_switcher)(void);
extern void (*_pm_vesa_scroller)(void);
extern void (*_pm_vesa_pallete)(void);

extern int _mmio_segment;

extern void *_accel_driver;

extern int _accel_active;

extern void *_accel_set_bank;
extern void *_accel_idle;

void _fill_vbeaf_libc_exports(void *ptr);
void _fill_vbeaf_pmode_exports(void *ptr);


/* sound lib stuff */
extern int _fm_port;
extern int _mpu_port;
extern int _mpu_irq;
extern int _sb_freq;
extern int _sb_port; 
extern int _sb_dma; 
extern int _sb_irq; 

int _sb_read_dsp_version(void);
int _sb_reset_dsp(int data);
void _sb_voice(int state);
int _sb_set_mixer(int digi_volume, int midi_volume);

void _mpu_poll(void);

int _dma_allocate_mem(int bytes, int *sel, unsigned long *phys);
void _dma_start(int channel, unsigned long addr, int size, int auto_init, int input);
void _dma_stop(int channel);
unsigned long _dma_todo(int channel);
void _dma_lock_mem(void);


#endif          /* ifndef INTERNDJ_H */


/* flag for how many times we have been initialised */
extern int _allegro_count;


/* some Allegro functions need a block of scratch memory */
extern void *_scratch_mem;
extern int _scratch_mem_size;

__INLINE__ void _grow_scratch_mem(int size)
{
   if (size > _scratch_mem_size) {
      size = (size+1023) & 0xFFFFFC00;
      _scratch_mem = realloc(_scratch_mem, size);
      _scratch_mem_size = size;
   }
}


/* list of functions to call at program cleanup */
void _add_exit_func(void (*func)(void));
void _remove_exit_func(void (*func)(void));


/* reads a translation file into memory */
void _load_config_text(void);


/* various bits of mouse stuff */
void _set_mouse_range(void);
extern BITMAP *_mouse_screen;
extern BITMAP *_mouse_sprite, *_mouse_pointer;
extern int _mouse_x_focus, _mouse_y_focus;
extern int _mouse_width, _mouse_height;


/* various bits of timer stuff */
extern int _timer_use_retrace;
extern volatile int _retrace_hpp_value;


/* caches and tables for svga bank switching */
extern int _last_bank_1, _last_bank_2; 
extern int *_gfx_bank; 


/* bank switching routines */
void _stub_bank_switch (void);
void _stub_bank_switch_end(void);


/* stuff for setting up bitmaps */
void _check_gfx_virginity(void);
BITMAP *_make_bitmap(int w, int h, unsigned long addr, GFX_DRIVER *driver, int color_depth, int bpl);
void _sort_out_virtual_width(int *width, GFX_DRIVER *driver);

GFX_VTABLE *_get_vtable(int color_depth);

extern GFX_VTABLE _screen_vtable;

extern int _sub_bitmap_id_count;

extern int _textmode;

#define BYTES_PER_PIXEL(bpp)     (((int)(bpp) + 7) / 8)

int _color_load_depth(int depth);

extern int _color_conv;

BITMAP *_fixup_loaded_bitmap(BITMAP *bmp, PALETTE pal, int bpp);


/* VGA register access routines */
void _vga_vsync(void);
void _vga_set_pallete_range(PALLETE p, int from, int to, int vsync);

extern int _crtc;


/* _read_vga_register:
 *  Reads the contents of a VGA register.
 */
__INLINE__ int _read_vga_register(int port, int index)
{
   if (port==0x3C0)
      inportb(_crtc+6); 

   outportb(port, index);
   return inportb(port+1);
}


/* _write_vga_register:
 *  Writes a byte to a VGA register.
 */
__INLINE__ void _write_vga_register(int port, int index, int v) 
{
   if (port==0x3C0) {
      inportb(_crtc+6);
      outportb(port, index);
      outportb(port, v);
   }
   else {
      outportb(port, index);
      outportb(port+1, v);
   }
}


/* _alter_vga_register:
 *  Alters specific bits of a VGA register.
 */
__INLINE__ void _alter_vga_register(int port, int index, int mask, int v)
{
   int temp;
   temp = _read_vga_register(port, index);
   temp &= (~mask);
   temp |= (v & mask);
   _write_vga_register(port, index, temp);
}


/* _vsync_out_h:
 *  Waits until the VGA is not in either a vertical or horizontal retrace.
 */
__INLINE__ void _vsync_out_h(void)
{
   do {
   } while (inportb(0x3DA) & 1);
}


/* _vsync_out_v:
 *  Waits until the VGA is not in a vertical retrace.
 */
__INLINE__ void _vsync_out_v(void)
{
   do {
   } while (inportb(0x3DA) & 8);
}


/* _vsync_in:
 *  Waits until the VGA is in the vertical retrace period.
 */
__INLINE__ void _vsync_in(void)
{
   if (_timer_use_retrace) {
      int t = retrace_count; 

      do {
      } while (t == retrace_count);
   }
   else {
      do {
      } while (!(inportb(0x3DA) & 8));
   }
}


/* _write_hpp:
 *  Writes to the VGA pelpan register.
 */
__INLINE__ void _write_hpp(int value)
{
   if (_timer_use_retrace) {
      _retrace_hpp_value = value;

      do {
      } while (_retrace_hpp_value == value);
   }
   else {
      do {
      } while (!(inportb(0x3DA) & 8));

      _write_vga_register(0x3C0, 0x33, value);
   }
}


void _set_vga_virtual_width(int old_width, int new_width);


/* current drawing mode */
extern int _drawing_mode;
extern BITMAP *_drawing_pattern;
extern int _drawing_x_anchor;
extern int _drawing_y_anchor;
extern unsigned int _drawing_x_mask;
extern unsigned int _drawing_y_mask;


/* graphics drawing routines */
void _normal_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
void _normal_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);

int  _linear_getpixel8(struct BITMAP *bmp, int x, int y);
void _linear_putpixel8(struct BITMAP *bmp, int x, int y, int color);
void _linear_vline8(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _linear_hline8(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _linear_draw_sprite8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_v_flip8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_h_flip8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_vh_flip8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_trans_sprite8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_lit_sprite8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_draw_rle_sprite8(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_trans_rle_sprite8(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_lit_rle_sprite8(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);
void _linear_draw_character8(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_textout_fixed8(struct BITMAP *bmp, void *f, int h, unsigned char *str, int x, int y, int color);
void _linear_blit8(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_blit_backward8(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_masked_blit8(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_clear_to_color8(struct BITMAP *bitmap, int color);

#ifdef ALLEGRO_COLOR16

void _linear_putpixel15(struct BITMAP *bmp, int x, int y, int color);
void _linear_vline15(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _linear_hline15(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _linear_draw_trans_sprite15(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_lit_sprite15(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_draw_rle_sprite15(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_trans_rle_sprite15(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_lit_rle_sprite15(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);

int  _linear_getpixel16(struct BITMAP *bmp, int x, int y);
void _linear_putpixel16(struct BITMAP *bmp, int x, int y, int color);
void _linear_vline16(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _linear_hline16(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _linear_draw_sprite16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_256_sprite16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_v_flip16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_h_flip16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_vh_flip16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_trans_sprite16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_lit_sprite16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_draw_rle_sprite16(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_trans_rle_sprite16(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_lit_rle_sprite16(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);
void _linear_draw_character16(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_textout_fixed16(struct BITMAP *bmp, void *f, int h, unsigned char *str, int x, int y, int color);
void _linear_blit16(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_blit_backward16(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_masked_blit16(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_clear_to_color16(struct BITMAP *bitmap, int color);

#endif

#ifdef ALLEGRO_COLOR24

int  _linear_getpixel24(struct BITMAP *bmp, int x, int y);
void _linear_putpixel24(struct BITMAP *bmp, int x, int y, int color);
void _linear_vline24(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _linear_hline24(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _linear_draw_sprite24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_256_sprite24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_v_flip24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_h_flip24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_vh_flip24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_trans_sprite24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_lit_sprite24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_draw_rle_sprite24(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_trans_rle_sprite24(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_lit_rle_sprite24(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);
void _linear_draw_character24(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_textout_fixed24(struct BITMAP *bmp, void *f, int h, unsigned char *str, int x, int y, int color);
void _linear_blit24(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_blit_backward24(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_masked_blit24(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_clear_to_color24(struct BITMAP *bitmap, int color);

#endif

#ifdef ALLEGRO_COLOR32

int  _linear_getpixel32(struct BITMAP *bmp, int x, int y);
void _linear_putpixel32(struct BITMAP *bmp, int x, int y, int color);
void _linear_vline32(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _linear_hline32(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _linear_draw_sprite32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_256_sprite32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_v_flip32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_h_flip32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_sprite_vh_flip32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_trans_sprite32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _linear_draw_lit_sprite32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_draw_rle_sprite32(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_trans_rle_sprite32(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _linear_draw_lit_rle_sprite32(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);
void _linear_draw_character32(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _linear_textout_fixed32(struct BITMAP *bmp, void *f, int h, unsigned char *str, int x, int y, int color);
void _linear_blit32(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_blit_backward32(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_masked_blit32(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _linear_clear_to_color32(struct BITMAP *bitmap, int color);

#endif

int  _x_getpixel(struct BITMAP *bmp, int x, int y);
void _x_putpixel(struct BITMAP *bmp, int x, int y, int color);
void _x_vline(struct BITMAP *bmp, int x, int y1, int y2, int color);
void _x_hline(struct BITMAP *bmp, int x1, int y, int x2, int color);
void _x_draw_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _x_draw_sprite_v_flip(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _x_draw_sprite_h_flip(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _x_draw_sprite_vh_flip(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _x_draw_trans_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y);
void _x_draw_lit_sprite(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _x_draw_rle_sprite(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _x_draw_trans_rle_sprite(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y);
void _x_draw_lit_rle_sprite(struct BITMAP *bmp, struct RLE_SPRITE *sprite, int x, int y, int color);
void _x_draw_character(struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color);
void _x_textout_fixed(struct BITMAP *bmp, void *f, int h, unsigned char *str, int x, int y, int color);
void _x_blit_from_memory(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_blit_to_memory(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_blit(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_blit_forward(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_blit_backward(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_masked_blit(struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void _x_clear_to_color(struct BITMAP *bitmap, int color);


/* asm helper for stretch_blit() */
void _do_stretch(BITMAP *source, BITMAP *dest, void *drawer, int sx, fixed sy, fixed syd, int dx, int dy, int dh, int color_depth);


/* number of fractional bits used by the polygon rasteriser */
#define POLYGON_FIX_SHIFT     18


/* bitfield specifying which polygon attributes need interpolating */
#define INTERP_FLAT           1
#define INTERP_1COL           2
#define INTERP_3COL           4
#define INTERP_FIX_UV         8
#define INTERP_Z              16
#define INTERP_FLOAT_UV       32
#define OPT_FLOAT_UV_TO_FIX   64
#define COLOR_TO_RGB          128


/* information for polygon scanline fillers */
typedef struct POLYGON_SEGMENT
{
   fixed u, v, du, dv;              /* fixed point u/v coordinates */
   fixed c, dc;                     /* single color gouraud shade values */
   fixed r, g, b, dr, dg, db;       /* RGB gouraud shade values */
   float z, dz;                     /* polygon depth (1/z) */
   float fu, fv, dfu, dfv;          /* floating point u/v coordinates */
   unsigned char *texture;          /* the texture map */
   int umask, vmask, vshift;        /* texture map size information */
   int seg;                         /* destination bitmap selector */
} POLYGON_SEGMENT;


/* an active polygon edge */
typedef struct POLYGON_EDGE 
{
   int top;                         /* top y position */
   int bottom;                      /* bottom y position */
   fixed x, dx;                     /* fixed point x position and gradient */
   fixed w;                         /* width of line segment */
   POLYGON_SEGMENT dat;             /* texture/gouraud information */
   struct POLYGON_EDGE *prev;       /* doubly linked list */
   struct POLYGON_EDGE *next;
} POLYGON_EDGE;


/* prototype for the scanline filler functions */
typedef void (*SCANLINE_FILLER)(unsigned long addr, int w, POLYGON_SEGMENT *info);


/* polygon helper functions */
extern SCANLINE_FILLER _optim_alternative_drawer;
POLYGON_EDGE *_add_edge(POLYGON_EDGE *list, POLYGON_EDGE *edge, int sort_by_x);
POLYGON_EDGE *_remove_edge(POLYGON_EDGE *list, POLYGON_EDGE *edge);
void _fill_3d_edge_structure(POLYGON_EDGE *edge, V3D *v1, V3D *v2, int flags, BITMAP *bmp);
void _fill_3d_edge_structure_f(POLYGON_EDGE *edge, V3D_f *v1, V3D_f *v2, int flags, BITMAP *bmp);
SCANLINE_FILLER _get_scanline_filler(int type, int *flags, POLYGON_SEGMENT *info, BITMAP *texture, BITMAP *bmp);
void _clip_polygon_segment(POLYGON_SEGMENT *info, int gap, int flags);


/* polygon scanline filler functions */
void _poly_scanline_gcol8(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_grgb8(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex8(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex8(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask8(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask8(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_lit8(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_lit8(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask_lit8(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit8(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_grgb8x(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_grgb15(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask15(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask15(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_lit15(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_lit15(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask_lit15(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit15(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_grgb15x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_lit15x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_lit15x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask_lit15x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit15x(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_ptex_lit15d(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit15d(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_grgb16(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex16(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex16(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask16(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask16(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_lit16(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_lit16(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask_lit16(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit16(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_grgb16x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_lit16x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_lit16x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask_lit16x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit16x(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_ptex_lit16d(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit16d(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_grgb24(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex24(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex24(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask24(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask24(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_lit24(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_lit24(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask_lit24(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit24(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_grgb24x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_lit24x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_lit24x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask_lit24x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit24x(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_ptex_lit24d(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit24d(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_grgb32(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex32(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex32(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask32(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask32(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_lit32(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_lit32(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask_lit32(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit32(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_grgb32x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_lit32x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_lit32x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_atex_mask_lit32x(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit32x(unsigned long addr, int w, POLYGON_SEGMENT *info);

void _poly_scanline_ptex_lit32d(unsigned long addr, int w, POLYGON_SEGMENT *info);
void _poly_scanline_ptex_mask_lit32d(unsigned long addr, int w, POLYGON_SEGMENT *info);


/* sound lib stuff */
extern int _digi_volume;
extern int _midi_volume;
extern int _flip_pan; 
extern int _sound_hq;

extern int (*_midi_init)(void);
extern void (*_midi_exit)(void);

int _midi_allocate_voice(int min, int max);

extern volatile long _midi_tick;

int _digmid_find_patches(char *dir, char *file);

#define VIRTUAL_VOICES  256


typedef struct          /* a virtual (as seen by the user) soundcard voice */
{
   SAMPLE *sample;      /* which sample are we playing? (NULL = free) */
   int num;             /* physical voice number (-1 = been killed off) */
   int autokill;        /* set to free the voice when the sample finishes */
   long time;           /* when we were started (for voice allocation) */
   int priority;        /* how important are we? */
} VOICE;

extern VOICE _voice[VIRTUAL_VOICES];


typedef struct          /* a physical (as used by hardware) soundcard voice */
{
   int num;             /* the virtual voice currently using me (-1 = free) */
   int playmode;        /* are we looping? */
   int vol;             /* current volume (fixed point .12) */
   int dvol;            /* volume delta, for ramping */
   int target_vol;      /* target volume, for ramping */
   int pan;             /* current pan (fixed point .12) */
   int dpan;            /* pan delta, for sweeps */
   int target_pan;      /* target pan, for sweeps */
   int freq;            /* current frequency (fixed point .12) */
   int dfreq;           /* frequency delta, for sweeps */
   int target_freq;     /* target frequency, for sweeps */
} PHYS_VOICE;

extern PHYS_VOICE _phys_voice[DIGI_VOICES];


#define MIXER_DEF_SFX               8
#define MIXER_MAX_SFX               64

int _mixer_init(int bufsize, int freq, int stereo, int is16bit, int *voices);
void _mixer_exit(void);
void _mix_some_samples(unsigned long buf, unsigned short seg, int issigned);

void _mixer_init_voice(int voice, SAMPLE *sample);
void _mixer_release_voice(int voice);
void _mixer_start_voice(int voice);
void _mixer_stop_voice(int voice);
void _mixer_loop_voice(int voice, int loopmode);
int  _mixer_get_position(int voice);
void _mixer_set_position(int voice, int position);
int  _mixer_get_volume(int voice);
void _mixer_set_volume(int voice, int volume);
void _mixer_ramp_volume(int voice, int time, int endvol);
void _mixer_stop_volume_ramp(int voice);
int  _mixer_get_frequency(int voice);
void _mixer_set_frequency(int voice, int frequency);
void _mixer_sweep_frequency(int voice, int time, int endfreq);
void _mixer_stop_frequency_sweep(int voice);
int  _mixer_get_pan(int voice);
void _mixer_set_pan(int voice, int pan);
void _mixer_sweep_pan(int voice, int time, int endpan);
void _mixer_stop_pan_sweep(int voice);
void _mixer_set_echo(int voice, int strength, int delay);
void _mixer_set_tremolo(int voice, int rate, int depth);
void _mixer_set_vibrato(int voice, int rate, int depth);

/* dummy functions for the NoSound drivers */
int  _dummy_detect(int input);
int  _dummy_init(int input, int voices);
void _dummy_exit(int input);
int  _dummy_mixer_volume(int volume);
void _dummy_init_voice(int voice, SAMPLE *sample);
void _dummy_noop1(int p);
void _dummy_noop2(int p1, int p2);
void _dummy_noop3(int p1, int p2, int p3);
int  _dummy_get_position(int voice);
int  _dummy_get(int voice);
void _dummy_raw_midi(unsigned char data);
int  _dummy_load_patches(char *patches, char *drums);
void _dummy_adjust_patches(char *patches, char *drums);
void _dummy_key_on(int inst, int note, int bend, int vol, int pan);


/* from djgpp's libc, needed to find which directory we were run from */
extern int __crt0_argc;
extern char **__crt0_argv;


#endif          /* ifndef INTERNAL_H */
