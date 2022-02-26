#include "sight_ui_project.h"
#include "sight.h"
#include "sight_project.h"
#include <algorithm>

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

}