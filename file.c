#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "error.h"
#include "xalloc.h"
#include "file.h"

bool check_file(const char *path)
{
	struct stat sb;
	return stat(path, &sb) == 0 && S_ISREG(sb.st_mode);
}

bool check_directory(const char *path)
{
	struct stat sb;
	return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}

char *file_to_string(const char *filePath)
{
	FILE *file = fopen(filePath, "rb");
	if (!file)
	{
		error("could not open %s.", filePath);
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	if (fileSize == -1L)
	{
		error("could not get size of %s.", filePath);
	}
	fileSize++; // for '\0'
	rewind(file);

	char *fileString = xmalloc(sizeof (*fileString) * fileSize);
	size_t result = fread(fileString, sizeof (*fileString), fileSize - 1, file);
	if (result != (unsigned long)fileSize - 1)
	{
		error("could not read %s.", filePath);
	}
	fileString[fileSize - 1] = '\0';

	fclose(file);

	return fileString;
}