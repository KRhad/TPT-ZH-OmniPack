#ifndef AUTHORS_H
#define AUTHORS_H

#include "json/json.h"

extern Json::Value authors;

void MergeStampAuthorInfo(Json::Value stampAuthors);
void MergeAuthorInfo(Json::Value linksToAdd);
void SaveAuthorInfo(Json::Value *saveInto);

void DefaultSaveInfo();

#endif
