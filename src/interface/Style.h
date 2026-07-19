#ifndef STYLE_H
#define STYLE_H

#include "graphics/ARGBColour.h"
#include "graphics/Pixel.h"

namespace ui
{
class Style
{
public:
	/** The default color for window borders and all components */
	constexpr static pixel Border = COLARGB(255, 220, 220, 220);

	/** The amount to increase rgb values for certain components when they are clicked */
	constexpr static unsigned int ClickedModifier = 35;


	/** When components are disabled, rgb components are multiplied by this amount */
	constexpr static float DisabledMultiplier = 0.55f;

	/** When certain components are deselected, rgb components are multiplied by this amount */
	constexpr static float DeselectedMultiplier = 0.6f;


	/** When buttons are highlighted, they use the primary color with this alpha for the background */
	constexpr static unsigned int HighlightAlpha = 100;

	/** When buttons are highlighted, and the mouse is hovering over it, they use the primary color with this alpha for the background */
	constexpr static unsigned int HighlightAlphaHover = 155;

	/** When component are inverted, and the mouse is hovering over it, they use the primary color with this alpha for the background */
	constexpr static unsigned int InvertAlphaHover = 200;

	/** When the mouse hovers over certain components, they use the primary color with this alpha for the background */
	constexpr static unsigned int HoverAlpha = 100;
};
}
#endif // STYLE_H
