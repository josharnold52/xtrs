#ifdef __cplusplus
extern "C"
{
#endif

void vga_needs_reset();
void vga_screen_write_glyph_64_16(char *glyphRows, int position);
void vga_screen_scroll_64_16();
#ifdef __cplusplus
} // extern "C"
#endif
