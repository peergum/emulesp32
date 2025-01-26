#include "stdio.h"
#include "string.h"
#include "font.c"

FILE *fp;

int main() {
  fp = fopen("apple_font.c", "wt");
  fprintf(fp, "#include \"stdint.h\"\n\nconst uint8_t appleFont[] = {\n");
  for (int i = 0; i < 64; i++) {
    char sRow[80] = "  ";
    for (int r = 0; r < 8; r++) {
      const char *row = font[i*8 + r];
      int b = 0;
      for (int c = 0; c < 7; c++) {
        b <<= 1;
        b |= (row[c] == '*' ? 1 : 0);
      }
      char code[10];
      sprintf(code, "0x%02x", b);
      if (r>0) {
        strcat(sRow, ", ");
      }
      strcat(sRow, code);
    }
    if  (i<63) {
      strcat(sRow, ",");
    }
    sprintf(sRow, "%-40s // character %02x [%c]\n", sRow, i, i<32 ? 64+i : i);
    fprintf(fp, "%s", sRow);
  }
  fprintf(fp, "};\n");
  fclose(fp);
}