#ifndef FAVORITE_H
#define FAVORITE_H

#include <string>
#include <vector>
#include <deque>
#include "common/Singleton.h"

class Favorite : public Singleton<Favorite>
{
	std::vector<std::string> favorites;
	std::deque<std::string> recents;
public:
	Favorite();
	
	void AddFavorite(std::string identifier);
	bool RemoveFavorite(std::string identifier);
	bool IsFavorite(std::string identifier);
	
	void AddRecent(std::string identifier);
	bool RemoveRecent(std::string identifier);
	bool IsRecent(std::string identifier);
	
	int GetSize(bool favoritesOnly=false);
	std::vector<std::string> BuildFavoritesList(bool favoritesOnly=false);
};

#endif // FAVORITE_H
