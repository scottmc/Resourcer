//------------Includes----------------------
#include <image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <app/Message.h>
#include <interface/GridLayout.h>
#include <interface/GroupLayout.h>
#include <interface/LayoutItem.h>
#include <interface/Size.h>
#include <interface/SpaceLayoutItem.h>
#include <interface/StringView.h>
#include <interface/TextControl.h>
#include <interface/View.h>
#include <interface/Window.h>
//-------------------------------------------

//	An 'NWAD' resource holds a BNetworkAddress, which is flattened as the
//	plain bytes of the socket address (a BSD style sockaddr):
//		uint8 length, uint8 family, then the rest depends on the family.
//	For IPv4 (family 1) that is a 16 bit port and a 32 bit address, in 32
//	bytes, and for IPv6 (family 5) a port, 32 bits of flow information, the
//	128 bit address and a 32 bit scope id, in 28 bytes. The port and addresses
//	are in network (big-endian) order. A link level address (family 4) is an
//	interface index and a MAC address.
//
//	IPv4 and IPv6 addresses can be edited: type the address, and which of the
//	two it is follows from what was typed. Any other kind of address, or one
//	that is damaged, is shown but not edited, and is saved as it was. The text
//	is read and written here, rather than by BNetworkAddress, so no network
//	library is needed.

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Network address";

//	The message the two text fields send when they are changed
static const uint32 kEdited = (uint32)-101;

//	The first line shows the family, and keeps the bytes of the resource so
//	each window has its own copy to save back unchanged.
class FamilyLine : public BStringView {
	public:
		FamilyLine(const char *text,const unsigned char *data,size_t length)
			: BStringView("family",text) {
			kept = NULL;
			keptsize = 0;
			if (data != NULL && length > 0) {
				kept = new unsigned char[length];
				memcpy(kept,data,length);
				keptsize = length;
			}
		}
		~FamilyLine() {
			delete [] kept;
		}
		unsigned char *kept;
		size_t keptsize;
};

static const char *family_name(unsigned family) {
	switch (family) {
		case 0: return "Unspecified (AF_UNSPEC)";
		case 1: return "IPv4 (AF_INET)";
		case 2: return "AppleTalk (AF_APPLETALK)";
		case 3: return "Route (AF_ROUTE)";
		case 4: return "Link level (AF_LINK)";
		case 5: return "IPv6 (AF_INET6)";
		case 6: return "Data link (AF_DLI)";
		case 7: return "IPX (AF_IPX)";
		case 8: return "Notify (AF_NOTIFY)";
		case 9: return "Local (AF_LOCAL)";
		case 10: return "Bluetooth (AF_BLUETOOTH)";
		default: break;
	}
	return NULL;
}

//	IPv6 written the usual way, with the longest run of zero groups as "::"
static void ipv6_text(const unsigned char *bytes,char *out) {
	unsigned groups[8];
	for (int i = 0; i < 8; i++)
		groups[i] = (bytes[i * 2] << 8) | bytes[i * 2 + 1];
	int bestStart = -1;
	int bestLength = 0;
	for (int i = 0; i < 8;) {
		if (groups[i] != 0) {
			i++;
			continue;
		}
		int start = i;
		while (i < 8 && groups[i] == 0)
			i++;
		if (i - start > bestLength) {
			bestStart = start;
			bestLength = i - start;
		}
	}
	if (bestLength < 2)
		bestStart = -1;
	out[0] = '\0';
	for (int i = 0; i < 8; i++) {
		if (i == bestStart) {
			strcat(out,"::");
			i += bestLength - 1;
			continue;
		}
		if (i != 0 && !(bestStart >= 0 && i == bestStart + bestLength))
			strcat(out,":");
		sprintf(out + strlen(out),"%x",groups[i]);
	}
}

static void hex_text(const unsigned char *bytes,size_t count,char *out,size_t room) {
	out[0] = '\0';
	for (size_t i = 0; i < count && strlen(out) + 4 < room; i++)
		sprintf(out + strlen(out),i == 0 ? "%02x" : ":%02x",bytes[i]);
}

//	Four decimal numbers separated by dots.
static bool parse_ipv4(const char *text,unsigned char *out) {
	int part = 0;
	int digits = 0;
	int value = 0;
	for (;; text++) {
		char c = *text;
		if (c >= '0' && c <= '9') {
			value = value * 10 + (c - '0');
			digits++;
			if (digits > 3 || value > 255)
				return false;
		} else if (c == '.' || c == '\0') {
			if (digits == 0 || part > 3)
				return false;
			out[part++] = (unsigned char)(value);
			digits = 0;
			value = 0;
			if (c == '\0')
				break;
		} else {
			return false;
		}
	}
	return part == 4;
}

//	Groups of up to four hex digits separated by colons, with one "::" for a
//	run of zero groups, and an IPv4 address allowed in place of the last two.
static bool parse_ipv6(const char *text,unsigned char *out) {
	unsigned groups[8];
	int count = 0;
	int gap = -1;
	const char *s = text;
	if (s[0] == ':') {
		if (s[1] != ':')
			return false;
		gap = 0;
		s += 2;
	}
	while (*s != '\0') {
		const char *end = s;
		while (*end != '\0' && *end != ':')
			end++;
		size_t length = end - s;
		if (length == 0)
			return false;
		if (memchr(s,'.',length) != NULL) {
			//	an IPv4 address ends the text
			unsigned char four[4];
			char copy[32];
			if (*end != '\0' || length >= sizeof(copy) || count + 2 > 8)
				return false;
			memcpy(copy,s,length);
			copy[length] = '\0';
			if (!parse_ipv4(copy,four))
				return false;
			groups[count++] = (four[0] << 8) | four[1];
			groups[count++] = (four[2] << 8) | four[3];
			break;
		}
		if (length > 4 || count >= 8)
			return false;
		unsigned value = 0;
		for (const char *p = s; p < end; p++) {
			unsigned digit;
			if (*p >= '0' && *p <= '9')
				digit = *p - '0';
			else if (*p >= 'a' && *p <= 'f')
				digit = *p - 'a' + 10;
			else if (*p >= 'A' && *p <= 'F')
				digit = *p - 'A' + 10;
			else
				return false;
			value = value * 16 + digit;
		}
		groups[count++] = value;
		s = end;
		if (*s == ':') {
			s++;
			if (*s == ':') {
				if (gap >= 0)
					return false;
				gap = count;
				s++;
			} else if (*s == '\0') {
				return false;
			}
		}
	}
	if (gap < 0 ? count != 8 : count > 7)
		return false;
	unsigned all[8];
	int missing = 8 - count;
	for (int i = 0, from = 0; i < 8; i++) {
		if (gap >= 0 && i >= gap && i < gap + missing)
			all[i] = 0;
		else
			all[i] = groups[from++];
	}
	for (int i = 0; i < 8; i++) {
		out[i * 2] = (unsigned char)(all[i] >> 8);
		out[i * 2 + 1] = (unsigned char)(all[i] & 0xFF);
	}
	return true;
}

//	A port number, or nothing for 0.
static bool parse_port(const char *text,unsigned *port) {
	unsigned value = 0;
	int digits = 0;
	for (; *text != '\0'; text++, digits++) {
		if (*text < '0' || *text > '9' || digits >= 5)
			return false;
		value = value * 10 + (*text - '0');
	}
	if (value > 65535)
		return false;
	*port = value;
	return true;
}

//	Copies the text without the spaces around it.
static void trimmed(const char *text,char *out,size_t room) {
	while (*text == ' ' || *text == '\t')
		text++;
	strncpy(out,text,room - 1);
	out[room - 1] = '\0';
	size_t length = strlen(out);
	while (length > 0 && (out[length - 1] == ' ' || out[length - 1] == '\t'))
		out[--length] = '\0';
}

//	Makes the bytes of the address from what was typed. The flow information
//	and scope of an IPv6 address are not typed, so they are kept from the old
//	bytes when those were IPv6 too. Returns 4 or 6 for the kind of address,
//	and 0 when the text is not an address and a port.
static int build(const char *addressText,const char *portText,const unsigned char *old,size_t oldLength,unsigned char *out,size_t *outLength) {
	char address[200];
	char portcopy[40];
	unsigned port = 0;
	unsigned char bytes[16];
	trimmed(addressText,address,sizeof(address));
	trimmed(portText,portcopy,sizeof(portcopy));
	if (!parse_port(portcopy,&port))
		return 0;
	memset(out,0,32);
	if (strchr(address,':') != NULL) {
		if (!parse_ipv6(address,bytes))
			return 0;
		out[0] = 28;
		out[1] = 5;
		out[2] = port >> 8;
		out[3] = port & 0xFF;
		if (old != NULL && oldLength >= 28 && old[1] == 5) {
			memcpy(out + 4,old + 4,4);
			memcpy(out + 24,old + 24,4);
		}
		memcpy(out + 8,bytes,16);
		*outLength = 28;
		return 6;
	}
	if (!parse_ipv4(address,bytes))
		return 0;
	out[0] = 32;
	out[1] = 1;
	out[2] = port >> 8;
	out[3] = port & 0xFF;
	memcpy(out + 4,bytes,4);
	*outLength = 32;
	return 4;
}

//	Tells what the fields hold, and what will be saved.
static void update_status(BView *bkgview) {
	FamilyLine *line = (FamilyLine *)(bkgview->FindView("family"));
	BTextControl *address = (BTextControl *)(bkgview->FindView("address"));
	BTextControl *port = (BTextControl *)(bkgview->FindView("port"));
	if (line == NULL || address == NULL || port == NULL)
		return;
	unsigned char bytes[32];
	size_t length = 0;
	int kind = build(address->Text(),port->Text(),line->kept,line->keptsize,bytes,&length);
	if (kind == 4)
		line->SetText("IPv4 (AF_INET)");
	else if (kind == 6)
		line->SetText("IPv6 (AF_INET6)");
	else if (address->Text()[0] == '\0' && line->keptsize == 0)
		line->SetText("Type an IPv4 or IPv6 address");
	else if (line->keptsize == 0)
		line->SetText("Not an address and port (nothing will be saved)");
	else
		line->SetText("Not an address and port (the old value will be kept)");
}

//	Makes the window for an address that can be edited.
static void edit_window(unsigned char *data,size_t length,BView *bkgview) {
	char addressText[120];
	char portText[20];
	addressText[0] = '\0';
	portText[0] = '\0';
	if (length >= 8 && data[1] == 1) {
		sprintf(addressText,"%u.%u.%u.%u",data[4],data[5],data[6],data[7]);
		sprintf(portText,"%u",(data[2] << 8) | data[3]);
	} else if (length >= 24 && data[1] == 5) {
		ipv6_text(data + 8,addressText);
		sprintf(portText,"%u",(data[2] << 8) | data[3]);
	}
	BTextControl *address = new BTextControl("address","Address:",addressText,new BMessage(kEdited));
	BTextControl *port = new BTextControl("port","Port:",portText,new BMessage(kEdited));
	address->SetModificationMessage(new BMessage(kEdited));
	port->SetModificationMessage(new BMessage(kEdited));
	address->SetTarget(bkgview->Window());
	port->SetTarget(bkgview->Window());

	BGridLayout *grid = new BGridLayout(8,8);
	bkgview->SetLayout(grid);
	grid->SetInsets(10,10,10,10);
	grid->AddItem(address->CreateLabelLayoutItem(),0,0);
	//	room for the longest address, written out in full
	float wide = be_plain_font->StringWidth("0000:0000:0000:0000:0000:0000:0000:0000") + 24;
	BLayoutItem *field = address->CreateTextViewLayoutItem();
	field->SetExplicitMinSize(BSize(wide,B_SIZE_UNSET));
	field->SetExplicitPreferredSize(BSize(wide,B_SIZE_UNSET));
	grid->AddItem(field,1,0);
	grid->AddItem(port->CreateLabelLayoutItem(),0,1);
	BLayoutItem *portfield = port->CreateTextViewLayoutItem();
	portfield->SetExplicitMaxSize(BSize(be_plain_font->StringWidth("00000") + 24,B_SIZE_UNSET));
	grid->AddItem(portfield,1,1);
	FamilyLine *line = new FamilyLine("",data,length);
	grid->AddView(line,0,2,2,1);
	grid->AddItem(BSpaceLayoutItem::CreateGlue(),0,3,2,1);
	update_status(bkgview);
}

//	Makes the window for an address that is only shown.
static void view_window(unsigned char *data,size_t length,BView *bkgview) {
	char family[100];
	char address[200];
	char note[200];
	address[0] = '\0';
	note[0] = '\0';
	if (length < 2) {
		strcpy(family,"Not a network address");
		sprintf(note,"%lu byte",(unsigned long)(length));
	} else {
		unsigned code = data[1];
		const char *name = family_name(code);
		if (name != NULL)
			strcpy(family,name);
		else
			sprintf(family,"Unknown family %u",code);
		sprintf(note,"%lu bytes, length byte %u",(unsigned long)(length),(unsigned)(data[0]));
		if (code == 1) {
			strcpy(address,"(too short for an IPv4 address)");
		} else if (code == 5) {
			strcpy(address,"(too short for an IPv6 address)");
		} else if (code == 4) {
			//	frame type and interface index, then the name and link level
			//	address, whose lengths come first
			strcpy(address,"(too short for a link level address)");
			if (length >= 12) {
				unsigned nameLength = data[9];
				unsigned addressLength = data[10];
				size_t start = 12 + nameLength;
				if (start + addressLength <= length) {
					hex_text(data + start,addressLength,address,sizeof(address));
					unsigned long index = data[4] | (data[5] << 8) | (data[6] << 16) | ((unsigned long)(data[7]) << 24);
					sprintf(note + strlen(note),", interface %lu",index);
				}
			}
		} else if (code != 0) {
			size_t shown = length - 2 < 16 ? length - 2 : 16;
			hex_text(data + 2,shown,address,sizeof(address));
			if (shown < length - 2)
				strcat(address," ...");
		}
	}
	BGroupLayout *top = new BGroupLayout(B_VERTICAL,8);
	bkgview->SetLayout(top);
	top->SetInsets(10,10,10,10);
	top->AddView(new FamilyLine(family,data,length));
	if (address[0] != '\0')
		top->AddView(new BStringView("shown",address));
	top->AddView(new BStringView("note",note));
	top->AddView(new BStringView("hint","This kind of address can be viewed but not edited"));
	for (int32 i = 0; i < top->CountItems(); i++)
		top->ItemAt(i)->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,B_ALIGN_TOP));
}

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	//	new, IPv4 and IPv6 addresses are edited
	bool editable = (length == 0 || data == NULL)
		|| (length >= 8 && data[1] == 1) || (length >= 24 && data[1] == 5);
	if (editable)
		edit_window(data,length,bkgview);
	else
		view_window(data,length,bkgview);

	//	Ask for the minimum size first: the preferred size can come back as NaN
	//	until the layout is validated, so the tests replace a NaN.
	BSize min = bkgview->MinSize();
	BSize size = bkgview->PreferredSize();
	if (!(size.width >= min.width))
		size.width = min.width;
	if (!(size.height >= min.height))
		size.height = min.height;
	bkgview->Window()->ResizeTo(size.width,size.height);
}

//	What is typed is saved when it is an address, otherwise the resource is
//	saved as it was.
unsigned char* savedata(size_t *length,BView *bkgview) {
	FamilyLine *line = (FamilyLine *)(bkgview->FindView("family"));
	BTextControl *address = (BTextControl *)(bkgview->FindView("address"));
	BTextControl *port = (BTextControl *)(bkgview->FindView("port"));
	if (line == NULL) {
		*length = 0;
		return new unsigned char[1];
	}
	if (address != NULL && port != NULL) {
		unsigned char bytes[32];
		size_t size = 0;
		if (build(address->Text(),port->Text(),line->kept,line->keptsize,bytes,&size) != 0) {
			unsigned char *data = new unsigned char[size];
			memcpy(data,bytes,size);
			*length = size;
			return data;
		}
	}
	if (line->keptsize == 0) {
		*length = 0;
		return new unsigned char[1];
	}
	unsigned char *data = new unsigned char[line->keptsize];
	memcpy(data,line->kept,line->keptsize);
	*length = line->keptsize;
	return data;
}

void messaging(BMessage *message,BView *bkgview) {
	if (message->what == kEdited)
		update_status(bkgview);
}
