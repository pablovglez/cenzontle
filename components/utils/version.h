#ifndef __VERSION_H__
#define __VERSION_H__

#include <stddef.h>

#define VERSION_STRING_SIZE 12 // Minimum size for the version string, including null terminator

void show_version();
int get_version_string(char* buffer, size_t buffer_size);

#endif //__VERSION_H__