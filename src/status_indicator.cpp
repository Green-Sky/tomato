#include "./status_indicator.hpp"

#include <solanaceae/contact/contact_store_i.hpp>

#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/components.hpp>

#include "./icon_generator.hpp"

#include <memory>

void StatusIndicator::updateState(State state) {
	if (_state == state) {
		return;
	}

	_state = state;

	std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surf{nullptr, &SDL_DestroySurface};

	switch (state) {
		case State::base:
			surf = {IconGenerator::base(), &SDL_DestroySurface};
			break;
		case State::unread:
			surf = {IconGenerator::colorIndicator(1.f, .5f, 0.f, 1.f), &SDL_DestroySurface};
			break;
		case State::none:
			break;
	}

	SDL_SetWindowIcon(_main_window, surf.get());

	if (_tray != nullptr) {
		_tray->setIcon(surf.get());
	}
}

StatusIndicator::StatusIndicator(
	RegistryMessageModelI& rmm,
	ContactStore4I& cs,
	SDL_Window* main_window,
	SystemTray* tray
) :
	_rmm(rmm),
	_cs(cs),
	_main_window(main_window),
	_tray(tray)
{
	// start off with base icon
	updateState(State::base);
}

void StatusIndicator::render(float delta) {
	_cooldown -= delta;
	if (_cooldown <= 0.f) {
		_cooldown = 1.f; // once a second

		bool has_unread = false;
		for (const auto& c : _cs.registry().view<Contact::Components::TagBig>()) {
			// maybe cache mm?
			if (const auto* mm = _rmm.get(c); mm != nullptr) {
				if (const auto* unread_storage = mm->storage<Message::Components::TagUnread>(); unread_storage != nullptr && !unread_storage->empty()) {
					has_unread = true;
					break;
				}
			}
		}

		if (has_unread) {
			updateState(State::unread);
		} else {
			updateState(State::base);
		}
	}
}

