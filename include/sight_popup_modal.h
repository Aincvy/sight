
#pragma once

#include <string>
#include <functional>
#include <string_view>
#include <vector>

namespace sight {

    /**
     * @brief Imgui ids
     * 
     */
    struct ModalLabels {
        // static ids
        static constexpr const char* modalAskData = "###modalAskData";
        static constexpr const char* modalAsk1FieldData = "###modalAsk1FieldData";
        static constexpr const char* modalSaveData = "###modalSaveData";
        static constexpr const char* modalAlertData = "###modalAlertData";

        // dynamic ids, strcat on program load.
    };

    struct ModalInputField {
        std::string label;
        char* buf = nullptr;
        size_t bufLength = 0;

        ModalInputField(std::string_view label, char* buf, size_t bufLength);
    };

    struct BaseModal {
        std::string title;
        std::string content;

        bool enabled = true;     // default is true.
        bool init = false;
        bool callbackInvoked = false;

        BaseModal() = default;
        virtual ~BaseModal() = default;


        void checkInit(std::string_view label);

        void show();

        virtual std::string_view getLabel() const = 0;

    protected:
        virtual void showContent() = 0;

        /**
         * @brief from enabled to disabled.
         * 
         */
        virtual void onDisable() = 0;
    };

    struct AskModal : BaseModal {

        std::vector<ModalInputField> inputFields;
        
        std::function<void(bool)> callback;

        AskModal() = default;

        std::string_view getLabel() const override;

        void doCallback(bool result);

    protected:
        void showContent() override;

        void onDisable() override;
    };

    enum class SaveOperationResult {
        // save file
        Save,
        // do not do anything
        Cancel,
        // do not save file
        Drop,
    };

    // save
    struct SaveModal : BaseModal {
        std::function<void(SaveOperationResult)> callback;

        SaveModal() = default;


        std::string_view getLabel() const override;

        void doCallback(SaveOperationResult result);


    protected:
        void showContent() override;

        void onDisable() override;
    };


    // alert
    struct AlertModal : BaseModal {
        AlertModal() = default;

        std::string_view getLabel() const override;

    protected:
        void showContent() override;
        void onDisable() override;
    };
    
}
