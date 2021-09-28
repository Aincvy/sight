#pragma once

#include "sight_widgets.h"

namespace sight {


    struct KeyBindings {
        // SightTwoKey save;
        // SightEmptyKey* save;

        /**
         * @brief Control Key, Maybe Ctrl or Cmd
         * 
         */
        SightKeyWrapper controlKey;

        SightKeyWrapper prefixCommand;

        /**
         * @brief Do NOT contains control key
         * 
         */
        SightKeyWrapper saveFile;

        SightKeyWrapper openGraph;


    };


    KeyBindings* loadKeyBindings(const char* path);
}