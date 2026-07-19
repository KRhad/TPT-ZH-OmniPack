/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SaveInfo.h"

SaveInfo::SaveInfo():
	saveOpened(false),
	fileOpened(false),
	saveName(""),
	fileName(""),
	saveID(-1),
	published(false),
	description(""),
	author(""),
	tags(""),
	myVote(0)
{
	
}

SaveInfo::SaveInfo(const SaveInfo &other):
	saveOpened(other.saveOpened),
	fileOpened(other.fileOpened),
	saveName(other.saveName),
	fileName(other.fileName),
	saveID(other.saveID),
	published(other.published),
	description(other.description),
	author(other.author),
	tags(other.tags),
	myVote(other.myVote)
{
	
}


bool SaveInfo::GetSaveOpened() const
{
	return saveOpened;
}

void SaveInfo::SetSaveOpened(bool value)
{
	saveOpened = value;
}

bool SaveInfo::GetFileOpened() const
{
	return fileOpened;
}

void SaveInfo::SetFileOpened(bool value)
{
	fileOpened = value;
}

std::string SaveInfo::GetSaveName() const
{
	return saveName;
}

void SaveInfo::SetSaveName(const std::string &value)
{
	saveName = value.substr(0, 63);
}

std::string SaveInfo::GetFileName() const
{
	return fileName;
}

void SaveInfo::SetFileName(const std::string &value)
{
	fileName = value; //.substr(0, 254)
}

int SaveInfo::GetSaveID() const
{
	return saveID;
}

void SaveInfo::SetSaveID(int value)
{
	saveID = value; // capped at length 15 in tpt++
}

std::string SaveInfo::GetSaveVersion() const
{
	return saveVersion;
}

void SaveInfo::SetSaveVersion(std::string value)
{
	saveVersion = value;
}

bool SaveInfo::GetPublished() const
{
	return published;
}

void SaveInfo::SetPublished(bool value)
{
	published = value;
}

std::string SaveInfo::GetDescription() const
{
	return description;
}

void SaveInfo::SetDescription(const std::string &value)
{
	description = value; //.substr(0, 254)
}

std::string SaveInfo::GetAuthor() const
{
	return author;
}

void SaveInfo::SetAuthor(const std::string &value)
{
	author = value; //.substr(0, 63)
}

std::string SaveInfo::GetTags() const
{
	return tags;
}

void SaveInfo::SetTags(const std::string &value)
{
	tags = value; //.substr(0, 255)
}

int SaveInfo::GetMyVote() const
{
	return myVote;
}

void SaveInfo::SetMyVote(int value)
{
	myVote = value;
}
