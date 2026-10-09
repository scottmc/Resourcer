// Free helper functions shared by the application.
// Prototypes are in preamble.h.

#include "includes.h"
#include "preamble.h"

BRect find_center(uint32 width, uint32 height) {
	BRect a = BScreen(B_MAIN_SCREEN_ID).Frame();
	uint32 screen_width = (uint32)(a.right - a.left);
	uint32 screen_height = (uint32)(a.bottom - a.top);
	uint32 c = (uint32)(((screen_width / 2) - (width / 2)) + a.left);
	uint32 d = (uint32)(((screen_height / 2) - (height / 2)) + a.top);
	BRect toreturn;
	toreturn.left = c;
	toreturn.top = d;
	toreturn.right = c + width;
	toreturn.bottom = d + height;
	return toreturn;
}

BRect find_center(BRect rect,uint32 width, uint32 height) {
	BRect a = rect;
	uint32 screen_width = (uint32)(a.right - a.left);
	uint32 screen_height = (uint32)(a.bottom - a.top);
	uint32 c = (uint32)(((screen_width / 2) - (width / 2)) + a.left);
	uint32 d = (uint32)(((screen_height / 2) - (height / 2)) + a.top);
	BRect toreturn;
	toreturn.left = c;
	toreturn.top = d;
	toreturn.right = c + width;
	toreturn.bottom = d + height;
	return toreturn;
}

#if __INTEL__ || defined(__x86_64__)
type_code flipcode(type_code original) {
	type_code type = original;
	char *one = (char *)(&original);
	char *two = one + 1;
	char *three = two + 1;
	char *four = three + 1;
	*((char *)(&type)) = *four;
	*((char *)(&type) + 1) = *three;
	*((char *)(&type) + 2) = *two;
	*((char *)(&type) + 3) = *one;
	return type;
}
#endif

bool alphabet(const char *string1, const char *string2) {
	size_t length1 = strlen(string1);
	size_t length2 = strlen(string2);
	size_t length;
	bool xgreater = false;
	if (length1 > length2)
		length = length2;
	else
		length = length1;
	size_t i;
	BString s1(string1);
	BString s2(string2);
	s1.ToLower();
	s2.ToLower();
	for (i = 0;i < length;i++) {
		if (s1[i] != s2[i]) {
			if (s1[i] > s2[i]) {
				xgreater = true;
				break;
			} else {
				xgreater = false;
				break;
			}
		}
	}
	return xgreater;
}

int sortmenu(const void *first,const void *second) {
	if (alphabet((*((BMenuItem **)(first)))->Label(),(*((BMenuItem **)(second)))->Label()))
		return -1;
	else
		return 1;
	return 0;
}
