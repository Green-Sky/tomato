#pragma once

struct ConfigModelI;

class SettingsWindow {
	bool _show_window {false};
	ConfigModelI& _conf;

	public:
		SettingsWindow(ConfigModelI& conf);

		void render(void);
};

