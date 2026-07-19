#include <sstream>
#include <iomanip>
#include "Sign.h"
#include "powder.h"
#include "graphics/VideoBuffer.h"
#include "simulation/Simulation.h"

std::vector<Sign> signs;
int MSIGN = -1;

void ClearSigns()
{
	signs.clear();

	MSIGN = -1;
}

void DeleteSignsInArea(Point topLeft, Point bottomRight)
{
	for (int i = signs.size()-1; i >= 0; i--)
	{
		Point realPos = signs[i].GetRealPos();
		if (realPos.X >= topLeft.X && realPos.Y >= topLeft.Y && realPos.X < bottomRight.X && realPos.Y < bottomRight.Y)
		{
			signs.erase(signs.begin()+i);
		}
	}
}

// returns -1 if mouse is not inside a link sign, else returns the sign id
// allsigns argument makes it return whether inside any sign (not just link signs)
int InsideSign(Simulation * sim, int mx, int my, bool allsigns)
{
	int x, y, w, h;
	for (int i = (int)signs.size()-1; i >= 0; i--)
	{
		signs[i].GetPos(sim, x, y, w, h);
		if (mx >= x && mx <= x+w && my >= y && my <= y+h)
		{
			if (allsigns || signs[i].GetType() != Sign::Normal)
				return i;
		}
	}
	return -1;
}

Sign::Sign(std::string text, int x, int y, Justification justification):
	x(x),
	y(y),
	ju(justification),
	type(Normal)
{
	SetText(text);
}

Sign::Sign(const Sign & sign):
	ju(sign.GetJustification())
{
	SetPos(sign.GetRealPos());
	SetText(sign.GetText());
}

void Sign::SetText(std::string newText)
{
	type = Normal;
	text = displayText = newText;
	linkText = "";

	// split link signs into parts here
	int len = text.length(), splitStart;
	if (len > 2 && text[0] == '{')
	{
		switch(text[1])
		{
		case 'c':
		case 't':
			if (text[2] == ':' && text[3] >= '0' && text[3] <= '9')
			{
				splitStart = 4;
				while (splitStart < len && text[splitStart] >= '0' && text[splitStart] <= '9')
					splitStart++;
				linkText = text.substr(3, splitStart-3);
				type = text[1] == 'c' ? SaveLink : ThreadLink;
			}
			else
				return;
			break;
		case 'b':
			splitStart = 2;
			type = Spark;
			break;
		case 's':
			splitStart = 4;
			while (splitStart < len && text[splitStart] != '|')
				splitStart++;
			linkText = text.substr(3, splitStart-3);
			type = SearchLink;
			break;
		default:
			return;
		}

		if (text[splitStart] == '|' && text[len-1] == '}')
		{
			displayText = text.substr(splitStart+1, len-splitStart-2);
		}
		else
		{
			linkText = text;
			type = Normal;
			return;
		}
	}
}

const particle *GetParticleAt(Simulation *sim, int x, int y)
{
	if (!sim)
		return nullptr;
	if (photons[y][x])
		return &sim->parts[ID(photons[y][x])];
	else if (pmap[y][x])
		return &sim->parts[ID(pmap[y][x])];
	return nullptr;
}

std::string Sign::GetDisplayText(Simulation * sim, bool *v95) const
{
	if (type == Normal && text.length() && x >= 0 && x < XRES && y >= 0 && y < YRES)
	{
		std::stringstream displayTextStream;

		size_t left_curly_pos, right_curly_pos = -1, last_appended_pos = 0;
		while ((left_curly_pos = text.find('{', right_curly_pos + 1)) != text.npos)
		{
			displayTextStream << text.substr(right_curly_pos + 1, left_curly_pos - (right_curly_pos + 1));
			last_appended_pos = left_curly_pos;

			right_curly_pos = text.find('}', left_curly_pos + 1);
			if (right_curly_pos == text.npos)
				break;
			last_appended_pos = right_curly_pos + 1;

			std::string between_curlies = text.substr(left_curly_pos + 1, right_curly_pos - (left_curly_pos + 1));
			if (between_curlies == "t" || between_curlies == "temp")
			{
				const particle *part = GetParticleAt(sim, x, y);
				if (part)
					displayTextStream << std::fixed << std::setprecision(2) << parts[ID(pmap[y][x])].temp-273.15f;
				else
					displayTextStream << "N/A";
				// * We would really only need to do this if the sign used the new
				//   keyword "temp" or if the text was more than just "{t}", but 95.0
				//   upgrades such signs at load time anyway.
				// * The same applies to "{p}" and "{aheat}" signs.
				if (v95)
					*v95 = true;
			}
			else if (between_curlies == "p" || between_curlies == "pres")
			{
				float pres = 0.0f;
				if (sim)
					pres = sim->air->pv[y/CELL][x/CELL];
				displayTextStream << std::fixed << std::setprecision(2) << pres;
				if (v95)
					*v95 = true;
			}
			else if (between_curlies == "a" || between_curlies == "aheat")
			{
				float aheat = 0.0f;
				if (sim)
					aheat = sim->air->hv[y/CELL][x/CELL];
				displayTextStream << std::fixed << std::setprecision(2) << aheat;
				if (v95)
					*v95 = true;
			}
			else if (between_curlies == "type")
			{
				const particle *part = GetParticleAt(sim, x, y);
				if (part)
					displayTextStream << sim->ElementResolve(part->type, part->ctype);
				else if (displayTextStream.str().empty())
					displayTextStream << "Empty";
				else
					displayTextStream << "empty";
				if (v95)
					*v95 = true;
			}
			else if (between_curlies == "ctype")
			{
				const particle *part = GetParticleAt(sim, x, y);
				if (part)
				{
					int ctype = part->ctype;
					if (sim->IsElementOrNone(ctype))
						displayTextStream << sim->ElementResolve(ctype, 0);
					else
						displayTextStream << ctype;
				}
				else if (displayTextStream.str().empty())
					displayTextStream << "Empty";
				else
					displayTextStream << "empty";
				if (v95)
					*v95 = true;
			}
			else if (between_curlies == "life")
			{
				const particle *part = GetParticleAt(sim, x, y);
				displayTextStream << (part ? part->life : 0);
				if (v95)
					*v95 = true;
			}
			else if (between_curlies == "tmp")
			{
				const particle *part = GetParticleAt(sim, x, y);
				displayTextStream << (part ? part->tmp : 0);
				if (v95)
					*v95 = true;
			}
			else if (between_curlies == "tmp2")
			{
				const particle *part = GetParticleAt(sim, x, y);
				displayTextStream << (part ? part->tmp2 : 0);
				if (v95)
					*v95 = true;
			}
			else
				displayTextStream << '{' << between_curlies << '}';
		}
		if (right_curly_pos == (size_t)-1)
			return text;
		else
			displayTextStream << text.substr(last_appended_pos);

		return displayTextStream.str();
	}
	return displayText;
}

void Sign::GetPos(Simulation * sim, int & x0, int & y0, int & w, int & h) const
{
	w = gfx::VideoBuffer::TextSize(GetDisplayText(sim)).X + 4;
	h = 14;
	x0 = (ju == Right) ? x - w :
		  (ju == Left) ? x : x - w/2;
	y0 = (y > 18) ? y - 18 : y + 4;
}

bool Sign::IsSignInArea(Point topLeft, Point bottomRight)
{
	return (x >= topLeft.X && y >= topLeft.Y && x < bottomRight.X && y < bottomRight.Y);
}
