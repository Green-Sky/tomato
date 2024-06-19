#include "./direct.hpp"

#include <array>

static void drawIconDirectLines(
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

		// arrow 1
		// (3,1) -> (9,7)
		PLINE(0.3f, 0.1f, 0.9f, 0.7f)
		// (9,7) -> (9,5)
		PLINE(0.9f, 0.7f, 0.9f, 0.5f)
		// (9,7) -> (7,7)
		PLINE(0.9f, 0.7f, 0.7f, 0.7f)

		// arrow 2
		// (7,9) -> (1,3)
		PLINE(0.7f, 0.9f, 0.1f, 0.3f)
		// (1,3) -> (3,3)
		PLINE(0.1f, 0.3f, 0.3f, 0.3f)
		// (1,3) -> (1,5)
		PLINE(0.1f, 0.3f, 0.1f, 0.5f)
#undef PLINE
}

void drawIconDirect(
	const ImVec2 p0,
	const ImVec2 p1_o,
	const ImU32 col_main,
	const ImU32 col_back
) {
	// dark background
	// the circle looks bad in light mode
	//ImGui::GetWindowDrawList()->AddCircleFilled({p0.x + p1_o.x*0.5f, p0.y + p1_o.y*0.5f}, p1_o.x*0.5f, col_back);
	drawIconDirectLines(p0, p1_o, col_back, 4.0f);
	drawIconDirectLines(p0, p1_o, col_main, 1.5f);
}

