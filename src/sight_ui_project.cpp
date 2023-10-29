#include "sight_ui_project.h"
#include "sight.h"
#include "sight_project.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>

#include "IconsMaterialDesign.h"
#include "sight_ui.h"

#include "imgui.h"
#include "sight_widgets.h"

namespace sight {

    namespace {
    }

    bool convert(SightEntity& entity, const UICreateEntity& createEntityData) {
        entity.name = createEntityData.name;
        entity.templateAddress = createEntityData.templateAddress;
        entity.parentEntity = createEntityData.parentEntity;

        for( const auto& item: createEntityData.fields){
            auto c = &item;
            entity.fields.emplace_back(c->name, c->type, c->defaultValue);
            auto& f = entity.fields.back();
            f.options = item.fieldOptions;
        }

        if (!createEntityData.useRawTemplateAddress) {
            entity.fixTemplateAddress();
        }

        // check parent
        if (!entity.parentEntity.empty()) {
            auto& fields = entity.fields;
            auto project = currentProject();

            auto parent = project->getSightEntity(entity.parentEntity);
            if (parent == nullptr) {
                return false;
            }

            // add parent's fields
            while (parent) {
                for (const auto& item : parent->fields) {
                    auto find = std::find_if(fields.begin(), fields.end(), [&item](SightEntityField const& f) {
                        return f.name == item.name;
                    });
                    if (find == fields.end()) {
                        // do not have
                        fields.push_back(item);
                    }
                }

                if (parent->parentEntity.empty()) {
                    break;
                }
                parent = project->getSightEntity(parent->parentEntity);
            }

        }

        return true;
    }

    int addEntity(const UICreateEntity& createEntityData) {
        SightEntity entity;
        bool f = convert(entity, createEntityData);
        if (!f) {
            return CODE_CONVERT_ENTITY_FAILED;
        }

        auto p = currentProject();
        return p->addEntity(entity) ? CODE_OK : CODE_FAIL;
    }

    int updateEntity(const UICreateEntity& createEntityData) {
        if (!createEntityData.isEditMode()) {
            return CODE_FAIL;
        }

        SightEntity entity;
        if (!convert(entity, createEntityData)) {
            return CODE_CONVERT_ENTITY_FAILED;
        }

        auto p = currentProject();
        return p->updateEntity(entity, createEntityData.editingEntity) ? CODE_OK : CODE_FAIL;
    }

    void uiDeleteEntity(std::string_view delFullName) {
        auto p = currentProject();
        // check if used.
        if (!checkTemplateNodeIsUsed(p, delFullName)) {
            // delete ask
            std::string str{ "! You only can delete the NOT-USED entity !\nYou will delete: " };
            str += delFullName;
            openAskModal("Delete entity", str.c_str(), [p, delFullName](bool f) {
                logDebug(f);
                if (f) {
                    p->delEntity(delFullName);
                    currentUIStatus()->toastController.info().toast("Delete success!", delFullName);
                }
            });
        } 
    }


    bool checkTemplateNodeIsUsed(Project* p, std::string_view templateAddress, bool alert) {
        std::string pathOut;
        if (p->isAnyGraphHasTemplate(templateAddress, &pathOut)) {
            // used, cannot be deleted.
            if (alert) {
                std::string contentString{ "Used in graph: " };
                contentString += pathOut;
                // if (out) {
                //     // question
                //     contentString += "\nAre you still want to do that?";
                //     openAskModal(ICON_MD_QUESTION_MARK, contentString, [](bool f){});
                // } else {
                // }
                openAlertModal(ICON_MD_WARNING " Operation denied", contentString);
            }
            return true;
        }

        return false;
    }

    void showCodeSetSettingsWindow() {
        auto uiStatus = currentUIStatus();
        auto project = currentProject();
        if(ImGui::Begin("CodeSetSettings", &(uiStatus->windowStatus.codeSetSettingsWindow))){
            auto& settings = project->getSightCodeSetSettings();

            ImGui::Text("EntityTemplate: ");
            ImGui::SameLine();
            showEntityOperationList(uiStatus);

            ImGui::Dummy(ImVec2(0,10));
            ImGui::Text("Output path");
            ImGui::Separator();
            ImGui::Text("Entity: ");
            helpMarker("entity class output path.");
            ImGui::SameLine();
            static char entityClassOutputPath[FILENAME_BUF_SIZE] = {0};
            if(strlen(entityClassOutputPath) == 0 && !settings.entityClzOutputPath.empty()) {
                sprintf(entityClassOutputPath, "%s", settings.entityClzOutputPath.c_str());
            }
            if(ImGui::InputText("##entityClassOutputPath", entityClassOutputPath, std::size(entityClassOutputPath))){
                settings.entityClzOutputPath = entityClassOutputPath;    
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_FOLDER_OPEN "##open_entity_class_output_path"))
            {
                // select folder
                auto baseDir = strlen(entityClassOutputPath) <= 0 ? "./" : entityClassOutputPath;
                int folderOpenStatus = 0;
                std::string path = openFolderDialog(baseDir, &folderOpenStatus);
                if(folderOpenStatus == CODE_OK) {
                    // 
                    sprintf(entityClassOutputPath, "%s", path.c_str());
                    settings.entityClzOutputPath = path;
                }
            }

        }
        ImGui::End();
    }

    /**
     * Most copy from sight_ui.cpp showEntityOperations()
     */
    void showEntityOperationList(UIStatus* uiStatus){
        auto& entityOperations = uiStatus->entityOperations;
        checkEntityOperations(entityOperations);
        const auto& operationNames = entityOperations.names;

        if(!operationNames.empty()) {
            const char* comboLabel = "##Operations";
            auto selectedOperationName = operationNames[entityOperations.selected].c_str();

            if (ImGui::BeginCombo(comboLabel, selectedOperationName)) {
                int tmpIndex = -1;
                for (const auto& item : operationNames) {
                    bool isSelected = (++tmpIndex) == entityOperations.selected;
                    if (ImGui::Selectable(item.c_str(), isSelected)) {
                        entityOperations.selected = tmpIndex;
                        
                        auto p = currentProject();
                        p->getSightCodeSetSettings().entityTemplateFuncName = item;
                        p->saveConfigFile();
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            helpMarker(entityOperations.map[selectedOperationName].description.c_str());
        }
    }

}