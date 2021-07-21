//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
// Include node editor ui part and logic part.

#pragma once

#include "imgui.h"


namespace sight {

    /**
     * init
     * @return
     */
    int initNodeEditor();

    /**
     * destroy and free
     * @return
     *
     */
    int destroyNodeEditor();


    int testNodeEditor(const ImGuiIO& io);

}