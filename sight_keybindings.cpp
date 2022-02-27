#include "sight_keybindings.h"
#include "sight_log.h"
#include "sight_util.h"
#include "sight_widgets.h"
#include "IconsMaterialDesign.h"

#include "yaml-cpp/yaml.h"

#include "iostream"
#include <fstream>
#include <string>
#include <absl/strings/substitute.h>

#define CHECK_RETURN(keyName, var) \
    if (!func((keyName), (var))) {  \
        return keybindingds;       \
    }


namespace sight {

    KeyBindings* loadKeyBindings(const char* path) {
        KeyBindings* keybindingds = new KeyBindings();
        std::ifstream fin(path);
        if (!fin) {
            return keybindingds;
        }

        auto root = YAML::Load(fin);
        std::string controlKey{}, prefixCommand{};
        auto func = [&root,&controlKey,&prefixCommand](std::string const& keyName, SightKeyWrapper& wrapper) {
            SightEmptyKey* pointer = nullptr;
            auto temp = root[keyName];
            if (temp.IsDefined() && !temp.IsNull()) {
                std::string str = temp.as<std::string>();
                if (str.empty()) {
                    return true;
                }
                pointer = getFromKeyMap(str);
                if (!pointer) {
                    logDebug(keyName);
                    return false;
                }
                wrapper = pointer;

                if (keyName == "controlKey") {
                    // 
                    auto temp = str;
                    lowerCase(temp);
                    if (temp == "super" || temp == "cmd" || temp == "win") {
                        controlKey = ICON_MD_KEYBOARD_COMMAND_KEY;
                    }  else if (temp == "alt") {
                        controlKey = ICON_MD_KEYBOARD_OPTION_KEY;
                    } else if (temp == "ctrl") {
                        controlKey = ICON_MD_KEYBOARD_CONTROL_KEY;
                    } else if (temp == "shift") {
                        controlKey = "⇧";
                    }
                    else {
                        controlKey = str;
                    }
                    // controlKey = str;
                } else if (keyName == "prefixCommand") {
                    prefixCommand = controlKey + "+" + str;
                } else {
                    // common commands
                    auto keyTip = str;
                    upperCase(keyTip);
                    if (prefixCommand.empty()) {
                        wrapper.setTips(absl::Substitute("$0$1", controlKey, keyTip));
                    } else {
                        wrapper.setTips(absl::Substitute("$0 $1$2", prefixCommand, controlKey, keyTip));
                    }
                }
            }
            return true;
        };

        CHECK_RETURN("controlKey", keybindingds->controlKey);
        func("prefixCommand", keybindingds->prefixCommand);
        CHECK_RETURN("saveFile", keybindingds->saveFile);
        CHECK_RETURN("openGraph", keybindingds->openGraph);
        CHECK_RETURN("duplicateNode", keybindingds->duplicateNode);
        CHECK_RETURN("redo", keybindingds->redo);
        CHECK_RETURN("undo", keybindingds->undo);

        CHECK_RETURN("copy", keybindingds->copy);
        CHECK_RETURN("paste", keybindingds->paste);
        CHECK_RETURN("delete", keybindingds->_delete);

        keybindingds->esc = getFromKeyMap("esc");
        return keybindingds;
    }

}