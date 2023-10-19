#pragma once

struct SimpleConfigModel;

class SettingsWindow {
	bool _show_window {false};
	// TODO: add iteration api to interface
	SimpleConfigModel& _conf;

	public:
		SettingsWindow(SimpleConfigModel& conf);

		void render(void);
};

