/*
 * fiflegacy.c  -  Version 1.0
 * ======================================================================
 * (c)2026 by Rezar Behzad | www.FractalImageCompression.com
 *
 * Command-line tool: converts legacy Fractal Image Format files (*.fif)
 * to PNG, or shows information about them.  Uses libfiflegacy.c, which
 * must be in the same directory.  Plain standard C (C99), no other
 * dependencies, compiles on every platform with a C compiler.
 *
 * ----------------------------------------------------------------------
 * BUILD
 * ----------------------------------------------------------------------
 *   Linux / macOS / BSD:   cc -O2 -o fiflegacy fiflegacy.c
 *                          clang -O2 -o fiflegacy fiflegacy.c
 *   Windows (MSVC):        cl /O2 fiflegacy.c
 *   Windows (MinGW/clang): gcc -O2 -o fiflegacy.exe fiflegacy.c
 *
 * ----------------------------------------------------------------------
 * USAGE
 * ----------------------------------------------------------------------
 *   fiflegacy <input.fif> [output.png]   convert to PNG
 *   fiflegacy <input.fif> --info         show file information
 *   fiflegacy --help | --version
 *
 * Without output.png the input name is used with the extension .png.
 * If output ends with a path separator (/ or \) it is treated as a
 * directory and the PNG gets the input's base name.
 *
 * Exit codes:  0 success, 1..12 libfiflegacy error code (absolute value),
 *              64 wrong command-line usage.
 */
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS          /* MSVC: allow the standard fopen() */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libfiflegacy.c"

#define FIFLEGACY_VERSION_MAJOR  1
#define FIFLEGACY_VERSION_MINOR  0
#define FIFLEGACY_VERSION        "1.0"

#define EXIT_USAGE 64

static void print_copyright(void) {
    printf("(c)2026 fiflegacy v%s / libfiflegacy v%s by Rezar Behzad | www.FractalImageCompression.com\n\n",
           FIFLEGACY_VERSION, LIBFIFLEGACY_VERSION);
    fflush(stdout);                                  /* keep order with messages on stderr */
}

static void print_usage(FILE *out) {
    fprintf(out,
        "Usage:\n"
        "  fiflegacy <input.fif> [output.png]   convert a FIF file to PNG\n"
        "  fiflegacy <input.fif> --info         show information about a FIF file\n"
        "  fiflegacy --help                     show this help\n"
        "  fiflegacy --version                  show the version numbers\n"
        "\n"
        "If output.png is omitted, the input file name is used with the extension\n"
        ".png. If the output ends with / or \\ it is used as a directory.\n"
        "\n"
        "Exit codes: 0 = success, 1..12 = library error code, 64 = wrong usage\n");
}

static int str_ieq(const char *a, const char *b) {           /* case-insensitive equality */
    for (; *a && *b; a++, b++) {
        char x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x = (char)(x - 'A' + 'a');
        if (y >= 'A' && y <= 'Z') y = (char)(y - 'A' + 'a');
        if (x != y) return 0;
    }
    return *a == *b;
}

static int has_fif_extension(const char *p) {
    size_t n = strlen(p);
    return n > 4 && str_ieq(p + n - 4, ".fif");
}

static int is_sep(char c) { return c == '/' || c == '\\' || c == ':'; }

static const char *base_name(const char *p) {
    const char *b = p;
    for (; *p; p++) if (is_sep(*p)) b = p + 1;
    return b;
}

/* "dir/name.fif" -> "<prefix>name.png"; prefix NULL = keep the input directory */
static char *make_png_name(const char *input, const char *dir) {
    const char *base = base_name(input), *dot = NULL, *p;
    size_t keep, dlen = dir ? strlen(dir) : 0;
    char *out;
    for (p = base; *p; p++) if (*p == '.') dot = p;
    if (!dot || dot == base) dot = base + strlen(base);          /* no extension: append */
    keep = dir ? (size_t)(dot - base) : (size_t)(dot - input);
    out = (char *)malloc(dlen + keep + 5);
    if (!out) return NULL;
    if (dir) { memcpy(out, dir, dlen); memcpy(out + dlen, base, keep); }
    else memcpy(out, input, keep);
    memcpy(out + dlen + keep, ".png", 5);
    return out;
}

static long file_size(const char *path) {                    /* -1 if not readable */
    FILE *f = fopen(path, "rb");
    long n = -1;
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) == 0) n = ftell(f);
    fclose(f);
    return n;
}

static void print_size(const char *label, long n) {          /* 421,613 bytes (411.7 KB) */
    char digits[32], grouped[48];
    int len, i, j = 0;
    sprintf(digits, "%ld", n);
    len = (int)strlen(digits);
    for (i = 0; i < len; i++) {
        if (i && (len - i) % 3 == 0) grouped[j++] = ',';
        grouped[j++] = digits[i];
    }
    grouped[j] = 0;
    if (n >= 1024L * 1024L) printf("  %-16s %s bytes (%.2f MB)\n", label, grouped, (double)n / (1024.0 * 1024.0));
    else if (n >= 1024L) printf("  %-16s %s bytes (%.1f KB)\n", label, grouped, (double)n / 1024.0);
    else printf("  %-16s %s bytes\n", label, grouped);
}

static void print_error(int code, const char *input, const char *output) {
    fflush(stdout);
    fprintf(stderr, "Error %d: ", code);
    switch (code) {
    case FIF_ERR_EXTENSION:
        fprintf(stderr, "The input file \"%s\" does not have the extension .fif.\n"
                        "  Only files whose names end in .fif (any upper/lower case) are accepted.\n", input);
        break;
    case FIF_ERR_OPEN:
        fprintf(stderr, "The input file \"%s\" cannot be opened.\n"
                        "  Check that the file exists, the path is correct and you have read permission.\n", input);
        break;
    case FIF_ERR_UNKNOWN_FORMAT:
        fprintf(stderr, "\"%s\" is not a Fractal Image Format file (no FTC/FIF1 signature found).\n", input);
        break;
    case FIF_ERR_VIDEO:
        fprintf(stderr, "\"%s\" is a fractal VIDEO file (P.OEM or FJF video).\n"
                        "  Videos cannot be converted to PNG.\n"
                        "  Tip: \"fiflegacy %s --info\" shows details of the video.\n", input, input);
        break;
    case FIF_ERR_UNSUPPORTED:
        fprintf(stderr, "\"%s\" uses a variant of the fractal format that is not supported.\n"
                        "  Tip: \"fiflegacy %s --info\" shows which variant it is.\n", input, input);
        break;
    case FIF_ERR_MALFORMED:
        fprintf(stderr, "\"%s\" is damaged, incomplete or inconsistent (a format check failed).\n"
                        "  Tip: \"fiflegacy %s --info\" shows the readable header details.\n", input, input);
        break;
    case FIF_ERR_FTT_MISSING:
        fprintf(stderr, "\"%s\" needs an external template file (*.ftt) that was not found.\n"
                        "  Copy the .ftt file into the same directory as the .fif file.\n"
                        "  Tip: \"fiflegacy %s --info\" shows the name of the required .ftt file.\n", input, input);
        break;
    case FIF_ERR_MEMORY:
        fprintf(stderr, "Not enough memory to process \"%s\".\n", input);
        break;
    case FIF_ERR_TOO_LARGE:
        fprintf(stderr, "\"%s\" is too large (limits: 1 GiB file size, 268 million pixels).\n", input);
        break;
    case FIF_ERR_WRITE:
        fprintf(stderr, "The output file \"%s\" cannot be written.\n"
                        "  Check that the directory exists, is writable and the disk is not full.\n", output ? output : "");
        break;
    case FIF_ERR_ARGUMENT:
        fprintf(stderr, "Invalid file name.\n");
        break;
    default:
        fprintf(stderr, "Unexpected error.\n");
        break;
    }
}

int main(int argc, char **argv) {
    const char *input = NULL, *output = NULL;
    char *outbuf = NULL;
    int info = 0, i, rc, existed;
    long insize, outsize;
    FIF_RESULT *r;

    print_copyright();

    for (i = 1; i < argc; i++) {                      /* parse the command line */
        const char *a = argv[i];
        if (str_ieq(a, "--help") || str_ieq(a, "-h") || !strcmp(a, "/?")) { print_usage(stdout); return 0; }
        if (str_ieq(a, "--version") || str_ieq(a, "-v")) {
            printf("fiflegacy     %s\nlibfiflegacy  %s\n", FIFLEGACY_VERSION, LIBFIFLEGACY_VERSION);
            return 0;
        }
        if (str_ieq(a, "--info") || str_ieq(a, "-i")) { info = 1; continue; }
        if (!a[0]) {
            fprintf(stderr, "Error: empty file name given as argument %d.\n\n", i);
            print_usage(stderr);
            return EXIT_USAGE;
        }
        if (a[0] == '-' && a[1]) {
            fprintf(stderr, "Error: unknown option \"%s\".\n\n", a);
            print_usage(stderr);
            return EXIT_USAGE;
        }
        if (!input) input = a;
        else if (!output) output = a;
        else {
            fprintf(stderr, "Error: too many arguments (\"%s\").\n\n", a);
            print_usage(stderr);
            return EXIT_USAGE;
        }
    }
    if (!input) {
        fprintf(stderr, "Error: no input file given.\n\n");
        print_usage(stderr);
        return EXIT_USAGE;
    }
    if (info && output) {
        fprintf(stderr, "Error: --info does not take an output file (\"%s\").\n\n", output);
        print_usage(stderr);
        return EXIT_USAGE;
    }

    /* ---------------- information mode ---------------- */
    if (info) {
        r = ConvertLegacyFIFtoPNG(input, NULL, FIF_MODE_INFO);
        if (!r) { print_error(FIF_ERR_MEMORY, input, NULL); return -FIF_ERR_MEMORY; }
        if (r->text) fputs(r->text, stdout);
        rc = r->error;
        free(r);
        if (rc == FIF_ERR_EXTENSION || rc == FIF_ERR_OPEN || rc == FIF_ERR_MEMORY ||
            rc == FIF_ERR_TOO_LARGE || rc == FIF_ERR_ARGUMENT) {
            printf("\n");
            print_error(rc, input, NULL);
        }
        return -rc;
    }

    /* ---------------- conversion mode ---------------- */
    if (!has_fif_extension(input)) { print_error(FIF_ERR_EXTENSION, input, NULL); return -FIF_ERR_EXTENSION; }
    if (!output) output = outbuf = make_png_name(input, NULL);
    else if (is_sep(output[strlen(output) - 1])) output = outbuf = make_png_name(input, output);
    if (!output) { print_error(FIF_ERR_MEMORY, input, NULL); return -FIF_ERR_MEMORY; }
    if (str_ieq(input, output)) {
        fprintf(stderr, "Error: input and output are the same file (\"%s\").\n"
                        "  Choose a different output name, e.g. one ending in .png.\n", input);
        free(outbuf);
        return EXIT_USAGE;
    }
    existed = file_size(output) >= 0;

    printf("Converting \"%s\" -> \"%s\" ...\n", input, output);
    fflush(stdout);
    r = ConvertLegacyFIFtoPNG(input, output, FIF_MODE_FILE);
    if (!r) { print_error(FIF_ERR_MEMORY, input, output); free(outbuf); return -FIF_ERR_MEMORY; }
    rc = r->error;
    if (rc != FIF_OK) {
        print_error(rc, input, output);
        free(r);
        free(outbuf);
        return -rc;
    }
    insize = file_size(input);
    outsize = file_size(output);
    printf("Success: PNG file written%s.\n", existed ? " (existing file replaced)" : "");
    printf("  %-16s %d x %d pixels\n", "Image size:", r->width, r->height);
    print_size("FIF file size:", insize);
    print_size("PNG file size:", outsize);
    free(r);
    free(outbuf);
    return 0;
}
