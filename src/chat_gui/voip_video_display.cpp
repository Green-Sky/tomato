#include "./voip_video_display.hpp"

#include <solanaceae/contact/contact_store_i.hpp>

#include <vector>

VoIPVideoDisplay::VoIPVideoDisplay(
	ObjectStore2& os,
	ContactStore4I& cs,
	StreamManager& sm,
	TextureUploaderI& tu
) : _os(os), _cs(cs), _sm(sm), _tu(tu) {
}

VoIPVideoDisplay::~VoIPVideoDisplay(void) {
}

float VoIPVideoDisplay::render(float delta_time) {
	// go over all contacts with os comp

	{ // destory unrendered
		std::vector<Contact4> to_destory;
		_cs.registry()
			.view<Contact::Components::VoIPVideoTexture>()
			.each([this, &to_destory](Contact4 c, Contact::Components::VoIPVideoTexture& comp) {
				if (comp.frames_since_render >= _frames_till_destory) {
					// TODO: video sink

					_tu.destroy(comp.texture);
					to_destory.push_back(c);
					return;
				}
		});
		_cs.registry().erase<Contact::Components::VoIPVideoTexture>(to_destory.cbegin(), to_destory.cend());
	}

	// create/update rendered
	_cs.registry()
		.view<Contact::Components::VoIPVideoTexture>()
		.each([this](Contact4 c, Contact::Components::VoIPVideoTexture& comp) {
			// TODO: magic
			if (comp.texture == 0) {

				//comp.texture = _tu.upload(nullptr, w, h, format, TextureUploaderI::LINEAR, TextureUploaderI::STREAMING);
			}
	});

	// render popout windows

	return 1.f;
}
