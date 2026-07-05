#pragma once

class ChatGui4; // forward declaration

constexpr float LAYOUT_THRESHOLD_MOBILE_FONT_UNITS = 100.f;

class LayoutStrategy {
	public:
		virtual ~LayoutStrategy(void) = default;
		virtual void render(ChatGui4& gui, const float time_delta, const bool window_focused) = 0;
};

// old two plane layout. contact sidebar + chat window
class DesktopLayout final : public LayoutStrategy {
	public:
		void render(ChatGui4& gui, const float time_delta, const bool window_focused) override;
};

// single plane stack layout
class SinglePlaneLayout final : public LayoutStrategy {
	public:
		void render(ChatGui4& gui, const float time_delta, const bool window_focused) override;
};
