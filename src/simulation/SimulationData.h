#ifndef SIMULATIONDATA_H
#define SIMULATIONDATA_H

#define R_TEMP 22
#define MAX_TEMP 9999
#define MIN_TEMP 0
#define O_MAX_TEMP 3500
#define O_MIN_TEMP -273

constexpr float MAX_PRESSURE = 256.0f;
constexpr float MIN_PRESSURE = -256.0f;

#define TYPE_PART			0x0000001 //1 Powders
#define TYPE_LIQUID			0x0000002 //2 Liquids
#define TYPE_SOLID			0x0000004 //4 Solids
#define TYPE_GAS			0x0000008 //8 Gases (Includes plasma)
#define TYPE_ENERGY			0x0000010 //16 Energy (Thunder, Light, Neutrons etc.)
#define STATE_FLAGS			0x000001F
#define PROP_CONDUCTS		0x0000020 //32 Conducts electricity
#define PROP_BLACK			0x0000040 //64 Absorbs Photons (not currently implemented or used, a photwl attribute might be better)
#define PROP_NEUTPENETRATE	0x0000080 //128 Penetrated by neutrons
#define PROP_NEUTABSORB		0x0000100 //256 Absorbs neutrons, reflect is default
#define PROP_NEUTPASS		0x0000200 //512 Neutrons pass through, such as with glass
#define PROP_DEADLY			0x0000400 //1024 Is deadly for stickman
#define PROP_HOT_GLOW		0x0000800 //2048 Hot Metal Glow
#define PROP_LIFE			0x0001000 //4096 Is a GoL type
#define PROP_RADIOACTIVE	0x0002000 //8192 Radioactive
#define PROP_LIFE_DEC		0x0004000 //2^14 Life decreases by one every frame if > zero
#define PROP_LIFE_KILL		0x0008000 //2^15 Kill when life value is <= zero
#define PROP_LIFE_KILL_DEC	0x0010000 //2^16 Kill when life value is decremented to <= zero
#define PROP_INDESTRUCTIBLE	0x0020000 //2^17 Makes elements invincible, even to bomb/dest
#define PROP_CLONE			0x0040000 //2^18 Makes elements clone things that touch it
#define PROP_BREAKABLECLONE	0x0080000 //2^19 Makes breakable elements clone things that touch it
#define PROP_POWERED		0x0100000 //2^20 Makes an element turn on/off with PSCN/NSCN
#define PROP_SPARKSETTLE	0x0200000 //2^21 Allow Sparks/Embers to settle
#define PROP_NOAMBHEAT      0x0400000 //2^23 Don't transfer or receive heat from ambient heat.
#define PROP_NOCTYPEDRAW	0x1000000 //2^25 When this element is drawn upon with, do not set ctype (like BCLN for CLNE)

#define FLAG_STAGNANT	0x1
#define FLAG_SKIPMOVE	0x2  // Skip movement for one frame
//#define FLAG_WATEREQUAL 0x4  // If a liquid was already checked during equalization
#define FLAG_PHOTDECO	0x8  // compatibility with old saves (decorated photons), only applies to PHOT.
#define FLAG_EXPLODE	0x10 // EXPL explosion
#define FLAG_DISAPPEAR	0x20 // Will disappear on next frame no matter what

enum EdgeMode
{
	EDGE_VOID, EDGE_SOLID, EDGE_LOOP, NUM_EDGEMODES
};

enum AirMode
{
	AIR_ON, AIR_PRESSUREOFF, AIR_VELOCITYOFF, AIR_OFF, AIR_NOUPDATE, NUM_AIRMODES
};

enum GravityMode
{
	GRAV_VERTICAL, GRAV_OFF, GRAV_RADIAL, GRAV_CUSTOM, NUM_GRAVMODES
};

enum DecoSpace
{
	DECOSPACE_SRGB,
	DECOSPACE_LINEAR,
	DECOSPACE_GAMMA22,
	DECOSPACE_GAMMA18,
	NUM_DECOSPACES,
};


// Change this to change the amount of bits used to store type in pmap (and a few elements such as PIPE and CRAY)
#define PMAPBITS 9
#define PMAPMASK ((1<<PMAPBITS)-1)
#define ID(r) ((r)>>PMAPBITS)
#define TYP(r) ((r)&PMAPMASK)
#define PMAP(id, typ) ((id)<<PMAPBITS | ((typ)&PMAPMASK))
#define PMAPID(id) ((id)<<PMAPBITS)

#define PT_NUM	(1<<PMAPBITS)

#if PMAPBITS > 16
#error PMAPBITS is too large
#endif
#if ((XRES*YRES)<<PMAPBITS) > 0x100000000L
#error not enough space in pmap
#endif


// Defines for element transitions
#define IPL -257.0f
#define IPH 257.0f
#define ITL MIN_TEMP-1
#define ITH MAX_TEMP+1
// no transition (PT_NONE means kill part)
#define NT -1
// special transition - lava ctypes etc need extra code, which is only found and run if ST is given
#define ST PT_NUM

#endif
