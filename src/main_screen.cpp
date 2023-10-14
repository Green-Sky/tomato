#include "./main_screen.hpp"

#include <imgui/imgui.h>

#include <SDL3/SDL.h>

#include <memory>

MainScreen::MainScreen(SDL_Renderer* renderer_, std::string save_path, std::string save_password, std::vector<std::string> plugins) :
	renderer(renderer_),
	rmm(cr),
	mts(rmm),
	tc(save_path, save_password),
	ad(tc),
	tcm(cr, tc, tc),
	tmm(rmm, cr, tcm, tc, tc),
	ttm(rmm, cr, tcm, tc, tc),
	mmil(rmm),
	tam(rmm, cr, conf),
	sdlrtu(renderer_),
	cg(conf, rmm, cr, sdlrtu)
{
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
		g_provideInstance<ToxContactModel2>("ToxContactModel2", "host", &tcm);

		// TODO: pm?

		// graphics
		g_provideInstance("ImGuiContext", "host", ImGui::GetCurrentContext());
		g_provideInstance<TextureUploaderI>("TextureUploaderI", "host", &sdlrtu);
	}

	for (const auto& ppath : plugins) {
		if (!pm.add(ppath)) {
			std::cerr << "MS error: loading plugin '" << ppath << "' failed!\n";
			// thow?
			assert(false && "failed to load plugin");
		}
	}

	conf.dump();
}

MainScreen::~MainScreen(void) {
}

bool MainScreen::handleEvent(SDL_Event& e) {
	return false;
}

Screen* MainScreen::poll(bool& quit) {
	auto new_time = std::chrono::high_resolution_clock::now();
	const float time_delta {std::chrono::duration<float, std::chrono::seconds::period>(new_time - last_time).count()};
	last_time = new_time;

	quit = !tc.iterate();

	tcm.iterate(time_delta);

	tam.iterate();

	pm.tick(time_delta);

	mts.iterate();

	cg.render();

	if constexpr (false) {
		ImGui::ShowDemoWindow();
	}

#if 0
	{ // texture tests
		const size_t width = 8;
		const size_t height = 8;
#define W 0xff, 0xff, 0xff, 0xff
#define B 0x00, 0x00, 0x00, 0xff
#define P 0xff, 0x00, 0xff, 0xff
		static uint8_t raw_pixel[width*height*4] {
			P, W, W, W, W, W, W, P,
			W, W, W, W, W, W, W, W,
			W, W, W, W, W, W, W, W,
			W, W, W, B, B, W, W, W,
			W, W, W, B, B, W, W, W,
			W, W, W, W, W, W, W, W,
			W, W, W, W, W, W, W, W,
			P, W, W, W, W, W, W, P,
		};

		static uint64_t texture = sdlrtu.uploadRGBA(raw_pixel, width, height);

		if (ImGui::Begin("test texture")) {
			ImGui::Text("test texture windoajsdf");

			ImGui::Image(reinterpret_cast<void*>(texture), {width*10, height*10});
		}
		ImGui::End();
	}
#endif

	return nullptr;
}

