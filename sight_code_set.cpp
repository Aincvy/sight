#include "sight_code_set.h"
#include "sight.h"
#include <string>
#include <yaml-cpp/emittermanip.h>

namespace sight {

    static SightCodeSet* gCodeSet = nullptr;

    SightCodeType::SightCodeType(int id)
        : id(id)
    {
        
    }

    SightCodeType::SightCodeType(std::string_view name)
        : name(name)
    {
        
    }

    const char* SightCodeType::getNameCstr() const {
        return name.c_str();
    }

    std::string_view SightCodeType::getName() const {
        return this->name;
    }

    int SightCodeType::getId() const {
        return this->id;
    }

    void SightCodeType::checkName() {
        
    }

    bool SightCodeType::isVoid() const {
        return id == IntTypeVoid;
    }

    SightCodeClass::SightCodeClass(std::string_view name)
        : name(name)
    {
        
    }

    SightCodeClass::~SightCodeClass()
    {
        reset();
    }

    std::string_view SightCodeClass::getName() const {
        return this->name;
    }

    std::map<std::string, SightCodeField> const& SightCodeClass::getFieldMap() const {
        return this->fieldMap;
    }

    std::map<std::string, SightCodeFunction> const& SightCodeClass::getFunctionMap() const {
        return this->functionMap;
    }

    void SightCodeClass::reset() {
        fieldMap.clear();
        functionMap.clear();
    }

    YAML::Emitter& operator<< (YAML::Emitter& out, const SightCodeSetSettings& settings) {
        out << YAML::BeginMap;
        out << YAML::Key << "entityTemplateFuncName" << YAML::Value << settings.entityTemplateFuncName;
        out << YAML::Key << "entityClzOutputPath" << YAML::Value << settings.entityClzOutputPath;
        out << YAML::EndMap;
        return out;
    }

    bool operator>>(YAML::Node const&node, SightCodeSetSettings& settings) {
        settings.entityTemplateFuncName = node["entityTemplateFuncName"].as<std::string>("");
        settings.entityClzOutputPath = node["entityClzOutputPath"].as<std::string>("");
        return true;
    }

    SightCodeSet* currentCodeSet() {
        return gCodeSet;
    }

    void initCodeSet() {
        gCodeSet = new SightCodeSet();
    }

    void destroyCodeSet() {
        delete gCodeSet;
        gCodeSet = nullptr;
    }

    void SightCodeSet::reset() {
        clzMap.clear();
    }
    


}