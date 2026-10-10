//Note: The type code is the four letter code displayed by Resourcer
//      with single quotation marks around it.
//      For instance, CSTR would be written as 'CSTR'


#include <storage/Resources.h>
#include <support/DataIO.h>
#include <app/Application.h>
#include <stdio.h>
#include <string.h>

const void *AppResource(type_code type,int32 id,size_t *lengthFound);
const void *AppResource(type_code type,const char *name,size_t *lengthFound);
status_t get_app_resource(type_code type,const char *name,void** buffer,size_t *lengthFound);
status_t get_app_resource(type_code type,long id,void** buffer,size_t *lengthFound);

class ResourceIO : public BPositionIO {
	public:
		ResourceIO(type_code type,int32 id,BResources *res = be_app->AppResources());
		ResourceIO(type_code type,const char *name,BResources *res = be_app->AppResources());
		ssize_t Read(void *buffer,size_t numBytes);
		ssize_t ReadAt(off_t position,void *buffer,size_t numBytes);
		off_t Seek(off_t position,uint32 mode);
		off_t Position(void) const;
		ssize_t Write(const void *buffer,size_t numBytes);
		ssize_t WriteAt(off_t position,const void *buffer,size_t numBytes);
		size_t Size(void);
		status_t SetSize(off_t size);
		int32 ID(void);
		~ResourceIO(void);
	private:
		int32 id;
		type_code type;
		off_t curPos;
		char *name;
		BResources *res;
};


const void *AppResource(type_code type,int32 id,size_t *lengthFound) {
	return be_app->AppResources()->LoadResource(type,id,lengthFound);
}

const void *AppResource(type_code type,const char *name,size_t *lengthFound) {
	return be_app->AppResources()->LoadResource(type,name,lengthFound);
}

ResourceIO::ResourceIO(type_code type,int32 id,BResources *res) : BPositionIO() {
	this->id = id;
	this->type = type;
	this->res = res;
	name = NULL;
	curPos = 0;
}

ResourceIO::ResourceIO(type_code type,const char *name,BResources *res) : BPositionIO() {
	this->type = type;
	this->res = res;
	this->name = new char[strlen(name) + 1];
	strcpy(this->name,name);
	int32 id2;
	size_t useless;
	res->GetResourceInfo(type,name,&id2,&useless);
	id = id2;
	curPos = 0;
}

ResourceIO::~ResourceIO(void) {
	if (name != NULL)
		delete [] name;
}

ssize_t ResourceIO::Read(void *buffer,size_t numBytes) {
	ssize_t temp = ReadAt(curPos,buffer,numBytes);
	curPos += temp;
	return temp;
}

ssize_t ResourceIO::ReadAt(off_t position,void *buffer,size_t numBytes) {
	size_t bytesRead;
	const void *buf = res->LoadResource(type,id,&bytesRead);
	if (position > bytesRead)
		return B_ERROR;
	if ((bytesRead - position) > numBytes)
		bytesRead = numBytes;
	else
		bytesRead -= position;
	memcpy(buffer,(((unsigned char *)(buf)) + position),bytesRead);
	return (ssize_t)(bytesRead);
}

off_t ResourceIO::Seek(off_t position,uint32 mode) {
	char *name;
	size_t length;
	res->GetResourceInfo(type,id,(const char **)&name,&length);
	switch (mode) {
		case SEEK_SET:
			curPos = position;
			break;
		case SEEK_CUR:
			curPos += position;
			break;
		case SEEK_END:
			curPos = length + position;
			break;
	}
	return (off_t)curPos;
}

off_t ResourceIO::Position(void) const{
	return (off_t)curPos;
}

ssize_t ResourceIO::Write(const void *buffer,size_t numBytes) {
	ssize_t temp = WriteAt(curPos,buffer,numBytes);
	curPos += temp;
	return temp;
}

ssize_t ResourceIO::WriteAt(off_t position,const void *buffer,size_t numBytes) {
	if (position < 0)
		return B_BAD_VALUE;
	//BResources::WriteResource() is deprecated, so build the new contents from the
	//old ones and store the whole resource again, keeping its name.
	size_t oldLength = 0;
	const void *old = res->LoadResource(type,id,&oldLength);
	if (old == NULL)
		oldLength = 0;
	size_t newLength = (size_t)position + numBytes;
	if (newLength < oldLength)
		newLength = oldLength;
	unsigned char *data = new unsigned char[newLength];
	memset(data,0,newLength);			//Anything between the old end and position stays zero
	if (old != NULL)
		memcpy(data,old,oldLength);
	memcpy(data + position,buffer,numBytes);
	char *oldName = NULL;
	const char *resName;
	size_t useless;
	if (res->GetResourceInfo(type,id,&resName,&useless) && (resName != NULL)) {
		oldName = new char[strlen(resName) + 1];
		strcpy(oldName,resName);
	}
	res->RemoveResource(type,id);
	status_t status = res->AddResource(type,id,data,newLength,oldName);
	delete [] oldName;
	delete [] data;
	if (status != B_OK)
		return status;
	return numBytes;
}

size_t ResourceIO::Size(void) {
	char *name;
	size_t length;
	res->GetResourceInfo(type,id,(const char **)&name,&length);
	return length;
}

status_t ResourceIO::SetSize(off_t size) {
	unsigned char *buffer = new unsigned char[size];
	for (off_t i = 0; i < size; i++) {
		buffer[i] = 0;				//Zero the data
	}
	ReadAt(0,buffer,size);
	res->RemoveResource(type,id);
	res->AddResource(type,id,buffer,size,name);
	delete [] buffer;
	return B_OK;
}

int32 ResourceIO::ID(void) {
	return ((int32)(id));
}

//----------------FOR COMPATIBILITY ONLY----------------------

status_t get_app_resource(type_code type,long id,void** buffer/*<-- do not initialize*/,size_t *lengthFound) {
	const void *data = be_app->AppResources()->LoadResource(type,id,lengthFound);
	if (data == NULL)
		return B_ERROR;
	*buffer = new unsigned char[*lengthFound];
	memcpy(*buffer,data,*lengthFound);
	return B_OK;
}

status_t get_app_resource(type_code type,const char *name,void** buffer/*<-- do not initialize*/,size_t *lengthFound) {
	const void *data = be_app->AppResources()->LoadResource(type,name,lengthFound);
	if (data == NULL)
		return B_ERROR;
	*buffer = new unsigned char[*lengthFound];
	memcpy(*buffer,data,*lengthFound);
	return B_OK;
}

#include "nois.h"
#include "CSTR.h"
#include "cursor.h"
#include "bits.h"
#include "WIND.h"
