#include <algorithm>
#include "Favorite.h"

Favorite::Favorite():
	favorites(std::vector<std::string>()),
	recents(std::deque<std::string>())
{
	
}

void Favorite::AddFavorite(std::string identifier)
{
	if (!IsFavorite(identifier))
		favorites.push_back(identifier);
	RemoveRecent(identifier);
}

bool Favorite::RemoveFavorite(std::string identifier)
{
	return favorites.erase(std::remove(favorites.begin(), favorites.end(), identifier), favorites.end()) != favorites.end();
}

bool Favorite::IsFavorite(std::string identifier)
{
	return std::find(favorites.begin(), favorites.end(), identifier) != favorites.end();
}

void Favorite::AddRecent(std::string identifier)
{
	if (!IsRecent(identifier) && !IsFavorite(identifier))
	{
		recents.push_back(identifier);
		if (recents.size() > 20)
			recents.pop_front();
	}
}

bool Favorite::RemoveRecent(std::string identifier)
{
	return recents.erase(std::remove(recents.begin(), recents.end(), identifier), recents.end()) != recents.end();
}

bool Favorite::IsRecent(std::string identifier)
{
	return std::find(recents.begin(), recents.end(), identifier) != recents.end();
}

int Favorite::GetSize(bool favoritesOnly)
{
	int size = favorites.size();
	if (!favoritesOnly)
		size += recents.size();
	return size;
}

std::vector<std::string> Favorite::BuildFavoritesList(bool favoritesOnly)
{
	std::vector<std::string> builtList = std::vector<std::string>();
	builtList.insert(builtList.end(), favorites.begin(), favorites.end());
	if (!favoritesOnly)
		builtList.insert(builtList.end(), recents.begin(), recents.end());
	return builtList;
}
