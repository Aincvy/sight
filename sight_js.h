//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
// Js engine and something else.

#pragma once

namespace sight{

    void testJs(char *arg1);

    /**
     * init js engine (v8)
     *
     * @param arg1
     * @return
     */
    int initJsEngine(char *arg1);


    /**
     * destroy js engine
     * @return
     */
    int destroyJsEngine();


}