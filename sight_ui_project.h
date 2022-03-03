//
// Created by Aincvy(aincvy@gmail.com) on 2021/12/19.
//
#pragma once

#include "sight_ui.h"
#include "sight_project.h"
#include <string_view>

namespace sight {

    /**
     * @brief 
     * 
     * @param entity 
     * @param createEntityData 
     */
    bool convert(SightEntity& entity, const UICreateEntity& createEntityData);

    /**
     *
     * @param createEntityData
     * @return 
     */
    int addEntity(const UICreateEntity& createEntityData);

    int updateEntity(const UICreateEntity& createEntityData);

    void uiEditEntity(SightEntity const& info);

    void uiDeleteEntity(std::string_view fullName);

    bool checkTemplateNodeIsUsed(Project* p, std::string_view templateAddress, bool alert = true);
    
}