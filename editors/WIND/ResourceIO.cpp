//------------------------------------------------------------------------------
//	ResourceIO.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <storage/Resources.h>
#include <support/ClassInfo.h>
#include <translation/TranslationUtils.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "ResourceIO.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

const void *AppResource(type_code type,int32 id,size_t *lengthFound) {
	return be_app->AppResources()->LoadResource(type,id,lengthFound);
}

const void *AppResource(type_code type,const char *name,size_t *lengthFound) {
	return be_app->AppResources()->LoadResource(type,name,lengthFound);
}

//------------------------------------------------------------------------------
ResourceIO::ResourceIO(type_code type,int32 id,BResources *res) : BPositionIO() {
	this->id = id;
	this->type = type;
	this->res = res;
	name = NULL;
	curPos = 0;
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
ssize_t ResourceIO::Read(void *buffer,size_t numBytes) {
	ssize_t temp = ReadAt(curPos,buffer,numBytes);
	curPos += temp;
	return temp;
}
//------------------------------------------------------------------------------
ssize_t ResourceIO::ReadAt(off_t position,void *buffer,size_t numBytes) {
	size_t bytesRead;
	const void *buf = res->LoadResource(type,id,&bytesRead);
	if (buf == NULL)
		return 0;
	if (position < 0 || (size_t)position > bytesRead)
		return B_ERROR;
	if ((bytesRead - position) > numBytes)
		bytesRead = numBytes;
	else
		bytesRead -= position;
	memcpy(buffer,(((unsigned char *)(buf)) + position),bytesRead);
	return (ssize_t)(bytesRead);
}
//------------------------------------------------------------------------------
off_t ResourceIO::Seek(off_t position,uint32 mode) {
	size_t length = Size();
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
//------------------------------------------------------------------------------
off_t ResourceIO::Position(void) const{
	return (off_t)curPos;
}
//------------------------------------------------------------------------------
ssize_t ResourceIO::Write(const void *buffer,size_t numBytes) {
	ssize_t temp = WriteAt(curPos,buffer,numBytes);
	curPos += temp;
	return temp;
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
size_t ResourceIO::Size(void) {
	const char *name;
	size_t length;
	res->GetResourceInfo(type,id,&name,&length);
	return length;
}
//------------------------------------------------------------------------------
status_t ResourceIO::SetSize(off_t size) {
	unsigned char *buffer = new unsigned char[size];
	for (off_t i = 0; i < size; i++) {
		buffer[i] = 0;				//Zero the data
	}
	ReadAt(0,buffer,size);
	res->RemoveResource(type,id);
	res->AddResource(type,id,buffer,size,name);
	return B_OK;
}
//------------------------------------------------------------------------------
int32 ResourceIO::ID(void) {
	return ((int32)(id));
}
//------------------------------------------------------------------------------


//----------------FOR COMPATIBILITY ONLY----------------------------------------
status_t get_app_resource(type_code type,long id,void** buffer/*<-- do not initialize*/,size_t *lengthFound) {
	const void *data = be_app->AppResources()->LoadResource(type,id,lengthFound);
	if (data == NULL)
		return B_ERROR;
	*buffer = new unsigned char[*lengthFound];
	memcpy(*buffer,data,*lengthFound);
	return B_OK;
}
//------------------------------------------------------------------------------
status_t get_app_resource(type_code type,const char *name,void** buffer/*<-- do not initialize*/,size_t *lengthFound) {
	const void *data = be_app->AppResources()->LoadResource(type,name,lengthFound);
	if (data == NULL)
		return B_ERROR;
	*buffer = new unsigned char[*lengthFound];
	memcpy(*buffer,data,*lengthFound);
	return B_OK;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

