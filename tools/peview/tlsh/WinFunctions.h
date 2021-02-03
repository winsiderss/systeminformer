#ifdef WINDOWS
#ifndef WINFUNCTIONS_H
#define WINFUNCTIONS_H

#define TLSH_LIB

#include <windows.h>
#include <stdio.h>
#include <time.h>

#ifndef TLSH_LIB
#  ifdef TLSH_EXPORTS
#  define TLSH_API __declspec(dllexport)
#  else
#  define TLSH_API __declspec(dllimport)
#  endif
#else
#  define TLSH_API
#endif

#define strdup _strdup
#define NAME_LENGTH MAX_PATH
#define snprintf _snprintf
#define strcasecmp _stricmp
#define random rand
#define srandom srand

struct dirent {
	char d_name[NAME_LENGTH];
};

typedef struct _DIR
{
    char dirname[NAME_LENGTH];
	HANDLE hFind;
	WIN32_FIND_DATA findFileData;
	struct dirent findDirEnt;
} DIR;

extern DIR *opendir(const char *dirname);
extern struct dirent *readdir(DIR *dir);
extern int closedir(DIR *dir);
extern struct tm *localtime_r(const time_t *timep, struct tm *results);
extern bool read_file_win(const char *fname, int sizefile, unsigned char* data);

#endif  // #ifndef WINFUNCTIONS_H

#endif  // #ifdef WINDOWS
