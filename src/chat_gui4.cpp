#include "./chat_gui4.hpp"

#include "./chat_gui/layout_strategy.hpp"

#include <solanaceae/util/utils.hpp>

#include <solanaceae/contact/contact_store_i.hpp>
#include <solanaceae/contact/contact_model4.hpp>

#include <solanaceae/message3/components.hpp>
#include <solanaceae/tox_messages/msg_components.hpp>
#include <solanaceae/tox_messages/obj_components.hpp>
#include <solanaceae/object_store/meta_components_file.hpp>
#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/contact_components.hpp>

// HACK: remove them
#include <solanaceae/tox_contacts/components.hpp>

#include <entt/entity/entity.hpp>
#include <entt/entity/runtime_view.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_internal.h>

#include <SDL3/SDL.h>

#include "./chat_gui/contact_list.hpp"
#include "./chat_gui/contact_info.hpp"
#include "./chat_gui/contact_window.hpp"

#include <string>

ChatGui4::ChatGui4(
	ConfigModelI& conf,
	ObjectStore2& os,
	RegistryMessageModelI& rmm,
	ContactStore4Impl& cs,
	TextureUploaderI& tu,
	ContactTextureCache& contact_tc,
	MessageTextureCache& msg_tc,
	Theme& theme
) :
	_conf(conf),
	_os(os),
	_os_sr(_os.newSubRef(this)),
	_rmm(rmm),
	_cs(cs),
	_tu(tu),
	_contact_tc(contact_tc),
	_msg_tc(msg_tc),
	_b_tc(_bil, tu),
	_theme(theme),
	_fss(theme),
	_ivp(_msg_tc),
	_ciw(cs),
	_cls(cs)
{
	_os_sr.subscribe(ObjectStore_Event::object_update);
}

ChatGui4::~ChatGui4(void) {
}

float ChatGui4::render(float time_delta, bool window_hidden, bool window_focused) {
	_fss.render();
	_ivp.render(time_delta);
	_ciw.render();
	_b_tc.update();
	_b_tc.workLoadQueue();

	if (window_hidden) {
		// annoying, but all of the above needs to continue while not rendering
		return 1000.f;
	}

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
	TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

	const float viewport_width = viewport->WorkSize.x;
	selectLayoutStrategy(viewport_width / TEXT_BASE_WIDTH);

	constexpr auto bg_window_flags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	if (ImGui::Begin("tomato", nullptr, bg_window_flags)) {
		if (ImGui::BeginMenuBar()) {
			//ImGui::Separator();
			ImGui::Text("%.1fFPS", ImGui::GetIO().Framerate);
			ImGui::EndMenuBar();
		}

		if (static_cast<bool>(_next_contact)) {
			_contact_stack.push(std::make_unique<ContactWindow>(
				_cs, _rmm, _os,
				_theme, _contact_tc, _msg_tc, _b_tc,
				_ciw, _fss, _ivp, _cb, _tu,
				[this](ContactHandle4 new_c) {
					_next_contact = new_c;
				},
				_next_contact
			));
			_next_contact = {};
		}

		if (_layout_strategy) {
			_layout_strategy->render(*this, time_delta, window_focused);
		}
	}
	ImGui::End();

	return 1000.f; // TODO: higher min fps?
}

void ChatGui4::sendFilePath(std::string_view file_path) {
	if (!_contact_stack.empty()) {
		_contact_stack.top()->sendFilePath(file_path);
	}
}

void ChatGui4::sendFileList(const std::vector<std::string_view>& list) {
	if (!_contact_stack.empty()) {
		_contact_stack.top()->sendFileList(list);
	}
}

void ChatGui4::renderChatFilesTab(Contact4 c) {
	// TODO: we need a way to list objects by contact
	//_os._reg.view<ObjComp::F::SingleInfo>();

	// go over file messages instead
	auto* msg_reg_ptr = _rmm.get(c);
	if (msg_reg_ptr == nullptr) {
		return;
	}
	// TODO: copy/refactor message log view. or do we want something different?
	//msg_reg_ptr->view<Message::Components::MessageFileObject>();
	//auto o = reg.get<Message::Components::MessageFileObject>(e).o;
}

void ChatGui4::renderContactList(float width) {
	if (ImGui::BeginChild("contacts", {width, 0})) {
		_contact_list_sortable = !ImGui::IsWindowHovered();

		ContactHandle4 selected_contact{};
		if (!_contact_stack.empty()) {
			// meh, probably makes more sense for bottom
			selected_contact = _contact_stack.top()->c;
		}

		auto& cr = _cs.registry();

		if (::renderContactList(
			_cs,
			cr,
			_rmm,
			_theme,
			_contact_tc,
			contact_const_runtime_view{}.iterate(cr.storage<Contact::Components::ContactSortTag>()),
			_ciw,
			selected_contact
		)) {
			// cleanup
			while (!_contact_stack.empty()) {_contact_stack.pop();}

			_contact_stack.push(std::make_unique<ContactWindow>(
				_cs, _rmm, _os,
				_theme, _contact_tc, _msg_tc, _b_tc,
				_ciw, _fss, _ivp, _cb, _tu,
				[this](ContactHandle4 new_c) {
					_next_contact = new_c;
				},
				selected_contact
			));
		}
	}
	ImGui::EndChild();
}

bool ChatGui4::renderContactListContactSmall(const Contact4 c, const bool selected) const {
	std::string label;

	const auto& cr = _cs.registry();

	label += (cr.all_of<Contact::Components::Name>(c) ? cr.get<Contact::Components::Name>(c).name.c_str() : "<unk>");
	label += "###";
	label += std::to_string(entt::to_integral(c));

	return ImGui::Selectable(label.c_str(), selected);
}

bool ChatGui4::onEvent(const ObjectStore::Events::ObjectUpdate& e) {
	if (e.e.any_of<ObjComp::F::LocalHaveBitset, ObjComp::F::RemoteHaveBitset>()) {
		_b_tc.stale(e.e);
	}

	return false;
}

void ChatGui4::selectLayoutStrategy(float viewport_font_units) {
	// only recompute when actual width changes
	if (viewport_font_units == _last_viewport_width) {
		return;
	}
	_last_viewport_width = viewport_font_units;

	// TODO: maybe move out of here?
	if (viewport_font_units >= LAYOUT_THRESHOLD_MOBILE) {
		_layout_strategy = std::make_unique<DesktopLayout>();
	} else {
		_layout_strategy = std::make_unique<SinglePlaneLayout>();
	}
}

