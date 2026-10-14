//------------Includes----------------------
#include <image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <app/Message.h>
#include <interface/Font.h>
#include <interface/GroupLayout.h>
#include <interface/ScrollView.h>
#include <interface/Size.h>
#include <interface/TextView.h>
#include <interface/View.h>
#include <interface/Window.h>
#include <support/String.h>
#include <support/SupportDefs.h>
//-------------------------------------------

//	An 'SCTD' resource holds a flattened BPropertyInfo: the list of properties,
//	and the commands, specifiers and data types each one accepts, that an
//	application publishes for scripting (usually in a "suites" attribute).
//	Following BPropertyInfo::Unflatten, the layout is:
//		a byte that is 1 when the data was written on a big-endian machine,
//		int32 property count, int32 flags (1: properties, 2: values),
//		for each property: name, usage, int32 extra data, the commands
//			(int32s ending in 0), the specifiers (int32s ending in 0),
//		for each property: the types (int32s ending in 0) and the compound
//			types (each a list of name and type pairs ending in 0, the list
//			of them ending in 0),
//		for the values: a uint16 count, then for each value an int32 kind,
//			an int32 value, name, usage and an int32 of extra data.
//
//	This is a viewer only: it lists what is there, and hands back the
//	resource's own bytes when saving, so nothing changes. The data is read
//	here with every length checked, rather than by BPropertyInfo, because
//	that does not check them and the resource may be damaged.

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
//	Tells Resourcer that this editor wants a resizable window
extern "C" _EXPORT const bool resizable = true;
extern "C" _EXPORT const char description[] = "Scripting property info";

//	Reads the flattened data, and notes when it runs out.
class Reader {
	public:
		Reader(const unsigned char *data,size_t size) {
			bytes = data;
			length = size;
			position = 0;
			swap = false;
			bad = false;
		}
		bool Bad() const {return bad;}
		size_t Position() const {return position;}
		size_t Length() const {return length;}
		void SetSwap(bool value) {swap = value;}
		//	one byte
		bool Byte(uint8 *value) {
			if (position + 1 > length) {
				bad = true;
				return false;
			}
			*value = bytes[position++];
			return true;
		}
		//	a 32-bit value, swapped when the data came from the other kind of machine
		bool Int(uint32 *value) {
			if (position + 4 > length) {
				bad = true;
				return false;
			}
			uint32 raw = 0;
			memcpy(&raw,bytes + position,4);
			position += 4;
			*value = swap ? Swap32(raw) : raw;
			return true;
		}
		bool Short(uint16 *value) {
			if (position + 2 > length) {
				bad = true;
				return false;
			}
			uint16 raw = 0;
			memcpy(&raw,bytes + position,2);
			position += 2;
			*value = swap ? (uint16)((raw >> 8) | (raw << 8)) : raw;
			return true;
		}
		//	true when the next four bytes are all zero (they are then used up),
		//	false when they are not (they are left to be read as a name)
		bool EndMark() {
			if (position + 4 > length) {
				bad = true;
				return true;
			}
			if (bytes[position] == 0 && bytes[position + 1] == 0
				&& bytes[position + 2] == 0 && bytes[position + 3] == 0) {
				position += 4;
				return true;
			}
			return false;
		}
		//	a string ending in a zero byte
		bool String(BString *value) {
			size_t end = position;
			while (end < length && bytes[end] != 0)
				end++;
			if (end >= length) {
				bad = true;
				return false;
			}
			value->SetTo((const char *)(bytes + position));
			position = end + 1;
			return true;
		}
	private:
		static uint32 Swap32(uint32 raw) {
			return (raw >> 24) | ((raw >> 8) & 0xFF00) | ((raw << 8) & 0xFF0000) | (raw << 24);
		}
		const unsigned char *bytes;
		size_t length;
		size_t position;
		bool swap;
		bool bad;
};

//	A four character code, as 'abcd' when it can be read that way.
static void add_code(BString *text,uint32 code) {
	char shown[16];
	bool printable = true;
	for (int32 i = 0; i < 4; i++) {
		unsigned char c = (code >> (24 - i * 8)) & 0xFF;
		if (c < 32 || c > 126)
			printable = false;
	}
	if (printable)
		sprintf(shown,"'%c%c%c%c'",(char)(code >> 24),(char)(code >> 16),(char)(code >> 8),(char)(code));
	else
		sprintf(shown,"0x%08lx",(unsigned long)(code));
	text->Append(shown);
}

//	The scripting commands and specifiers that have names.
static const char *command_name(uint32 command) {
	switch (command) {
		case 'PGET': return "Get";
		case 'PSET': return "Set";
		case 'PCRT': return "Create";
		case 'PDEL': return "Delete";
		case 'PCNT': return "Count";
		case 'PEXE': return "Execute";
		case 'SUIT': return "Get suites";
		default: break;
	}
	return NULL;
}

static const char *specifier_name(uint32 specifier) {
	switch (specifier) {
		case 1: return "Direct";
		case 2: return "Index";
		case 3: return "Reverse index";
		case 4: return "Range";
		case 5: return "Reverse range";
		case 6: return "Name";
		case 7: return "ID";
		default: break;
	}
	return NULL;
}

//	Adds "Name ('code')" or just the code when there is no name.
static void add_command(BString *text,uint32 command) {
	const char *name = command_name(command);
	if (name != NULL)
		text->Append(name).Append(" (");
	add_code(text,command);
	if (name != NULL)
		text->Append(")");
}

static void add_specifier(BString *text,uint32 specifier) {
	const char *name = specifier_name(specifier);
	char number[32];
	if (name != NULL) {
		text->Append(name);
	} else {
		sprintf(number,"%lu",(unsigned long)(specifier));
		text->Append(number);
		if (specifier > 128)
			text->Append(" (defined by the application)");
	}
}

//	Turns the resource into the text that is shown. Says so at the end when
//	the data stops early, or when it is not property info at all.
static void describe(const unsigned char *data,size_t length,BString *text) {
	Reader in(data,length);
	uint8 order = 0;
	uint32 count = 0;
	uint32 flags = 0;
	if (length == 0) {
		text->SetTo("No property info");
		return;
	}
	in.Byte(&order);
	in.SetSwap(order != 0);	// Haiku and BeOS on Intel write little-endian, so 1 means swapping
	in.Int(&count);
	in.Int(&flags);
	if (in.Bad() || (order != 0 && order != 1) || (flags & ~3UL) != 0 || count > length) {
		char line[100];
		sprintf(line,"Not property info (%lu bytes)",(unsigned long)(length));
		text->SetTo(line);
		return;
	}

	char line[200];
	BString *names = new BString[count + 1];
	BString *usages = new BString[count + 1];
	uint32 *extras = new uint32[count + 1];
	BString *commands = new BString[count + 1];
	BString *specifiers = new BString[count + 1];
	BString *types = new BString[count + 1];
	BString *compounds = new BString[count + 1];
	uint32 read = 0;

	if (flags & 1) {
		//	the main chunks
		for (; read < count && !in.Bad(); ) {
			uint32 value = 0;
			extras[read] = 0;
			in.String(&names[read]);
			in.String(&usages[read]);
			in.Int(&extras[read]);
			while (in.Int(&value) && value != 0) {
				if (commands[read].Length() > 0)
					commands[read].Append(", ");
				add_command(&commands[read],value);
			}
			while (in.Int(&value) && value != 0) {
				if (specifiers[read].Length() > 0)
					specifiers[read].Append(", ");
				add_specifier(&specifiers[read],value);
			}
			//	a property that was cut short is not listed
			if (in.Bad())
				break;
			read++;
		}
		//	the type chunks
		for (uint32 i = 0; i < read && !in.Bad(); i++) {
			uint32 value = 0;
			while (in.Int(&value) && value != 0) {
				if (types[i].Length() > 0)
					types[i].Append(", ");
				add_code(&types[i],value);
			}
			while (!in.Bad() && !in.EndMark()) {
				//	one compound type: pairs of a name and a type
				BString pairs;
				while (!in.Bad() && !in.EndMark()) {
					BString name;
					uint32 type = 0;
					in.String(&name);
					in.Int(&type);
					if (in.Bad())
						break;
					if (pairs.Length() > 0)
						pairs.Append(", ");
					pairs.Append(name).Append(" ");
					add_code(&pairs,type);
				}
				if (compounds[i].Length() > 0)
					compounds[i].Append("; ");
				compounds[i].Append("{").Append(pairs).Append("}");
			}
		}
	}

	text->SetTo("");
	sprintf(line,"%lu properties",(unsigned long)(count));
	text->Append(line);
	if (read < count || in.Bad())
		text->Append(" (the data ends early)");
	text->Append("\n");
	for (uint32 i = 0; i < read; i++) {
		text->Append("\nProperty: ").Append(names[i]).Append("\n");
		if (usages[i].Length() > 0)
			text->Append("  Usage: ").Append(usages[i]).Append("\n");
		if (extras[i] != 0) {
			sprintf(line,"  Extra data: %lu\n",(unsigned long)(extras[i]));
			text->Append(line);
		}
		if (commands[i].Length() > 0)
			text->Append("  Commands: ").Append(commands[i]).Append("\n");
		else
			text->Append("  Commands: all\n");
		if (specifiers[i].Length() > 0)
			text->Append("  Specifiers: ").Append(specifiers[i]).Append("\n");
		else
			text->Append("  Specifiers: all\n");
		if (types[i].Length() > 0)
			text->Append("  Types: ").Append(types[i]).Append("\n");
		if (compounds[i].Length() > 0)
			text->Append("  Compound types: ").Append(compounds[i]).Append("\n");
	}

	//	the values, kept apart from the properties
	if ((flags & 2) && !in.Bad() && read == count) {
		uint16 values = 0;
		in.Short(&values);
		sprintf(line,"\n%u values",(unsigned)(values));
		text->Append(line);
		uint32 shown = 0;
		BString body;
		for (; shown < values && !in.Bad(); shown++) {
			uint32 kind = 0;
			uint32 value = 0;
			uint32 extra = 0;
			BString name;
			BString usage;
			in.Int(&kind);
			in.Int(&value);
			in.String(&name);
			in.String(&usage);
			in.Int(&extra);
			if (in.Bad())
				break;
			body.Append("\nValue: ").Append(name).Append("\n");
			body.Append(kind == 0 ? "  Command: " : (kind == 1 ? "  Type code: " : "  Kind unknown, value: "));
			add_code(&body,value);
			body.Append("\n");
			if (usage.Length() > 0)
				body.Append("  Usage: ").Append(usage).Append("\n");
			if (extra != 0) {
				sprintf(line,"  Extra data: %lu\n",(unsigned long)(extra));
				body.Append(line);
			}
		}
		if (shown < values || in.Bad())
			text->Append(" (the data ends early)");
		text->Append("\n").Append(body);
	}

	delete [] names;
	delete [] usages;
	delete [] extras;
	delete [] commands;
	delete [] specifiers;
	delete [] types;
	delete [] compounds;
}

//	A text view that is only for reading, and keeps the resource's bytes.
class SuiteText : public BTextView {
	public:
		SuiteText() : BTextView("suites",B_WILL_DRAW | B_FRAME_EVENTS) {
			data = NULL;
			size = 0;
			MakeEditable(false);
			SetStylable(false);
		}
		~SuiteText() {
			delete [] data;
		}
		void SetData(const unsigned char *newdata,size_t newsize) {
			delete [] data;
			data = NULL;
			size = 0;
			if (newdata != NULL && newsize > 0) {
				data = new unsigned char[newsize];
				memcpy(data,newdata,newsize);
				size = newsize;
			}
			BString text;
			describe(data,size,&text);
			SetText(text.String());
		}
		const unsigned char *Data() const {return data;}
		size_t Size() const {return size;}
	private:
		unsigned char *data;
		size_t size;
};

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	SuiteText *view = new SuiteText();
	view->SetData(data,length);

	//	The text view and its scroll bar are layout managed, so they follow the
	//	window when it is resized.
	BScrollView *scroll = new BScrollView("suitesScroll",view,B_WILL_DRAW | B_FRAME_EVENTS,false,true,B_FANCY_BORDER);
	BGroupLayout *group = new BGroupLayout(B_VERTICAL);
	bkgview->SetLayout(group);
	group->SetInsets(10,10,10,10);
	group->AddView(scroll);

	//	A comfortable starting size, based on the current font
	font_height height;
	be_plain_font->GetHeight(&height);
	float line = height.ascent + height.descent + height.leading;
	scroll->SetExplicitPreferredSize(BSize(be_plain_font->StringWidth("M") * 56,line * 20));
	//	Ask for the minimum size first: the preferred size can come back as NaN
	//	until the layout is validated, so the tests replace a NaN.
	BSize min = bkgview->MinSize();
	BSize size = bkgview->PreferredSize();
	if (!(size.width >= min.width))
		size.width = min.width;
	if (!(size.height >= min.height))
		size.height = min.height;
	bkgview->Window()->SetSizeLimits(min.width,B_SIZE_UNLIMITED,min.height,B_SIZE_UNLIMITED);
	bkgview->Window()->ResizeTo(size.width,size.height);
}

//	Nothing can be changed here, so the resource is saved as it was.
unsigned char* savedata(size_t *length,BView *bkgview) {
	SuiteText *view = (SuiteText *)(bkgview->FindView("suites"));
	if (view == NULL || view->Size() == 0) {
		*length = 0;
		return new unsigned char[1];
	}
	unsigned char *data = new unsigned char[view->Size()];
	memcpy(data,view->Data(),view->Size());
	*length = view->Size();
	return data;
}

void messaging(BMessage *message,BView *bkgview) {
}
