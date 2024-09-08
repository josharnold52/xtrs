#ifdef __cplusplus
extern "C"
{
#endif

void vga_needs_reset();
void vga_screen_write_glyph_64_16(char *glyphRows, int position);
void vga_screen_write_glyph_64_16_8ppc(char *glyphRows, int position);
void vga_screen_write_glyph_80_24_8ppc(char *glyphRows, int position);
void vga_screen_scroll_64_16();

void vga_set_hires();
void vga_set_text();
void vga_hires_set(int x, int y, unsigned char data);
unsigned char vga_hires_get(int x, int y);

#ifdef __cplusplus
} // extern "C"
#endif
