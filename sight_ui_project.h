//
// Created by Aincvy(aincvy@gmail.com) on 2021/12/19.
//
#pragma once

#include "sight_ui.h"
#include "sight_project.h"

namespace sight {

    /**
     * @brief 
     * 
     * @param entity 
     * @param createEntityData 
     */
    void convert(SightEntity& entity, const UICreateEntity& createEntityData);

    /**
     *
     * @param createEntityData
     * @return 
     */
    int addEntity(const UICreateEntity& createEntityData);

    int updateEntity(const UICreateEntity& createEntityData);
    
}