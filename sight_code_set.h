//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
#pragma once

#include <string>
#include <vector>
#include <map>
#include <string_view>

#include "sight.h"


#include "yaml-cpp/yaml.h"


namespace sight {

    struct SightCodeField;

    /**
     * type 
     */
    struct SightCodeType {
    public:
        SightCodeType(int id);
        SightCodeType(std::string_view name);

        const char* getNameCstr() const;
        std::string_view getName() const;

        int getId() const;

        /**
         * set name from id
         */
        void checkName();

        bool isVoid() const;

    private: 
        // type id  find from project
        int id = TypeUnSet;    
        // type final name. 
        std::string name;
    };

    /**
     * field or simple param
     */
    struct SightCodeFieldBase {
        // field name 
        std::string name;
        // 
        std::string defaultValue;

    };

    /**
     * 
     */
    struct SightCodeAnnotation{
        SightCodeType type;
        std::vector<SightCodeFieldBase> args;
    };

    struct SightCodeField: SightCodeFieldBase {
        // field type 
        SightCodeType type;

        std::vector<SightCodeAnnotation> annotations;
    };

    struct SightCodeFunction {
        // return type 
        SightCodeType rType;
        std::string name;
        std::vector<SightCodeField> params;
        // basically js code 
        std::string body;   
    };


    /**
     * class 
     */
    struct SightCodeClass {
    public:
        SightCodeClass(std::string_view name);
        ~SightCodeClass();

        std::string_view getName() const;

        std::map<std::string, SightCodeField> const& getFieldMap() const;
        std::map<std::string, SightCodeFunction> const& getFunctionMap() const;

        void reset();

    private:
        // class name
        std::string name; 

        // key: field name, value: info 
        std::map<std::string, SightCodeField> fieldMap;
        // key: function name, value: info 
        std::map<std::string, SightCodeFunction> functionMap;
    };

    /**
     *
     */
    struct SightCodeSet {
        // class map 
        std::map<std::string, SightCodeClass> clzMap;

        void reset();
    };

    /**
     * A writer.  Convert code set to target language.
     */
    struct SightCodeSetWriter {

    };

    /**
     * settings 
     */
    struct SightCodeSetSettings {
        std::string entityTemplateFuncName;
        std::string entityClzOutputPath;
    };

    YAML::Emitter& operator<< (YAML::Emitter& out, const SightCodeSetSettings& settings);
    bool operator>>(YAML::Node const&node, SightCodeSetSettings& settings);

    SightCodeSet* currentCodeSet();

    void initCodeSet();

    void destroyCodeSet();

}