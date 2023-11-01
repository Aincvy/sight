#include "sight.h"
#include "sight_log.h"
#include "sight_popup_modal.h"
#include "sight_util.h"
#include "sight_ui.h"

#include "imgui.h"
#include <string>

namespace sight {

    std::string_view AskModal::getLabel() const {
        return ModalLabels::modalAskData;
    }

    void AskModal::doCallback(bool result) {
        if (callbackInvoked) {
            return;
        }
        
        callbackInvoked = true;
        callback(result);
    }

    void AskModal::onDisable() {
        logDebug("AskModal::onDisable, title: $0", title);

        doCallback(false);
    }

    void AskModal::showContent() {
        ImGui::Text("%s", content.c_str());

        if (!this->inputFields.empty()) {
            ImGui::Separator();
            int index = -1;
            for (auto& inputField : this->inputFields) {
                index++;

                // same line: label, char* buf
                ImGui::Text("%s", inputField.label.c_str());
                ImGui::SameLine();
                std::string inputLabelBuf("##input-fields-");
                inputLabelBuf.append(std::to_string(index));
                ImGui::SetNextItemWidth(450);
                ImGui::InputText(inputLabelBuf.c_str(), inputField.buf, inputField.bufLength);
            }
        }

        ImGui::Dummy(ImVec2(0, 18));
        auto const& commonKeys = currentUIStatus()->languageKeys->commonKeys;
        if (buttonOK(commonKeys.ok)) {
            doCallback(true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(commonKeys.cancel)) {
            doCallback(false);
            ImGui::CloseCurrentPopup();
        }
    }

    std::string_view SaveModal::getLabel() const {
        return ModalLabels::modalSaveData;
    }

    void SaveModal::doCallback(SaveOperationResult result) {
        if (callbackInvoked) {
                return;
        }

        callbackInvoked = true;
        callback(result);
    }

    void SaveModal::onDisable() {
        doCallback(SaveOperationResult::Cancel);
    }

    void SaveModal::showContent() {
        ImGui::Text("%s", content.c_str());
        ImGui::Dummy(ImVec2(0, 18));

        auto const& commonKeys = currentUIStatus()->languageKeys->commonKeys;
        auto const& menuKeys = currentUIStatus()->languageKeys->menuKeys;
        if (buttonOK(menuKeys.save)) {
                doCallback(SaveOperationResult::Save);
                ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(commonKeys.cancel)) {
                doCallback(SaveOperationResult::Cancel);
                ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(commonKeys.drop)) {
                doCallback(SaveOperationResult::Drop);
                ImGui::CloseCurrentPopup();
        }
    }

    std::string_view AlertModal::getLabel() const {
        return ModalLabels::modalAlertData;
    }

    void AlertModal::onDisable() {    
    
    }

    void AlertModal::showContent() {
        ImGui::Text("%s", content.c_str());
        ImGui::Dummy(ImVec2(0, 18));

        auto const& commonKeys = currentUIStatus()->languageKeys->commonKeys;
        if (buttonOK(commonKeys.ok)) {
            ImGui::CloseCurrentPopup();
        }
    }

    void BaseModal::checkInit(std::string_view label) {
        if (this->init) {
            return;
        }
        
        this->init = true;
        if (!endsWith(title, label.data())) {
            title += label;
        }
        ImGui::OpenPopup(label.data());

        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    void BaseModal::show() {
        if (!enabled) {
            return;
        }

        auto label = getLabel();
        checkInit(label);

        if (ImGui::BeginPopupModal(label.data(), &enabled, ImGuiWindowFlags_AlwaysAutoResize)) {
            showContent();

            ImGui::EndPopup();
        } else {
            enabled = false;
        }

        if (!enabled) {
            // change to not enabled.

            onDisable();
        }
    }

    ModalInputField::ModalInputField(std::string_view label, char* buf, size_t bufLength)
        : label(label),
          buf(buf),
          bufLength(bufLength)
    {
    
    }

}