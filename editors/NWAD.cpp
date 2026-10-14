//------------Includes----------------------
#include <image.h>
#include <stdio.h>
#include <string.h>
#include <app/Message.h>
#include <interface/GroupLayout.h>
#include <interface/Size.h>
#include <interface/StringView.h>
#include <interface/View.h>
#include <interface/Window.h>
//-------------------------------------------

//	An 'NWAD' resource holds a BNetworkAddress, which is flattened as the
//	plain bytes of the socket address (a BSD style sockaddr):
//		uint8 length, uint8 family, then the rest depends on the family.
//	For IPv4 (family 1) that is a 16 bit port and a 32 bit address, and for
//	IPv6 (family 5) a port, 32 bits of flow information, the 128 bit address
//	and a 32 bit scope id. The port and addresses are in network (big-endian)
//	order. A link level address (family 4) is an interface index and a MAC
//	address.
//
//	This is a viewer only: it shows the address and hands back the resource's
//	own bytes when saving, so nothing changes. The text is written here, rather
//	than by BNetworkAddress, so no network library is needed.

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Network address";

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

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	char family[100];
	char address[200];
	char port[100];
	char note[200];
	address[0] = '\0';
	port[0] = '\0';
	note[0] = '\0';
	if (length == 0 || data == NULL) {
		strcpy(family,"No address");
	} else if (length < 2) {
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
			if (length >= 8) {
				sprintf(address,"%u.%u.%u.%u",data[4],data[5],data[6],data[7]);
				sprintf(port,"Port %u",(data[2] << 8) | data[3]);
			} else {
				strcpy(address,"(too short for an IPv4 address)");
			}
		} else if (code == 5) {
			if (length >= 24) {
				ipv6_text(data + 8,address);
				sprintf(port,"Port %u",(data[2] << 8) | data[3]);
				if (length >= 28) {
					unsigned long scope = data[24] | (data[25] << 8) | (data[26] << 16) | ((unsigned long)(data[27]) << 24);
					if (scope != 0)
						sprintf(port + strlen(port),", scope %lu",scope);
				}
			} else {
				strcpy(address,"(too short for an IPv6 address)");
			}
		} else if (code == 4) {
			//	interface index (4 bytes after the frame type), then the name and
			//	link level address, whose lengths come first
			if (length >= 12) {
				unsigned nameLength = data[9];
				unsigned addressLength = data[10];
				size_t start = 12 + nameLength;
				if (start + addressLength <= length)
					hex_text(data + start,addressLength,address,sizeof(address));
				else
					strcpy(address,"(too short for a link level address)");
				unsigned long index = data[4] | (data[5] << 8) | (data[6] << 16) | ((unsigned long)(data[7]) << 24);
				sprintf(port,"Interface %lu",index);
			} else {
				strcpy(address,"(too short for a link level address)");
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
		top->AddView(new BStringView("address",address));
	if (port[0] != '\0')
		top->AddView(new BStringView("port",port));
	if (note[0] != '\0')
		top->AddView(new BStringView("note",note));
	top->AddView(new BStringView("hint","This address can be viewed but not edited"));
	for (int32 i = 0; i < top->CountItems(); i++)
		top->ItemAt(i)->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,B_ALIGN_TOP));

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

//	Nothing can be changed here, so the resource is saved as it was.
unsigned char* savedata(size_t *length,BView *bkgview) {
	FamilyLine *line = (FamilyLine *)(bkgview->FindView("family"));
	if (line == NULL || line->keptsize == 0) {
		*length = 0;
		return new unsigned char[1];
	}
	unsigned char *data = new unsigned char[line->keptsize];
	memcpy(data,line->kept,line->keptsize);
	*length = line->keptsize;
	return data;
}

void messaging(BMessage *message,BView *bkgview) {
}
