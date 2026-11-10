#include <Carbon/Carbon.h>
#include <Cocoa/Cocoa.h>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <sys/stat.h>
#include <cstdlib>

inline FILE *ownership_fopen(const char *, const char *) { return NULL; }
inline int ownership_fclose(FILE *) { std::abort(); }
inline size_t ownership_fwrite(const void *, size_t, size_t, FILE *) { std::abort(); }
inline size_t ownership_fread(void *, size_t, size_t, FILE *) { std::abort(); }
inline int ownership_fgetpos(FILE *, fpos_t *) { std::abort(); }
inline int ownership_fsetpos(FILE *, const fpos_t *) { std::abort(); }
inline int ownership_fseek(FILE *, long, int) { std::abort(); }
inline long ownership_ftell(FILE *) { std::abort(); }
inline int ownership_feof(FILE *) { std::abort(); }

#define fopen ownership_fopen
#define fclose ownership_fclose
#define fwrite ownership_fwrite
#define fread ownership_fread
#define fgetpos ownership_fgetpos
#define fsetpos ownership_fsetpos
#define fseek ownership_fseek
#define ftell ownership_ftell
#define feof ownership_feof
