#ifndef RESOURCER_EDITORS_WIND_BITS_H
#define RESOURCER_EDITORS_WIND_BITS_H

// extracts a BBitmap from a Resource

//IMPORTANT!
//Requires libtranslation.so

#include <translation/TranslationUtils.h>
#include <translation/TranslatorFormats.h>
#include <interface/Screen.h>
#include <interface/ColorControl.h>
#include <interface/Bitmap.h>
#include <interface/View.h>
#include "ResourceIO.h"

BBitmap *get_bitmap(long id);
BBitmap *get_bitmap(const char *name);

BBitmap *get_bitmap(long id) {
	return BTranslationUtils::GetBitmap(B_TRANSLATOR_BITMAP,id);
}

BBitmap *get_bitmap(const char *name) {
	return BTranslationUtils::GetBitmap(B_TRANSLATOR_BITMAP,name);
}

#endif /* RESOURCER_EDITORS_WIND_BITS_H */
