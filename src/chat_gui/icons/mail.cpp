#include "./mail.hpp"

#include <array>

static void drawIconMailLines(
	const ImVec2 p0,
	const ImVec2 p1_o,
	const ImU32 col,
	const float thickness
) {
#define PLINE(x0, y0, x1, y1) \
		ImGui::GetWindowDrawList()->AddLine( \
			{p0.x + p1_o.x*(x0), p0.y + p1_o.y*(y0)}, \
			{p0.x + p1_o.x*(x1), p0.y + p1_o.y*(y1)}, \
			col, \
			thickness \
		);

		// quad
		// (1,2) -> (1,8)
		PLINE(0.1f, 0.2f, 0.1f, 0.8f)
		// (1,8) -> (9,8)
		PLINE(0.1f, 0.8f, 0.9f, 0.8f)
		// (9,8) -> (9,2)
		PLINE(0.9f, 0.8f, 0.9f, 0.2f)
		// (9,2) -> (1,2)
		PLINE(0.9f, 0.2f, 0.1f, 0.2f)

		// lip
		// (1,2) -> (5,5)
		PLINE(0.1f, 0.2f, 0.5f, 0.5f)
		// (5,5) -> (9,2)
		PLINE(0.5f, 0.5f, 0.9f, 0.2f)

#undef PLINE
}

void drawIconMail(
	const ImVec2 p0,
	const ImVec2 p1_o,
	const ImU32 col_main,
	const ImU32 col_back
) {
	// dark background
	// the circle looks bad in light mode
	//ImGui::GetWindowDrawList()->AddCircleFilled({p0.x + p1_o.x*0.5f, p0.y + p1_o.y*0.5f}, p1_o.x*0.5f, col_back);
	drawIconMailLines(p0, p1_o, col_back, 4.0f);
	drawIconMailLines(p0, p1_o, col_main, 1.5f);
}

