// $Id$

/**
 * @file Platform dependent stuff.
 *
 * Copyright (C) 2007 Patrik Rak (patrik@raxoft.cz)
 *
 * This source code is released under the MIT license, see included license.txt.
 */

#ifndef SYSDEFS_H
#define SYSDEFS_H 1

#ifdef _MSC_VER
#include <io.h>
#include <fcntl.h>
#define set_binary_mode(x)  _setmode( _fileno( stdin ), _O_BINARY ) ;
#else
#define set_binary_mode(x)
#endif

#endif // SYSDEFS_H

