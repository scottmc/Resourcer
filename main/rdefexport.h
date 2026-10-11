#ifndef RESOURCER_MAIN_RDEFEXPORT_H
#define RESOURCER_MAIN_RDEFEXPORT_H

//	Writes resources as rdef source, the text format Haiku's rc compiles.
//	The writing code is Haiku's own rsrc-to-rdef decompiler (src/bin/rc/
//	decompile.cpp, Copyright (c) 2003 Matthijs Hollemans, MIT license), cut
//	down to work on a BResources that is already open, so unsaved changes are
//	included, and to write one resource as easily as all of them.

#include <Resources.h>
#include <stdio.h>

//	Writes every resource in res to output. count, if given, gets how many.
status_t rdef_export_all(BResources *res, FILE *output, int32 *count);

//	Writes the one resource with this type and ID, or returns B_ENTRY_NOT_FOUND.
status_t rdef_export_resource(BResources *res, type_code type, int32 id, FILE *output);

#endif /* RESOURCER_MAIN_RDEFEXPORT_H */
