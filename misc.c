
//**************************************************************************
//**
//** misc.c
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#ifdef __NeXT__
#include <libc.h>
#else
#include <fcntl.h>
#include <stdlib.h>
#ifndef unix
#include <io.h>
#endif
#endif
#ifdef __GNUC__
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "misc.h"
#include "error.h"

// MACROS ------------------------------------------------------------------

#ifndef O_BINARY
#define O_BINARY 0
#endif

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern boolean acs_BigEndianHost;
extern boolean acs_VerboseMode;
extern boolean acs_DebugMode;
extern FILE *acs_DebugFile;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// MS_Alloc
//
//==========================================================================

void *MS_Alloc(size_t size, error_t error)
{
	void *mem;

	if((mem = malloc(size)) == NULL)
	{
		ERR_Exit(error, NO);
	}
	return mem;
}

//==========================================================================
//
// MS_Realloc
//
//==========================================================================

void *MS_Realloc(void *base, size_t size, error_t error)
{
	void *mem;

	if((mem = realloc(base, size)) == NULL)
	{
		ERR_Exit(error, NO);
	}
	return mem;
}

//==========================================================================
//
// MS_LittleUWORD
//
// Converts a host U_WORD (2 bytes) to little endian byte order.
//
//==========================================================================

U_WORD MS_LittleUWORD(U_WORD val)
{
	if(acs_BigEndianHost == NO)
	{
		return val;
	}
	return ((val&255)<<8)+((val>>8)&255);
}

//==========================================================================
//
// MS_LittleULONG
//
// Converts a host U_LONG (4 bytes) to little endian byte order.
//
//==========================================================================

U_LONG MS_LittleULONG(U_LONG val)
{
	if(acs_BigEndianHost == NO)
	{
		return val;
	}
	return ((val&255)<<24)+(((val>>8)&255)<<16)+(((val>>16)&255)<<8)
		+((val>>24)&255);
}

//==========================================================================
//
// MS_LoadFile
//
//==========================================================================

int MS_LoadFile(char *name, char **buffer)
{
	int handle;
	int size;
	int count;
	char *addr;
	struct stat fileInfo;

	if(strlen(name) >= MAX_FILE_NAME_LENGTH)
	{
		ERR_Exit(ERR_FILE_NAME_TOO_LONG, NO, name);
	}
	if((handle = open(name, O_RDONLY|O_BINARY, 0666)) == -1)
	{
		ERR_Exit(ERR_CANT_OPEN_FILE, NO, name);
	}
	if(fstat(handle, &fileInfo) == -1)
	{
		ERR_Exit(ERR_CANT_READ_FILE, NO, name);
	}
	size = fileInfo.st_size;
	if((addr = malloc(size)) == NULL)
	{
		ERR_Exit(ERR_NONE, NO, "Couldn't malloc %d bytes for "
			"file \"%s\".", size, name);
	}
	count = read(handle, addr, size);
	close(handle);
	if(count < size)
	{
		ERR_Exit(ERR_CANT_READ_FILE, NO, name);
	}
	*buffer = addr;
	return size;
}

//==========================================================================
//
// MS_SaveFile
//
//==========================================================================

boolean MS_SaveFile(char *name, void *buffer, int length)
{
	int handle;
	int count;

	handle = open(name, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0666);
	if(handle == -1)
	{
		return FALSE;
	}
	count = write(handle, buffer, length);
	close(handle);
	if(count < length)
	{
		return FALSE;
	}
	return TRUE;
}

//==========================================================================
//
// MS_StrCmp
//
//==========================================================================

int MS_StrCmp(char *s1, char *s2)
{
	for(; tolower(*s1) == tolower(*s2); s1++, s2++)
	{
		if(*s1 == '\0')
		{
			return 0;
		}
	}
	return tolower(*s1)-tolower(*s2);
}

//==========================================================================
//
// MS_StrLwr
//
//==========================================================================

char *MS_StrLwr(char *string)
{
	char *c;

	c = string;
	while(*c)
	{
		*c = tolower(*c);
		c++;
	}
	return string;
}

//==========================================================================
//
// MS_StrUpr
//
//==========================================================================

char *MS_StrUpr(char *string)
{
	char *c;

	c = string;
	while(*c)
	{
		*c = toupper(*c);
		c++;
	}
	return string;
}

//==========================================================================
//
// MS_SuggestFileExt
//
//==========================================================================

void MS_SuggestFileExt(char *base, char *extension)
{
	char *search;

	search = base+strlen(base)-1;
	while(*search != ASCII_SLASH && *search != ASCII_BACKSLASH
		&& search != base)
	{
		if(*search-- == '.')
		{
			return;
		}
	}
	strcat(base, extension);
}

//==========================================================================
//
// MS_StripFileExt
//
//==========================================================================

void MS_StripFileExt(char *name)
{
	char *search;

	search = name+strlen(name)-1;
	while(*search != ASCII_SLASH && *search != ASCII_BACKSLASH
		&& search != name)
	{
		if(*search == '.')
		{
			*search = '\0';
			return;
		}
		search--;
	}
}

//==========================================================================
//
// MS_StripFilename
//
//==========================================================================

boolean MS_StripFilename(char *name)
{
	char *c;

	c = name+strlen(name);
	do
	{
		if(--c == name)
		{ // No directory delimiter
			return NO;
		}
	} while(*c != DIRECTORY_DELIMITER_CHAR);
	*c = 0;
	return YES;
}

//==========================================================================
//
// MS_Message
//
//==========================================================================

void MS_Message(msg_t type, char *text, ...)
{
	FILE *fp;
	va_list argPtr;

	if(type == MSG_VERBOSE && acs_VerboseMode == NO)
	{
		return;
	}
	fp = stdout;
	if(type == MSG_DEBUG)
	{
		if(acs_DebugMode == NO)
		{
			return;
		}
		if(acs_DebugFile != NULL)
		{
			fp = acs_DebugFile;
		}
	}
	if(text)
	{
		va_start(argPtr, text);
		vfprintf(fp, text, argPtr);
		va_end(argPtr);
	}
}