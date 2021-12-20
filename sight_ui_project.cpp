#include "sight_ui_project.h"
#include "sight.h"

namespace sight {

    namespace {
        void convert(SightEntity& entity, const UICreateEntity& createEntityData) {
            entity.name = createEntityData.name;
            entity.templateAddress = createEntityData.templateAddress;

            auto c = createEntityData.first;
            while (c) {
                entity.fields.emplace_back(c->name, c->type, c->defaultValue);

                c = c->next;
            }

            entity.fixTemplateAddress();
        }
    }

    int addEntity(const UICreateEntity& createEntityData) {
        SightEntity entity;
        convert(entity, createEntityData);

        auto p = currentProject();
        return p->addEntity(entity) ? CODE_OK : CODE_FAIL;
    }

    int updateEntity(const UICreateEntity& createEntityData) {
        if (!createEntityData.isEditMode()) {
            return CODE_FAIL;
        }

        SightEntity entity;
        convert(entity, createEntityData);

        auto p = currentProject();
        return p->updateEntity(entity, createEntityData.editingEntity) ? CODE_OK : CODE_FAIL;
    }

}