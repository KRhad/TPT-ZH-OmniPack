#pragma once

/**
 * @brief Reasons why a ui::Window was deleted
 */
enum DeleteReason
{
	NoDeleteReason, // Unspecified
	Confirmed,      // Enter was pressed or some confirm action was completed
	Escape,         // Escape or window decoration "X" button was pressed
	ExitButton,     // Cancel / Close button was pressed
	MouseOutside,   // Mouse click outside the window borders,
	Programatic     // Closed by Lua or via console command
};
