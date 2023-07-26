#include "./main_screen.hpp"

#include <imgui/imgui.h>

#include <memory>

MainScreen::MainScreen(std::string save_path) : rmm(cr), tc(save_path) {
	tel.subscribeAll(tc);

	conf.set("tox", "save_file_path", save_path);

	{ // name stuff
		auto name = tc.toxSelfGetName();
		if (name.empty()) {
			name = "tomato";
		}
		conf.set("tox", "name", name);
		tc.setSelfName(name); // TODO: this is ugly
	}

	// TODO: remove
	std::cout << "own address: " << tc.toxSelfGetAddressStr() << "\n";

	{ // setup plugin instances
		g_provideInstance<ConfigModelI>("ConfigModelI", "host", &conf);
		g_provideInstance<Contact3Registry>("Contact3Registry", "host", &cr);
		g_provideInstance<RegistryMessageModel>("RegistryMessageModel", "host", &rmm);

		g_provideInstance<ToxI>("ToxI", "host", &tc);
		g_provideInstance<ToxEventProviderI>("ToxEventProviderI", "host", &tc);

		// TODO: pm?

		// graphics
		g_provideInstance("ImGuiContext", "host", ImGui::GetCurrentContext());
		//g_provideInstance<TextureUploaderI>("TextureUploaderI", "host", &ogltu);
	}

	conf.dump();
}

Screen* MainScreen::poll(bool& quit) {
	auto new_time = std::chrono::high_resolution_clock::now();
	const float time_delta {std::chrono::duration<float, std::chrono::seconds::period>(new_time - last_time).count()};
	last_time = new_time;

	quit = !tc.iterate();

	pm.tick(time_delta);

	{
		bool open = !quit;
		ImGui::ShowDemoWindow(&open);
		quit = !open;
	}

	return nullptr;
}

