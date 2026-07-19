#ifndef GOLNUMBERS_H
#define GOLNUMBERS_H
#include <list>
#include <string>
#include "graphics/ARGBColour.h"
#include "graphics/Pixel.h"

#define NGOL 24

//Old IDs for GOL types
#define GT_GOL 78
#define GT_HLIF 79
#define GT_ASIM 80
#define GT_2x2 81
#define GT_DANI 82
#define GT_AMOE 83
#define GT_MOVE 84
#define GT_PGOL 85
#define GT_DMOE 86
#define GT_34 87
#define GT_LLIF 88
#define GT_STAN 89
#define GT_SEED 134
#define GT_MAZE 135
#define GT_COAG 136
#define GT_WALL 137
#define GT_GNAR 138
#define GT_REPL 139
#define GT_MYST 140
#define GT_LOTE 142
#define GT_FRG2 143
#define GT_STAR 144
#define GT_FROG 145
#define GT_BRAN 146

//New IDs for GOL types
#define NGT_GOL 0
#define NGT_HLIF 1
#define NGT_ASIM 2
#define NGT_2x2 3
#define NGT_DANI 4
#define NGT_AMOE 5
#define NGT_MOVE 6
#define NGT_PGOL 7
#define NGT_DMOE 8
#define NGT_34 9
#define NGT_LLIF 10
#define NGT_STAN 11
#define NGT_SEED 12
#define NGT_MAZE 13
#define NGT_COAG 14
#define NGT_WALL 15
#define NGT_GNAR 16
#define NGT_REPL 17
#define NGT_MYST 18
#define NGT_LOTE 19
#define NGT_FRG2 20
#define NGT_STAR 21
#define NGT_FROG 22
#define NGT_BRAN 23

const int oldgolTypes[NGOL] =
{
	GT_GOL,
	GT_HLIF,
	GT_ASIM,
	GT_2x2,
	GT_DANI,
	GT_AMOE,
	GT_MOVE,
	GT_PGOL,
	GT_DMOE,
	GT_34,
	GT_LLIF,
	GT_STAN,
	GT_SEED,
	GT_MAZE,
	GT_COAG,
	GT_WALL,
	GT_GNAR,
	GT_REPL,
	GT_MYST,
	GT_LOTE,
	GT_FRG2,
	GT_STAR,
	GT_FROG,
	GT_BRAN,
};

struct BuiltinGOL
{
	std::string name;
	int oldtype;
	int ruleset;
	pixel color, color2;
	int goltype;
	std::string description;
};

const BuiltinGOL builtinGol[NGOL] = {
	// * Ruleset:
	//   * bits x = 8..0: stay if x neighbours present
	//   * bits x = 16..9: begin if x-8 neighbours present
	//   * bits 20..17: 4-bit unsigned int encoding the number of states minus 2; 2 states is
	//     encoded as 0, 3 states as 1, etc.
	//   * states are kind of long until a cell dies; normal ones use two states (living and dead),
	//     for others the intermediate states live but do nothing
	//   * the ruleset constants below look 20-bit, but rulesets actually consist of 21
	//     bits of data; bit 20 just happens to not be set for any of the built-in types,
	//     as none of them have 10 or more states
	{ "GOL",  GT_GOL , 0x0080C, COLPACK(0x0CAC00), COLPACK(0x0CAC00), NGT_GOL,  "Game Of Life: Begin 3/Stay 23" },
	{ "HLIF", GT_HLIF, 0x0480C, COLPACK(0xFF0000), COLPACK(0xFF0000), NGT_HLIF, "High Life: B36/S23" },
	{ "ASIM", GT_ASIM, 0x038F0, COLPACK(0x0000FF), COLPACK(0x0000FF), NGT_ASIM, "Assimilation: B345/S4567" },
	{ "2X2",  GT_2x2 , 0x04826, COLPACK(0xFFFF00), COLPACK(0xFFFF00), NGT_2x2,  "2X2: B36/S125" },
	{ "DANI", GT_DANI, 0x1C9D8, COLPACK(0x00FFFF), COLPACK(0x00FFFF), NGT_DANI, "Day and Night: B3678/S34678" },
	{ "AMOE", GT_AMOE, 0x0A92A, COLPACK(0xFF00FF), COLPACK(0xFF00FF), NGT_AMOE, "Amoeba: B357/S1358" },
	{ "MOVE", GT_MOVE, 0x14834, COLPACK(0xFFFFFF), COLPACK(0xFFFFFF), NGT_MOVE, "'Move' particles. Does not move things.. it is a life type: B368/S245" },
	{ "PGOL", GT_PGOL, 0x0A90C, COLPACK(0xE05010), COLPACK(0xE05010), NGT_PGOL, "Pseudo Life: B357/S238" },
	{ "DMOE", GT_DMOE, 0x1E9E0, COLPACK(0x500000), COLPACK(0x500000), NGT_DMOE, "Diamoeba: B35678/S5678" },
	{ "3-4",  GT_34  , 0x01818, COLPACK(0x500050), COLPACK(0x500050), NGT_34,   "3-4: B34/S34" },
	{ "LLIF", GT_LLIF, 0x03820, COLPACK(0x505050), COLPACK(0x505050), NGT_LLIF, "Long Life: B345/S5" },
	{ "STAN", GT_STAN, 0x1C9EC, COLPACK(0x5000FF), COLPACK(0x5000FF), NGT_STAN, "Stains: B3678/S235678" },
	{ "SEED", GT_SEED, 0x00400, COLPACK(0xFBEC7D), COLPACK(0xFBEC7D), NGT_SEED, "Seeds: B2/S" },
	{ "MAZE", GT_MAZE, 0x0083E, COLPACK(0xA8E4A0), COLPACK(0xA8E4A0), NGT_MAZE, "Maze: B3/S12345" },
	{ "COAG", GT_COAG, 0x189EC, COLPACK(0x9ACD32), COLPACK(0x9ACD32), NGT_COAG, "Coagulations: B378/S235678" },
	{ "WALL", GT_WALL, 0x1F03C, COLPACK(0x0047AB), COLPACK(0x0047AB), NGT_WALL, "Walled cities: B45678/S2345" },
	{ "GNAR", GT_GNAR, 0x00202, COLPACK(0xE5B73B), COLPACK(0xE5B73B), NGT_GNAR, "Gnarl: B1/S1" },
	{ "REPL", GT_REPL, 0x0AAAA, COLPACK(0x259588), COLPACK(0x259588), NGT_REPL, "Replicator: B1357/S1357" },
	{ "MYST", GT_MYST, 0x139E1, COLPACK(0x0C3C00), COLPACK(0x0C3C00), NGT_MYST, "Mystery: B3458/S05678" },
	{ "LOTE", GT_LOTE, 0x48938, COLPACK(0xFF0000), COLPACK(0xFFFF00), NGT_LOTE, "Living on the Edge: B37/S3458/4" },
	{ "FRG2", GT_FRG2, 0x20816, COLPACK(0x006432), COLPACK(0x00FF5A), NGT_FRG2, "Like Frogs rule: B3/S124/3" },
	{ "STAR", GT_STAR, 0x98478, COLPACK(0x000040), COLPACK(0x0000E6), NGT_STAR, "Like Star Wars rule: B278/S3456/6" },
	{ "FROG", GT_FROG, 0x21806, COLPACK(0x006400), COLPACK(0x00FF00), NGT_FROG, "Frogs: B34/S12/3" },
	{ "BRAN", GT_BRAN, 0x25440, COLPACK(0xFFFF00), COLPACK(0x969600), NGT_BRAN, "Brian 6: B246/S6/3" }
};

#endif
