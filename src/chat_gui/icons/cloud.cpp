#include "./cloud.hpp"

#include <array>

void drawIconCloud(
	const ImVec2 p0,
	const ImVec2 p1_o,
	const ImU32 col_main,
	const ImU32 col_back
) {
	std::array<ImVec2, 19> points {{
		{0.2f, 0.9f},
		{0.8f, 0.9f},
		{0.9f, 0.8f},
		{0.9f, 0.7f},
		{0.7f, 0.7f},
		{0.9f, 0.5f},
		{0.9f, 0.4f},
		{0.8f, 0.2f},
		{0.6f, 0.2f},
		{0.5f, 0.3f},
		{0.5f, 0.5f},
		{0.4f, 0.4f},
		{0.3f, 0.4f},
		{0.2f, 0.5f},
		{0.2f, 0.6f},
		{0.3f, 0.7f},
		{0.1f, 0.7f},
		{0.1f, 0.8f},
		{0.2f, 0.9f},
	}};
	for (auto& v : points) {
		v.y -= 0.1f;
		v = {p0.x + p1_o.x*v.x, p0.y + p1_o.y*v.y};
	}
	// the circle looks bad in light mode
	//ImGui::GetWindowDrawList()->AddCircleFilled({p0.x + p1_o.x*0.5f, p0.y + p1_o.y*0.5f}, p1_o.x*0.5f, col_back);
	ImGui::GetWindowDrawList()->AddPolyline(points.data(), points.size(), col_back, ImDrawFlags_None, 4.f);
	ImGui::GetWindowDrawList()->AddPolyline(points.data(), points.size(), col_main, ImDrawFlags_None, 1.5f);
}

