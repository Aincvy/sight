#include "sight_keybindings.h"
#include "dbg.h"
#include "sight_util.h"
#include "sight_widgets.h"

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
                    dbg(keyName);
                    return false;
                }
                wrapper = pointer;

                if (keyName == "controlKey") {
                    // todo the symbol cannot be shown now
                    // I need change the default font.
                    // auto temp = str;
                    // lowerCase(temp);
                    // if (temp == "super" || temp == "cmd" || temp == "win") {
                    //     controlKey = "⌘";
                    // }  else if (temp == "alt") {
                    //     controlKey = "⌥";
                    // } else if (temp == "ctrl") {
                    //     controlKey = "^";
                    // } else if (temp == "shift") {
                    //     controlKey = "⇧";
                    // }
                    // else {
                    //     controlKey = str;
                    // }
                    controlKey = str;
                } else if (keyName == "prefixCommand") {
                    prefixCommand = controlKey + "+" + str;
                } else {
                    // common commands
                    if (prefixCommand.empty()) {
                        wrapper.setTips(absl::Substitute("$0+$1", controlKey, str));
                    } else {
                        wrapper.setTips(absl::Substitute("$0 $1+$2", prefixCommand, controlKey, str));
                        
                    }
                }
            }
            return true;
        };

        // if (func("controlKey", keybindingds->controlKey)) {
        //     return keybindingds;
        // }
        CHECK_RETURN("controlKey", keybindingds->controlKey);
        func("prefixCommand", keybindingds->prefixCommand);
        CHECK_RETURN("saveFile", keybindingds->saveFile);
        CHECK_RETURN("openGraph", keybindingds->openGraph);

        return keybindingds;
    }

}