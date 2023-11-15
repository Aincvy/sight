#include "sight.h"
#include "sight_defines.h"
#include "sight_node.h"
#include "sight_node_graph.h"
#include "sight_log.h"
#include "sight_project.h"
#include "sight_js.h"
#include "sight_event_bus.h"

#include <iostream>
#include <stdexcept>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <vector>

#include "crude_json.h"
#include "v8-isolate.h"

#include "absl/container/flat_hash_set.h"

#define VALUE_STR 	"value"


namespace sight {
    
    int SightNodeGraph::saveToFile(const char* path, bool set, bool saveAnyway, SaveReason saveReason) {
        if (set) {
            this->setFilePath(path);
        }
        if (!saveAnyway && isBroken()) {
            return CODE_FAIL;
        }

        // ctor yaml object
        YAML::Emitter out;
        out << YAML::BeginMap;
        // file header
        out << YAML::Key << whoAmI << YAML::Value << "sight-graph";
        // nodes
        out << YAML::Key << "nodes";
        out << YAML::Value << YAML::BeginMap;

        loopOf([&out](SightNode* node) {
            out << YAML::Key << node->nodeId;
            out << YAML::Value << *node;
        });
        out << YAML::EndMap;

        // connections
        out << YAML::Key << "connections";
        out << YAML::Value << YAML::BeginMap;

        loopOf([&out](SightNodeConnection* connection) {
            out << YAML::Key << connection->connectionId << YAML::Value << *connection;
        });
        out << YAML::EndMap;

        // other info
        // settings
        out << YAML::Key << "settings" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "lang.type" << YAML::Value << settings.language.type;
        out << YAML::Key << "lang.version" << YAML::Value << settings.language.version;
        out << YAML::Key << "outputFilePath" << YAML::Value << settings.outputFilePath;
        out << YAML::Key << "codeTemplate" << YAML::Value << settings.codeTemplate;
        out << YAML::Key << "graphName" << YAML::Value << settings.graphName;
        out << YAML::Key << "connectionCodeTemplate" << YAML::Value << settings.connectionCodeTemplate;
        out << YAML::Key << "enterNode" << YAML::Value << settings.enterNode;
        out << YAML::EndMap;

        // this->saveAsJsonHistory
        out << YAML::Key << "saveAsJsonHistory" << YAML::Value << this->saveAsJsonHistory;
        
        out << YAML::EndMap;     // end of 1st begin map

        // save file
        std::ofstream outToFile(path, std::ios::out | std::ios::trunc);
        outToFile << out.c_str() << std::endl;
        outToFile.close();

        if (saveReason == SaveReason::User) {
            logDebug("save over to: $0", path);
        }
        return CODE_OK;
    }

    int SightNodeGraph::load(std::string_view path) {
        this->filepath = path;

        std::ifstream fin(path.data());
        if (!fin.is_open()) {
            return CODE_FILE_ERROR;
        }

        int status = CODE_OK;
        try {
            auto root = YAML::Load(fin);

            // nodes
            auto nodeRoot = root["nodes"];
            auto connectionRoot = root["connections"];
            logDebug(nodeRoot.size());

            int loadNodeStatus = CODE_OK;
            SightNode* sightNode = nullptr;
            for (const auto& nodeKeyPair : nodeRoot) {
                auto yamlNode = nodeKeyPair.second;
                loadNodeStatus = loadNodeData(yamlNode, sightNode, this);
                if (loadNodeStatus != CODE_OK) {
                    if (status == CODE_OK) {
                        status = loadNodeStatus;
                    }

                    broken = true;
                    return CODE_FAIL;
                }

                // registerNodeIds(sightNode);
                registerNode(sightNode);
            }

            // connections
            for (const auto& item : connectionRoot) {
                SightNodeConnection tmpConnection;
                tmpConnection.graph = this;

                auto connectionId = item.first.as<int>();
                item.second >> tmpConnection;
                int ret = createConnection(tmpConnection.left, tmpConnection.right, connectionId, tmpConnection.priority);
                if (ret < 0) {
                    broken = true;
                    return CODE_FAIL;
                }
                auto con = findConnection(connectionId);
                con->generateCode = tmpConnection.generateCode;

                // tmpConnection will auto free when current loop ends, so let new connection have componentContainer
                con->componentContainer = tmpConnection.componentContainer;
            }

            // settings
            auto settingsNode = root["settings"];
            if (settingsNode.IsDefined()) {
                settings.language.type = settingsNode["lang.type"].as<int>();
                settings.language.version = settingsNode["lang.version"].as<int>();
                settings.outputFilePath = settingsNode["outputFilePath"].as<std::string>();
                if (settingsNode["codeTemplate"]) {
                    settings.codeTemplate = settingsNode["codeTemplate"].as<std::string>();
                }
                if (settingsNode["graphName"]) {
                    snprintf(settings.graphName, std::size(settings.graphName) - 1, "%s", settingsNode["graphName"].as<std::string>().c_str());
                }
                if (settingsNode["connectionCodeTemplate"]) {
                    settings.connectionCodeTemplate = settingsNode["connectionCodeTemplate"].as<std::string>();
                }
                if (settingsNode["enterNode"]) {
                    settings.enterNode = settingsNode["enterNode"].as<int>();
                }
            }

            auto saveAsJsonHistoryNode = root["saveAsJsonHistory"];
            if (saveAsJsonHistoryNode.IsDefined()) {
                this->saveAsJsonHistory = saveAsJsonHistoryNode.as<std::vector<std::string>>();
            }

            // set graph ref
            // loop of this->nodes
            for (auto& node : this->nodes) {
                node.graph = this;

                if (node.componentContainer) {
                    // loop of container
                    for (auto& component : node.componentContainer->components) {
                        component->graph = this;
                    }
                }
            }

            // loop of this->connections
            for (auto& connection : this->connections) {
                connection.graph = this;

                if (connection.componentContainer) {
                    // loop of container
                    for (auto& component : connection.componentContainer->components) {
                        component->graph = this;
                    }
                }
            }

            this->editing = false;
            logDebug("load ok");
            return status;
        } catch (const YAML::BadConversion& e) {
            logError("read file %s error!", path.data());
            logError("%s", e.what() );

            this->reset();
            return CODE_FILE_ERROR;     // bad file
        }

        // return 0;
    }

    void SightNodeGraph::dispose() {
        SimpleEventBus::graphDisposed()->dispatch(*this);
    }

    const char* SightNodeGraph::getFilePath() const {
        return this->filepath.c_str();
    }

    std::string SightNodeGraph::getFileName(std::string_view replaceExt) const {
        if (filepath.empty()) {
            return {};
        }

        return std::filesystem::path(this->filepath).filename().replace_extension(replaceExt).generic_string();
    }

    void SightNodeGraph::setFilePath(const char* path) {
        this->filepath = path;
    }

    void SightNodeGraph::registerNodeIds(SightNode* p) {

        idMap[p->nodeId] = {
            SightAnyThingType::Node,
            p
        };

        auto nodeFunc = [p, this](std::vector<SightNodePort>& list) {
            for (auto& item : list) {
                item.node = p;
                idMap[item.id] = {
                    SightAnyThingType::Port,
                    p,
                    item.id
                };
            }
        };
        CALL_NODE_FUNC(p);
    }

    void SightNodeGraph::unregisterNodeIds(SightNode const* p) {

        auto nodeFunc = [this](std::vector<SightNodePort> const& list) {
            for (auto& item : list) {
                idMap.erase(item.id);
            }
        };

        idMap.erase(p->nodeId);
        CALL_NODE_FUNC(p);
    }



    SightNode* SightNodeGraph::findNode(uint id) {
        return findSightAnyThing(id).asNode();
    }

    SightNode* SightNodeGraph::findEnterNode(int* status) {
        if (settings.enterNode <= 0) {
            SET_INT_VALUE(status, CODE_GRAPH_NO_ENTER_NODE);
        } else {
            auto n = findNode(settings.enterNode);
            if (n) {
                SET_INT_VALUE(status, CODE_OK);
                return n;
            }
            SET_INT_VALUE(status, CODE_GRAPH_ERROR_ENTER_NODE);
        }
        return nullptr;
    }

    SightNodeConnection* SightNodeGraph::findConnection(int id) {
        return findSightAnyThing(id).asConnection();
    }



    int SightNodeGraph::createConnection(uint leftPortId, uint rightPortId, uint connectionId, int priority) {
        if (leftPortId == rightPortId) {
            return -2;
        }

        auto left = findPort(leftPortId);
        auto right = findPort(rightPortId);
        if (!left || !right) {
            return -1;
        }
        if (left->kind == right->kind) {
            return -3;
        }

        // if (left->getType() == IntTypeProcess && left->isConnect()) {
        //     return -4;
        // }

        auto id = connectionId > 0 ? connectionId : nextNodeOrPortId();
        auto thing = findSightAnyThing(id);
        if (thing.type != SightAnyThingType::Invalid) {
            return -5;
        }

        if (left->node == right->node) {
            return -6;
        }

        // find color ;
        ImU32 color = IM_COL32_WHITE;
        auto [typeInfo, find] = currentProject()->findTypeInfo(left->getType());
        if (find && typeInfo.style) {
            color = typeInfo.style->color;
        }

        SightNodeConnection connection = {
            id,
            leftPortId,
            rightPortId,
            color,
            priority,
        };
        addConnection(connection, left, right);
        return id;
    }

    void SightNodeGraph::addConnection(const SightNodeConnection& connection, SightNodePort* left, SightNodePort* right) {
        auto p = this->connections.add(connection);
        p->graph = this;
        idMap[connection.connectionId] = {
            SightAnyThingType::Connection,
            p
        };

        if (!left) {
            left = findPort(connection.leftPortId());
        }
        if (!right) {
            right = findPort(connection.rightPortId());
        }
        assert(left);
        assert(right);

        left->connections.push_back(p);
        left->sortConnections();

        right->connections.push_back(p);
        right->sortConnections();
    }

    void SightNodeGraph::registerNode(SightNode* p) {
        p->graph = this;
        p->updateChainPortPointer();
        registerNodeIds(p);
        p->callEventOnInstantiate();
        SimpleEventBus::nodeAdded()->dispatch(p);

        // this->editing = true;
        markDirty();
    }

    const SightArray<SightNode>& SightNodeGraph::getNodes() const {
        return this->nodes;
    }

    SightNodeGraphExternalData& SightNodeGraph::getExternalData() {
        return externalData;
    }

    void SightNodeGraph::callNodeEventsAfterLoad(v8::Isolate* isolate) {
        // call event one by one.
        // auto isolate = currentUIStatus()->isolate;
        auto nodeFunc = [isolate](std::vector<SightNodePort>& list) {
            for (auto& item : list) {
                if (item.templateNodePort) {
                    auto templateNodePort = item.templateNodePort;
                    templateNodePort->onValueChange(isolate, &item);
                }
            }
        };

        for (auto& item : this->nodes) {
            auto p = &item;
            if (item.templateNode) {
                item.templateNode->onReload(isolate, p);
            }

            // port events
            // CALL_NODE_FUNC(p);
        }
    }

    void SightNodeGraph::sortConnections() {
        for (auto& item : this->nodes) {
            item.sortConnections();
        }
    }

    int SightNodeGraph::verifyId(bool errorStop, std::function<void(SightNode*)> onNodeError, std::function<void(SightNodePort*)> onPortError,
                                 std::function<void(SightNodeConnection*)> onConnectionError) {
        using set = absl::flat_hash_set<uint>;
        set nodeIdSet;
        set portIdSet;
        set connectionIdSet;

        int status = CODE_OK;
        const uint maxNodeOrPortId = currentProject()->maxNodeOrPortId();

        auto isIdInvalid = [&nodeIdSet, &portIdSet, &connectionIdSet, maxNodeOrPortId](uint id) {
            return nodeIdSet.contains(id) || portIdSet.contains(id) || connectionIdSet.contains(id) || id > maxNodeOrPortId || id < START_NODE_ID;
        };

        auto nodeFunc = [&onPortError, &isIdInvalid, &portIdSet, &status](std::vector<SightNodePort>& list) {
            for (auto& item : list) {
                auto id = item.id;
                if (isIdInvalid(id)) {
                    status = CODE_FAIL;
                    if (onPortError) {
                        onPortError(&item);
                    }
                }

                portIdSet.emplace(id);
            }
        };

        for (auto& item : nodes) {
            auto id = item.nodeId;
            if (isIdInvalid(id)) {
                status = CODE_FAIL;
                // call
                if (onNodeError) {
                    onNodeError(&item);
                }
                if (errorStop) {
                    break;
                }
            }
            nodeIdSet.emplace(id);

            // check port
            CALL_NODE_FUNC(&item);
            if (status != CODE_OK && errorStop) {
                // return status;
                break;
            }
        }

        if (status == CODE_OK || !errorStop) {
            // check connections
            for (auto& item : connections) {
                auto id = item.connectionId;
                if (isIdInvalid(id)) {
                    status = CODE_FAIL;
                    // call
                    if (onConnectionError) {
                        onConnectionError(&item);
                    }
                    if (errorStop) {
                        break;
                    }

                    auto l = item.findLeftPort();
                    auto r = item.findRightPort();
                    if (l && r) {
                        if (l->node == r->node) {
                            logWarning("connection id $0, two ports $1 and $2 are same node's ports.", id, l->id, r->id);
                        }
                    }
                }

                connectionIdSet.emplace(id);
            }
        }

        return status;
    }

    void SightNodeGraph::checkAndFixIdError() {
        bool rebuildIdMap = false;
        auto nodeFunc = [&rebuildIdMap, this](SightNode* node) {
            auto n = findNode(node->nodeId);
            if (n == node) {
                rebuildIdMap = true;
            }
            node->nodeId = nextNodeOrPortId();
        };
        auto portFunc = [&rebuildIdMap, this](SightNodePort* port) {
            auto p = findPort(port->id);
            if (p == port) {
                rebuildIdMap = true;
            }

            port->id = nextNodeOrPortId();
        };
        auto connectionFunc = [&rebuildIdMap, this](SightNodeConnection* connection) {
            auto c = findConnection(connection->connectionId);
            if (c == connection) {
                rebuildIdMap = true;
            }

            connection->connectionId = nextNodeOrPortId();
        };

        verifyId(false, nodeFunc, portFunc, connectionFunc);
        markDirty();

        if (rebuildIdMap) {
            this->rebuildIdMap();
        }
    }

    void SightNodeGraph::markDirty() {
        editing = true;
    }

    bool SightNodeGraph::isDirty() const {
        return editing;
    }

    void SightNodeGraph::markBroken(bool broken, std::string_view str) {
        brokenReason = std::string(str);
        this->broken = broken;
    }

    const SightArray<SightNodeConnection>& SightNodeGraph::getConnections() const {
        return this->connections;
    }




    void SightNodeGraph::loopOf(std::function<void(SightNode const*)> func) const {

        // loop of this->nodes, and call func
        for (auto& item : this->nodes) {
            if (item.isDeleted() || item.isComponent()) {
                continue;
            }
            func(&item);
        }
    }

    void SightNodeGraph::loopOf(std::function<void(SightNode*)> func) {
        for (auto& item : this->nodes) {
            if (item.isDeleted() || item.isComponent()) {
                continue;
            }
            func(&item);
        }
    }

    void SightNodeGraph::loopOf(std::function<void(SightNodeConnection*)> func) {

        // loop of this->connections, and call func
        for (auto& item : this->connections) {
            if (item.isDeleted()) {
                continue;
            }
            func(&item);
        }
    }

    void SightNodeGraph::loopOf(std::function<void(SightNodeConnection const*)> func) const {

        for (auto& item : this->connections) {
            if (item.isDeleted()) {
                continue;
            }
            func(&item);
        }
    }

    // SightArray<SightNode> & SightNodeGraph::getComponents() {
    //     return components;
    // }

    SightNodePort* SightNodeGraph::findPort(uint id) {
        return findSightAnyThing(id).asPort();
    }

    SightNodePortHandle SightNodeGraph::findPortHandle(uint id) {
        auto t = findSightAnyThing(id);
        if (t.data.portHandle) {
            return t.data.portHandle;
        }
        return {};
    }

    bool SightNodeGraph::isNode(uint id) const {
        auto t = findSightAnyThing(id);
        return t.type == SightAnyThingType::Node;
    }

    bool SightNodeGraph::isConnection(uint id) const {
        auto t = findSightAnyThing(id);
        return t.type == SightAnyThingType::Connection;
    }

    bool SightNodeGraph::isPort(uint id) const {
        auto t = findSightAnyThing(id);
        return t.type == SightAnyThingType::Port;
    }

    const SightAnyThingWrapper& SightNodeGraph::findSightAnyThing(uint id) const {
        if (id <= 0) {
            return invalidAnyThingWrapper;
        }

        auto iter = idMap.find(id);
        if (iter == idMap.end()) {
            return invalidAnyThingWrapper;
        }
        return iter->second;
    }

    int SightNodeGraph::delNode(int id) {
        auto result = std::find_if(nodes.begin(), nodes.end(), [id](const SightNode& n) {
            return n.nodeId == id;
        });
        if (result == nodes.end()) {
            return CODE_FAIL;
        }

        if (result->hasConnections()) {
            return CODE_NODE_HAS_CONNECTIONS;
        }

        unregisterNodeIds(&(*result));
        nodes.erase(result);
        markDirty();
        return CODE_OK;
    }

    int SightNodeGraph::delNode(SightNode const* p) {
        return this->delNode(p->getNodeId());
    }

    int SightNodeGraph::delConnection(int id, bool removeRefs) {
        auto result = std::find_if(connections.begin(), connections.end(),
                                   [id](const SightNodeConnection& c) {
                                       return c.connectionId == id;
                                   });
        if (result == connections.end()) {
            return CODE_FAIL;
        }

        // deal refs.
        if (removeRefs) {
            result->removeRefs();
        }
        idMap.erase(id);

        connections.erase(result);
        markDirty();
        return CODE_OK;
    }

    int SightNodeGraph::save(SaveReason saveReason) {
        if (!editing) {
            return CODE_OK;
        }

        auto c = saveToFile(this->filepath.c_str(), false, false, saveReason);
        if (c == CODE_OK) {
            this->editing = false;
        }
        return c;
    }

    SightNodeGraph::SightNodeGraph() {
    }

    SightNodeGraph::~SightNodeGraph() {
        this->dispose();
    }

    void SightNodeGraph::reset() {
        this->nodes.clear();
        this->connections.clear();
        this->idMap.clear();
    }

    void SightNodeGraph::rebuildIdMap() {
        idMap.clear();

        auto nodeFunc = [this](std::vector<SightNodePort>& list, SightNode* n) {
            for (auto& item : list) {
                this->idMap[item.getId()] = {
                    SightAnyThingType::Port,
                    n,
                    item.getId(),
                };
            }
        };

        for (auto& item : this->nodes) {
            if (item.isDeleted()) {
                continue;
            }
            idMap[item.getNodeId()] = {
                SightAnyThingType::Node,
                &item
            };

            auto p = &item;
            CALL_NODE_FUNC(p, p);
        }
        for (auto& item : this->connections) {
            if (item.isDeleted()) {
                continue;
            }

            auto c = &item;
            idMap[item.connectionId] = {
                SightAnyThingType::Connection,
                c
            };
        }
    }

    SightArray<SightNode>& SightNodeGraph::getNodes() {
        return this->nodes;
    }

    bool SightNodeGraph::isBroken() const {
        return this->broken;
    }

    std::string const& SightNodeGraph::getBrokenReason() const {
        return brokenReason;
    }

    SightNodeGraphSettings& SightNodeGraph::getSettings() {
        return this->settings;
    }

    void SightNodeGraph::asyncParse() const {
        addJsCommand(JsCommandType::ParseGraph, CommandArgs::copyFrom(filepath));
        logInfo("use js-thread to start parsing.");
    }

    void SightNodeGraph::setName(std::string_view name) {
        snprintf(settings.graphName, std::size(settings.graphName) - 1, "%s", name.data());
    }

    std::string_view SightNodeGraph::getName() const {
        return settings.graphName;
    }


    void SightNodeGraph::addPortId(SightNodePort const& port) {
        if (idMap.find(port.getId()) != idMap.end()) {
            logError("port id already exists: $0", port.getId());
            return;
        }
        idMap[port.getId()] = {
            SightAnyThingType::Port,
            port.node,
            port.getId()
        };
    }

    void SightNodeGraph::addNodeId(SightNode* node) {

        if (idMap.find(node->getNodeId()) != idMap.end()) {
            logError("node id already exists: $0", node->getNodeId());
            return;
        }

        idMap[node->getNodeId()] = {
            SightAnyThingType::Node,
            node
        };
    }

    int SightNodeGraph::outputJson(std::string_view path, bool overwrite, SightNodeGraphOutputJsonConfig const& config) const {
        logDebug("try output json to $0", path);
        using namespace v8;

        int status = CODE_OK;
        auto setValue = [&status](crude_json::value& tmp, const SightNodeValue& value) {
            switch (value.getType()) {
            case IntTypeString:
                tmp[VALUE_STR] = value.getString();
                break;
            case IntTypeLargeString:
                tmp[VALUE_STR] = value.getLargeStringCopy();
                break;
            case IntTypeFloat:
                tmp[VALUE_STR] = value.u.f;
                break;
            case IntTypeInt:
                tmp[VALUE_STR] = value.u.i * 1.0;
                break;
            case IntTypeDouble:
            case IntTypeLong:
                tmp[VALUE_STR] = value.u.d;
                break;
            case IntTypeBool:
                tmp[VALUE_STR] = value.u.b;
                break;
            case IntTypeProcess:
                logWarning("not support process type, will fill use an empty array.");
                tmp[VALUE_STR] = crude_json::value(crude_json::type_t::array);
                break;
            default:
            {
                auto [typeInfo, isFind] = currentProject()->findTypeInfo(value.getType());
                if (isFind) {
                    if (!typeInfo.writeToJson(value, tmp, VALUE_STR)) {
                        logError("failed to write to json: $0", typeInfo.getSimpleName());
                        status = CODE_FAIL;
                    }
                } else {
                    logError("unknown type: $0", value.getType());
                    status = CODE_FAIL;
                }
                break;
            }
                
            }
        };

        auto doComponents = [](crude_json::value& parent, SightComponentContainer* componentContainer) {
            // logDebug("doComponents, component valid: $0", componentContainer != nullptr);
            if (!componentContainer) {
                return;
            }

            auto isolate = getJsIsolate();
            for (auto& item : componentContainer->components) {
                const auto& component = item->templateNode->component;
                if (!component.appendDataToOutput) {
                    continue;
                }

                auto result = component.appendDataToOutput.callByReadonly(isolate, item);
                if (!result.IsEmpty()) {
                    // v8::JSON::Stringify(Local<Context> context, Local<Value> json_object)
                    std::string str = v8pp::from_v8<std::string>(isolate, result.ToLocalChecked());
                    // logDebug("try append string: $0", str);

                    // convert and merge
                    auto fromComponent = crude_json::value::parse(str);
                    if (!fromComponent.is_object()) {
                        logError("append data failed, component not return object json string: $0", item->nodeId);
                        continue;
                    }

                    // loop of
                    for (const auto& item : fromComponent.underlyingObject()) {
                        if (parent.contains(item.first)) {
                            logError("append data failed, component already exists: $0", item.first);
                            continue;
                        }

                        parent[item.first] = item.second;
                    }
                }
            }
        };

        // check path has file ?
        if (std::filesystem::exists(path)) {
            if (!overwrite) {
                return CODE_FILE_EXISTS;
            }

            // delete
            // std::filesystem::remove(path);
        }

        // generate json
        crude_json::value root;
        crude_json::value nodes;
        crude_json::value connections;

        auto nodeFunc = [&config, &setValue](std::vector<SightNodePort> const& list, crude_json::value& root) {
            // logDebug("list size: $0", list.size());
            for (auto& item : list) {
                if (item.portName.empty()) {
                    // title input/output port
                    continue;
                }

                crude_json::value tmp;
                tmp["name"] = changeStringToCase(item.portName, config.fieldNameCaseType);
                tmp["id"] = item.id * 1.0;
                if (item.templateNodePort) {
                    setValue(tmp, item.value);
                }

                root.push_back(tmp);
            }
        };

        auto dataNodeFunc = [&config, &setValue](std::vector<SightNodePort> const& list, crude_json::value& root) {
            for (auto& item : list) {
                if (item.portName.empty()) {
                    // title input/output port
                    continue;
                }

                crude_json::value tmp;
                if (item.getType() == IntTypeProcess) {
                    crude_json::value connectionArray(crude_json::type_t::array);
                    for (auto& connection : item.connections) {
                        connectionArray.push_back(crude_json::value((double)connection->connectionId));
                    }
                    tmp[VALUE_STR] 	= connectionArray;
                } else {
                    if (item.isConnect()) {
                        continue;
                    }

                    //  if this is connected, then it should using the port value.   todo
                    setValue(tmp, item.value);
                }

                auto key = changeStringToCase(item.portName, config.fieldNameCaseType);
                // contains key, then ignore
                auto const& map = root.underlyingObject();
                if (map.find(key) != map.end()) {
                    logDebug("key $0 repeat, so jump it." , key);
                    continue;
                }

                root[key] = tmp[VALUE_STR];
            }
        };

        // fill nodes data
        auto index = -1;
        auto nodeWork = [&](const SightNode* np) {
            index++;

            const auto& item = *np;
            crude_json::value node;
            node["id"] = item.nodeId * 1.0;
            node["name"] = item.nodeName;
            if (item.templateNode) {
                // templateName
                node["templateName"] = item.templateNode->nodeName;
            }

            //
            crude_json::value members{ crude_json::type_t::array };
            CALL_NODE_FUNC(np, members);
            node["members"] = members;

            if (config.exportData) {
                crude_json::value data{ crude_json::type_t::object };
                dataNodeFunc(item.fields, data);
                dataNodeFunc(item.inputPorts, data);
                dataNodeFunc(item.outputPorts, data);

                node["data"] = data;
            }

            nodes[index] = node;
        };
        
        this->loopOf(nodeWork);

        
        // fill connections data
        index = -1;
        auto connWork = [&](const SightNodeConnection* c) {
            index++;

            const auto& item = *c;

            crude_json::value connection;

            connection["id"] = item.connectionId * 1.0;
            connection["left"] = item.left * 1.0;
            connection["right"] = item.right * 1.0;

            if (config.includeNodeIdOnConnectionData) {
                auto port = item.findLeftPort();
                if (port) {    
                    connection["leftNode"] = port->getNodeId() * 1.0;
                }

                port = item.findRightPort();
                if (port) {     
                    connection["rightNode"] = port->getNodeId() * 1.0;
                }
            }

            doComponents(connection, item.componentContainer);
            connections[index] = connection;
        };

        this->loopOf(connWork);

        if (status == CODE_OK) {
            // get file output stream
            std::ofstream out;
            out.open(path.data(), std::ios::trunc);

            if (!out.is_open()) {
                return CODE_FILE_ERROR;
            }

            root[config.nodeRootName] = nodes;
            root[config.connectionRootName] = connections;

            out << root.dump(2);
            out.close();
        }
        
        logDebug("output json to $0 over: $1 ", path, status);
        return status;
    }

    SightComponentContainer* SightNodeGraph::createComponentContainer() {
        auto c = this->componentContainers.add();
        c->graph = this;
        return c;
    }

    bool SightNodeGraph::removeComponentContainer(SightComponentContainer* container) {
        return this->componentContainers.remove(container);
    }

    std::vector<std::string> const& SightNodeGraph::getSaveAsJsonHistory() const {

        return saveAsJsonHistory;
    }

    int SightNodeGraph::fakeDeleteNode(uint id) {

        auto n = findNode(id);
        if (n) {
            return fakeDeleteNode(n);
        }

        return CODE_GRAPH_INVALID_ID;
    }

    int SightNodeGraph::fakeDeleteNode(SightNode* node) {
        if (node->hasConnections()) {
            return CODE_NODE_HAS_CONNECTIONS;
        }

        
        node->markAsDeleted();
        SimpleEventBus::nodeRemoved()->dispatch(*node);
        return CODE_OK;
    }

    int SightNodeGraph::fakeDeleteConnection(SightNodeConnection* connection) {
        connection->removeRefs();
        connection->markAsDeleted();
        return CODE_OK;
    }

    SightComponentContainer* SightNodeGraph::getComponentContainer(uint anyThingId) {
        auto anyThing = this->findSightAnyThing(anyThingId);
        if (anyThing.type == SightAnyThingType::Node) {
            return anyThing.asNode()->getComponentContainer();
        } else if (anyThing.type == SightAnyThingType::Connection) {
            return anyThing.asConnection()->getComponentContainer();
        }
        return nullptr;
    }

    bool SightNodeGraph::replaceNode(uint fromNodeId, uint toNodeId) {

        auto from = findNode(fromNodeId);
        auto to = findNode(toNodeId);

        if (!from || !to) {
            logError("replaceNode failed: node not found. fromNode=$0, toNode=$1", fromNodeId, toNodeId);
            return false;
        }

        // copy connection
        auto connectionFunc = [this, fromNodeId](SightNodePort* fromPort, SightNodePort* toPort) {
            if (!fromPort || !toPort) {
                return;
            }

            if (!fromPort->isConnect()) {
                return;
            }
            if (fromPort->getType() != toPort->getType()) {
                return;
            }

            // loop fromPort->connections from back using rbegin
            auto& connections = fromPort->connections;
            if (connections.empty()) {
                return;
            }

            for (auto& c : connections){
                logDebug("try change connection: $0", c->connectionId);

                if (c->leftPortId() == fromPort->getId()) {
                    c->left = toPort->getId();     // this way will keep all connection info.
                    toPort->addConnection(c);

                } else if (c->rightPortId() == fromPort->getId()) {
                    c->right = toPort->getId();
                    toPort->addConnection(c);
                } else {
                    logError("connection $0 ids are not match. left=$0, right=$1", c->connectionId, c->leftPortId(), c->rightPortId());
                }
            }
            connections.clear();

        };

        connectionFunc(from->chainInPort, to->chainInPort);
        connectionFunc(from->chainOutPort, to->chainOutPort);

        absl::flat_hash_set<uint> usedIds;
        auto copyConnectionFunc = [&connectionFunc, &usedIds](std::vector<SightNodePort>& list, std::vector<SightNodePort>& searchList) {
            for (auto& p : list) {
                if (p.portName.empty()) {
                    continue;
                }

                auto tmp = getReplaceablePort(searchList, p, usedIds);
                if (tmp) {
                    connectionFunc(&p, tmp);
                    usedIds.insert(tmp->getId());
                }
            }
        };

        copyConnectionFunc(from->inputPorts, to->inputPorts);
        copyConnectionFunc(from->outputPorts, to->outputPorts);

        // clear from connections.
        auto nodeFunc = [this](std::vector<SightNodePort>& list) {
            for (auto& p : list) {
                // loop of p.connections
                p.clearLinks();
            }
        };
        CALL_NODE_FUNC(from);

        auto pos = to->position;
        to->position = from->position;
        from->position = pos;

        return true;
    }

    bool SightNodeGraph::insertNodeAtConnectionMid(uint nodeId, uint connectionId) {
        auto node = findNode(nodeId);
        if (!node) {
            return false;
        }
        auto connection = findConnection(connectionId);
        if (!connection) {
            return false;
        }

        auto connectionLeftPort = connection->findLeftPort();
        auto connectionRightPort = connection->findRightPort();


        // find port.
        absl::flat_hash_set<uint> ignoreIds;
        SightNodePort* leftTypeSafePort = getReplaceablePort(node->inputPorts, *connectionLeftPort, ignoreIds);
        if (!leftTypeSafePort) {
            logError("can not found a valid left port on node $0", nodeId);
            return false;
        }

        SightNodePort* rightTypeSafePort = getReplaceablePort(node->outputPorts, *connectionRightPort, ignoreIds);
        if (!rightTypeSafePort) {
            logError("can not found a valid right port on node $0", nodeId);
            return false;
        }
        auto oldLeftConnectionSize = leftTypeSafePort->connectionsSize();
        auto oldRightConnectionSize = rightTypeSafePort->connectionsSize();
        
        connectionRightPort->removeConnection(connectionId);
        connection->right = leftTypeSafePort->getId();
        leftTypeSafePort->addConnection(connection);

        createConnection(rightTypeSafePort->getId(), connectionRightPort->getId());

        if (leftTypeSafePort->connections.size() != oldLeftConnectionSize + 1) {
            logError("something error left");
        }
        if (rightTypeSafePort->connections.size() != oldRightConnectionSize + 1) {
            logError("something error right");
        }
        return true;
    }

    bool SightNodeGraph::detachNodeConnections(uint nodeId, bool onlyTitleBarPort) {
        auto node = findNode(nodeId);
        if (!node) {
            return false;
        }

        // if left title bar and right title bar is connect, they are same type, then delete right connection, change left connection's right to right connections's right
        //      if not same type, then just delete them
        
        // 0 = just delete, 1 = change right.
        int titleBarOprType = 0;

        // collect info
        if (node->chainInPort->getType() == node->chainOutPort->getType()) {
            if (node->chainInPort->connectionsSize() == 1
                  && node->chainInPort->connectionsSize() == node->chainOutPort->connectionsSize()) {
                titleBarOprType = 1;
            }
        }

        if (titleBarOprType == 0) {
            // just delete connections
            node->chainInPort->clearLinks();
            node->chainOutPort->clearLinks();
        } else if (titleBarOprType == 1) {
            auto lc = node->chainInPort->connections[0];
            auto rc = node->chainOutPort->connections[0];

            lc->changeRight(rc->rightPortId());

            // node->chainInPort->removeConnection(lc->connectionId);
            node->chainOutPort->clearLinks();
        }

        if (onlyTitleBarPort) {
            return true;
        }

        logError("onlyTitleBarPort == false, that not impl..") ;
        return false;
    }

    int SightNodeGraph::fakeDeleteConnection(uint id) {

            auto c = findConnection(id);
            if (c) {
                return fakeDeleteConnection(c);
            }

            return CODE_GRAPH_INVALID_ID;
        }

    // SightArray<SightComponentContainer> & SightNodeGraph::getComponentContainers() {
    //     return this->componentContainers;
    // }

    SightComponentContainer* SightNodeGraph::deepClone(SightComponentContainer* original) {
        auto target = this->componentContainers.add();
        target->setGraph(this);

        for (auto& item : original->components) {
            auto n = this->nodes.add();
            n->graph = this;
            n->copyFrom(item, SightNode::CopyFromType::Component, true);

            target->addComponent(n);
        }

        return target;
    }

    SightNode* SightNodeGraph::deepClone(SightNode* original) {
        // if (original->templateNode) {
        //     // use template node ?
        // }

        auto target = this->nodes.add();
        target->graph = this;
        target->copyFrom(original, SightNode::CopyFromType::Duplicate, true);
        return target;
    }

    void SightNodeGraph::addSaveAsJsonHistory(std::string_view path) {

        saveAsJsonHistory.insert(saveAsJsonHistory.begin(), std::string(path));     // 将新元素插入到0号位置

        if (saveAsJsonHistory.size() > 5) {
            saveAsJsonHistory.pop_back();     // 删除最后一个元素
        }

        markDirty();
    }

    SightNodePort* getReplaceablePort(std::vector<SightNodePort> &ports, SightNodePort const& targetPort, absl::flat_hash_set<uint> const& ignoreIds) {

        // name same also need type same
        SightNodePort* nameSamePort = nullptr;
        SightNodePort* typeSamePort = nullptr;
        for (auto& search : ports) {
            if (ignoreIds.contains(search.getId())) {
                continue;
            }
            if (search.type == targetPort.type) {
                if (search.portName == targetPort.portName) {
                    nameSamePort = &search;
                    break;
                }
                if (!typeSamePort) {
                    typeSamePort = &search;
                }
            }
        }

        if (nameSamePort) {
            return nameSamePort;
        } else if (typeSamePort) {
            return typeSamePort;
        }

        return nullptr;
    }

}