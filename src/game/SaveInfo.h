#ifndef SAVEINFO_H
#define SAVEINFO_H

#include <string>
//#include <vector>

/* Stores current opened save / local save info */

class SaveInfo
{
	bool saveOpened;
	bool fileOpened;

	std::string saveName;
	std::string fileName;

	// these are only used for online saves
	int saveID;
	std::string saveVersion;
	bool published;
	std::string description;
	std::string author;
	//std::vector<std::string> tags;
	// TODO: tags should be a list, not a space separated string
	std::string tags;
	int myVote;

public:
	SaveInfo();
	SaveInfo(const SaveInfo &other);

	bool GetSaveOpened() const;
	void SetSaveOpened(bool value);
	bool GetFileOpened() const;
	void SetFileOpened(bool value);

	std::string GetSaveName() const;
	void SetSaveName(const std::string &value);
	std::string GetFileName() const;
	void SetFileName(const std::string &value);

	int GetSaveID() const;
	void SetSaveID(int value);
	std::string GetSaveVersion() const;
	void SetSaveVersion(std::string value);
	bool GetPublished() const;
	void SetPublished(bool value);
	std::string GetDescription() const;
	void SetDescription(const std::string &value);
	std::string GetAuthor() const;
	void SetAuthor(const std::string &value);
	//std::vector<std::string> GetTags() const;
	//void SetTags(const std::vector<std::string> &value);
	std::string GetTags() const;
	void SetTags(const std::string &value);
	int GetMyVote() const;
	void SetMyVote(int value);
};

#endif
