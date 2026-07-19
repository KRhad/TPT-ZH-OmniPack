#ifndef SIGN_H
#define SIGN_H
#include <string>
#include <vector>
#include "common/Point.h"

class Simulation;
class Sign
{
public:
	enum Justification { Left, Middle, Right, NoJustification, Max };
	enum Type { Normal = 0, SaveLink = 1, ThreadLink = 2, Spark = 3, SearchLink = 4 };

private:
	std::string text, displayText, linkText;
	int x, y;
	Justification ju;
	Type type;
	bool isV95 = false;

public:
	Sign(std::string text, int x, int y, Justification justification);
	Sign(const Sign & sign);

	void SetText(std::string newText);
	std::string GetText() const { return text; }
	std::string GetLinkText() const { return linkText; }
	std::string GetDisplayText(Simulation * sim, bool *v95 = nullptr) const;

	void SetJustification(Justification newJustification) { ju = newJustification; }
	Justification GetJustification() const { return ju; }
	//void SetType(Type newType);
	Type GetType() const { return type; }

	Point GetRealPos() const { return Point(x, y); }
	void GetPos(Simulation * sim, int & x0, int & y0, int & w, int & h) const;
	void SetPos(Point newPos) { x = newPos.X; y = newPos.Y; }
	bool IsSignInArea(Point topLeft, Point bottomRight);
};

#define MAXSIGNS 16
extern std::vector<Sign> signs;
extern int MSIGN;

void ClearSigns();
void DeleteSignsInArea(Point topLeft, Point bottomRight);
int InsideSign(Simulation * sim, int mx, int my, bool allsigns);

#endif
