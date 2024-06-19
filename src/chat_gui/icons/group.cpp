#include "./group.hpp"

#include <array>

static void drawIconGroupLines(
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

		// person front
		//  x     y
		PLINE(
			0.1f, 0.9f,
			0.6f, 0.9f
		)
		PLINE(
			0.6f, 0.9f,
			0.6f, 0.7f
		)
		PLINE(
			0.6f, 0.7f,
			0.5f, 0.6f
		)
		PLINE(
			0.5f, 0.6f,
			0.4f, 0.6f
		)
		PLINE(
			0.4f, 0.6f,
			0.5f, 0.5f
		)
		PLINE(
			0.5f, 0.5f,
			0.5f, 0.4f
		)
		PLINE(
			0.5f, 0.4f,
			0.4f, 0.3f
		)
		PLINE(
			0.4f, 0.3f,
			0.3f, 0.3f
		)
		PLINE(
			0.3f, 0.3f,
			0.2f, 0.4f
		)
		PLINE(
			0.2f, 0.4f,
			0.2f, 0.5f
		)
		PLINE(
			0.2f, 0.5f,
			0.3f, 0.6f
		)
		PLINE(
			0.3f, 0.6f,
			0.2f, 0.6f
		)
		PLINE(
			0.2f, 0.6f,
			0.1f, 0.7f
		)
		PLINE(
			0.1f, 0.7f,
			0.1f, 0.9f
		)

		// person back
		//  x     y
		PLINE(
			0.7f, 0.7f,
			0.9f, 0.7f
		)
		PLINE(
			0.9f, 0.7f,
			0.9f, 0.5f
		)
		PLINE(
			0.9f, 0.5f,
			0.8f, 0.4f
		)
		PLINE(
			0.8f, 0.4f,
			0.7f, 0.4f
		)
		PLINE(
			0.7f, 0.4f,
			0.8f, 0.3f
		)
		PLINE(
			0.8f, 0.3f,
			0.8f, 0.2f
		)
		PLINE(
			0.8f, 0.2f,
			0.7f, 0.1f
		)
		PLINE(
			0.7f, 0.1f,
			0.6f, 0.1f
		)
		PLINE(
			0.6f, 0.1f,
			0.5f, 0.2f
		)
		PLINE(
			0.5f, 0.2f,
			0.5f, 0.3f
		)
		PLINE(
			0.5f, 0.3f,
			0.6f, 0.4f
		)

#undef PLINE
}

void drawIconGroup(
	const ImVec2 p0,
	const ImVec2 p1_o,
	const ImU32 col_main
) {
	// dark background
	// the circle looks bad in light mode
	//ImGui::GetWindowDrawList()->AddCircleFilled({p0.x + p1_o.x*0.5f, p0.y + p1_o.y*0.5f}, p1_o.x*0.5f, col_back);
	//drawIconGroupLines(p0, p1_o, col_back, 3.5f);
	drawIconGroupLines(p0, p1_o, col_main, 1.0f);
}

