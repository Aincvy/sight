//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//

#include <utility>
#include <vector>
#include <atomic>
#include <algorithm>
#include <fstream>

#include "sight.h"
#include "sight_node_editor.h"
#include "sight_address.h"
#include "sight_util.h"

#include "imgui.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_canvas.h"
#include "imgui_internal.h"
#include "imgui_node_editor.h"

#include "yaml-cpp/yaml.h"

#define BACKGROUND_CONTEXT_MENU "BackgroundContextMenu"
#define LINK_CONTEXT_MENU "LinkContextMenu"
#define PIN_CONTEXT_MENU "PinContextMenu"
#define NODE_CONTEXT_MENU "NodeContextMenu"

#define CURRENT_GRAPH g_NodeEditorStatus->graph

namespace ed = ax::NodeEditor;

// node editor status
static sight::NodeEditorStatus* g_NodeEditorStatus;

static std::string emptyString("");

namespace sight {

    // define SightNodeGraph::invalidAnyThingWrapper
    const SightAnyThingWrapper SightNodeGraph::invalidAnyThingWrapper = {
            SightAnyThingType::Invalid,
            static_cast<SightNode*>(nullptr),
    };

    void ImGuiEx_BeginColumn() {
        ImGui::BeginGroup();
    }

    void ImGuiEx_NextColumn() {
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
    }

    void ImGuiEx_EndColumn() {
        ImGui::EndGroup();
    }

    namespace {
        // private members and functions
        std::atomic<uint> typeIdIncr(IntTypeNext);
        // todo this need to serialization and deserialization. And bind to project.
        absl::btree_map<std::string, uint> typeMap;
        absl::btree_map<uint, std::string> reverseTypeMap;


        void showContextMenu(const ImVec2& openPopupPosition){
            ed::Suspend();

            if (ImGui::BeginPopup(NODE_CONTEXT_MENU)) {
                if (ImGui::MenuItem("itemA")) {
                    dbg("item a");
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(PIN_CONTEXT_MENU)) {
                if (ImGui::MenuItem("itemA")) {
                    dbg("item a");
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(LINK_CONTEXT_MENU)) {
                if (ImGui::MenuItem("itemA")) {
                    dbg("item a");
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(BACKGROUND_CONTEXT_MENU)) {
                for (auto &item : g_NodeEditorStatus->templateAddressList) {
                    item.showContextMenu(openPopupPosition);
                }

                ImGui::Separator();
                if (ImGui::MenuItem("custom operations")) {
                    dbg("custom operations");
                }

                ImGui::EndPopup();
            }

            ed::Resume();
        }

        void showNodePortIcon(ImColor color, bool isConnected = false){
            auto cursorPos = ImGui::GetCursorScreenPos();
            auto drawList  = ImGui::GetWindowDrawList();
            const int iconSize = 20;
            const ImVec2 size(iconSize, iconSize);
            auto rect = ImRect(cursorPos, cursorPos + size);
            auto rect_x         = rect.Min.x;
            auto rect_y         = rect.Min.y;
            auto rect_w         = rect.Max.x - rect.Min.x;
            auto rect_h         = rect.Max.y - rect.Min.y;
            auto rect_center_x  = (rect.Min.x + rect.Max.x) * 0.5f;
            auto rect_center_y  = (rect.Min.y + rect.Max.y) * 0.5f;
            auto rect_center    = ImVec2(rect_center_x, rect_center_y);
            const auto outline_scale  = rect_w / 24.0f;
            const auto extra_segments = static_cast<int>(2 * outline_scale); // for full circle
            // auto color = ImColor(0,99,160,255);
            const auto c = rect_center;
            auto triangleStart = rect_center_x + 0.32f * rect_w;

            if (isConnected) {
                drawList->AddCircleFilled(c, 0.5f * rect_w / 2.0f, color, 12 + extra_segments);
            } else {
                drawList->AddCircle(c, 0.5f * rect_w / 2.0f, color, 12 + extra_segments,1.35f);
            }

            const auto triangleTip = triangleStart + rect_w * (0.45f - 0.32f);

            drawList->AddTriangleFilled(
                    ImVec2(ceilf(triangleTip), rect_y + rect_h * 0.5f),
                    ImVec2(triangleStart, rect_center_y + 0.15f * rect_h),
                    ImVec2(triangleStart, rect_center_y - 0.15f * rect_h),
                    color);

            ImGui::Dummy(size);
        }

        /**
         * render a node
         * @param node
         * @return
         */
        int showNode(SightNode *node) {
            if (!node) {
                return -1;
            }

            ed::BeginNode(node->nodeId);
            ImGui::Dummy(ImVec2(7,5));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0,106,113,255),"%s", node->nodeName.c_str());
            // ImGui::Separator();
            ImGui::Dummy(ImVec2(0,3));

            // fields
            for (auto &item: node->fields) {
                ImGui::Text("%s", item.portName.c_str());
                ImGui::SameLine();
                showNodePortValue(&item);
            }

            auto color = ImColor(0,99,160,255);
            // inputPorts
            ImGuiEx_BeginColumn();
            for (auto &item : node->inputPorts) {
                ed::BeginPin(item.id, ed::PinKind::Input);
                ed::PinPivotAlignment(ImVec2(0, 0.5f));
                ed::PinPivotSize(ImVec2(0, 0));
                showNodePortIcon(color, item.isConnect());
                ImGui::SameLine();
                ImGui::Text("%s", item.portName.c_str());
                ed::EndPin();
            }

            ImGuiEx_NextColumn();
            for (SightNodePort &item : node->outputPorts) {
                // test value
                if (item.type == IntTypeProcess || !item.options.showValue){

                } else {
                    char buf[LITTLE_NAME_BUF_SIZE];
                    sprintf(buf, "## %d", item.id);
                    ImGui::SetNextItemWidth(120);
                    if (item.type == IntTypeFloat) {
                        ImGui::DragFloat(buf, &item.value.f, 0.5f);
                    }
                    else {
                        ImGui::InputText(buf, item.value.string, NAME_BUF_SIZE);
                    }
                    ImGui::SameLine();
                }

                ed::BeginPin(item.id, ed::PinKind::Output);
                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                ed::PinPivotSize(ImVec2(0, 0));
                ImGui::Text("%s", item.portName.c_str());
                ImGui::SameLine();
                showNodePortIcon(color, item.isConnect());
                ed::EndPin();
            }

            ImGuiEx_EndColumn();
            ed::EndNode();
            return 0;
        }

        int showNodes(const UIStatus &uiStatus) {
            auto &io = uiStatus.io;

            ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
            ImGui::Separator();

            ed::SetCurrentEditor(g_NodeEditorStatus->context);

            // Start interaction with editor.
            ed::Begin("My Editor", ImVec2(0.0, 0.0f));

            // nodes
            for (auto &node : CURRENT_GRAPH->getNodes()) {
                showNode(&node);
            }

            // Submit Links
            for (const auto &connection : CURRENT_GRAPH->getConnections()) {
                ed::Link(connection.connectionId, connection.leftPortId(), connection.rightPortId());
            }

            //
            // 2) Handle interactions
            //

            // Handle creation action, returns true if editor want to create new object (node or link)
            if (ed::BeginCreate()) {
                ed::PinId inputPinId, outputPinId;
                if (ed::QueryNewLink(&inputPinId, &outputPinId)) {
                    // QueryNewLink returns true if editor want to create new link between pins.
                    //
                    // Link can be created only for two valid pins, it is up to you to
                    // validate if connection make sense. Editor is happy to make any.
                    //
                    // Link always goes from input to output. User may choose to drag
                    // link from output pin or input pin. This determine which pin ids
                    // are valid and which are not:
                    //   * input valid, output invalid - user started to drag new ling from input pin
                    //   * input invalid, output valid - user started to drag new ling from output pin
                    //   * input valid, output valid   - user dragged link over other pin, can be validated

                    if (inputPinId && outputPinId) // both are valid, let's accept link
                    {
                        // ed::AcceptNewItem() return true when user release mouse button.
                        if (ed::AcceptNewItem()) {
                            // Since we accepted new link, lets add one to our list of links.
                            // g_Links.push_back({ed::LinkId(nextNodeOrPortId()), inputPinId, outputPinId});
                            auto id = CURRENT_GRAPH->createConnection(static_cast<int>(inputPinId.Get()), static_cast<int>(outputPinId.Get()));
                            if (id > 0) {
                                // valid connection.

                                // Draw new link.
                                ed::Link(id, inputPinId, outputPinId);
                            } else {
                                // may show something to user.
                            }
                        }

                        // You may choose to reject connection between these nodes
                        // by calling ed::RejectNewItem(). This will allow editor to give
                        // visual feedback by changing link thickness and color.
                    }
                }
            }
            ed::EndCreate(); // Wraps up object creation action handling.

            // Handle deletion action
            if (ed::BeginDelete())
            {
                ed::NodeId deletedNodeId;
                while (ed::QueryDeletedNode(&deletedNodeId)) {
                    // ask for node delete.
                    if (ed::AcceptDeletedItem()) {
                        CURRENT_GRAPH->delNode(static_cast<int>(deletedNodeId.Get()));
                    }
                }

                // There may be many links marked for deletion, let's loop over them.
                ed::LinkId deletedLinkId;
                while (ed::QueryDeletedLink(&deletedLinkId))
                {
                    // If you agree that link can be deleted, accept deletion.
                    if (ed::AcceptDeletedItem())
                    {
                        // Then remove link from your data.
                        CURRENT_GRAPH->delConnection(static_cast<int>(deletedLinkId.Get()));
                    }

                    // You may reject link deletion by calling:
                    // ed::RejectDeletedItem();
                }
            }
            ed::EndDelete(); // Wrap up deletion action


            auto openPopupPosition = ImGui::GetMousePos();
            ed::Suspend();
            static ed::NodeId contextNodeId      = 0;
            static ed::LinkId contextLinkId      = 0;
            static ed::PinId  contextPinId       = 0;
            if (ed::ShowNodeContextMenu(&contextNodeId)) {
                ImGui::OpenPopup(NODE_CONTEXT_MENU);
            } else if (ed::ShowPinContextMenu(&contextPinId)) {
                ImGui::OpenPopup(PIN_CONTEXT_MENU);
            }
            else if (ed::ShowLinkContextMenu(&contextLinkId)) {
                ImGui::OpenPopup(LINK_CONTEXT_MENU);
            }
            else if (ed::ShowBackgroundContextMenu())
            {
                ImGui::OpenPopup(BACKGROUND_CONTEXT_MENU);
            }
            ed::Resume();

            showContextMenu(openPopupPosition);
            // End of interaction with editor.
            ed::End();

            if (uiStatus.needInit)
                ed::NavigateToContent(0.0f);

            ed::SetCurrentEditor(nullptr);

            return 0;
        }

        /**
         * Sync id, value. From src to dst.
         * @param src
         * @param dst
         */
        void syncNodePort(std::vector<SightNodePort> & src, std::vector<SightNodePort> & dst){
            for (auto &item : dst) {
                for (const auto &srcItem : src) {
                    if (srcItem.portName == item.portName) {
                        // same port.
                        item.id = srcItem.id;
                        item.value = srcItem.value;
                        break;
                    }
                }
            }
        }

    }


    int initNodeEditor() {
        g_NodeEditorStatus = new NodeEditorStatus();

        loadEntities();
        return 0;
    }

    int destroyNodeEditor() {
        disposeGraph();

        delete g_NodeEditorStatus;
        g_NodeEditorStatus = nullptr;
        return 0;
    }

    void showNodePortValue(SightNodePort *port) {
        char labelBuf[NAME_BUF_SIZE];
        sprintf(labelBuf, "## %d", port->id);

        ImGui::SetNextItemWidth(160);
        switch (port->getType()) {
            case IntTypeFloat:
                ImGui::DragFloat(labelBuf, &port->value.f, 0.5f);
                break;
            case IntTypeDouble:
                ImGui::InputDouble(labelBuf, &port->value.d);
                break;
            case IntTypeInt:
                ImGui::DragInt(labelBuf, &port->value.i);
                break;
            case IntTypeString:
                ImGui::InputText(labelBuf, port->value.string, std::size(port->value.string));
                break;
            case IntTypeBool:
                ImGui::Checkbox(labelBuf, &port->value.b);
                break;
            case IntTypeColor:
                ImGui::ColorEdit3(labelBuf, &port->value.f);
                break;
            default:
                ImGui::LabelText(labelBuf, "Unknown type of %d", port->getType());
                break;
        }

    }

    int showNodeEditorGraph(const UIStatus &uiStatus) {
        if (uiStatus.needInit) {
            ImVec2 startPos = {
                    300, 20
            };
            auto windowSize = uiStatus.io.DisplaySize - startPos;
            ImGui::SetNextWindowPos(startPos);
            ImGui::SetNextWindowSize(windowSize);
        }

        ImGui::Begin("Node Editor Graph", nullptr,
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoSavedSettings);
        showNodes(uiStatus);

        ImGui::End();

        return 0;
    }

    uint nextNodeOrPortId() {
        return CURRENT_GRAPH->nodeOrPortId++;
    }

    int addNode(SightNode *node) {

        CURRENT_GRAPH->addNode(*node);
        delete node;
        return 0;
    }

    SightNodeGraph *getCurrentGraph() {
        return CURRENT_GRAPH;
    }

    void disposeGraph(bool context) {
        // DO NOT delete entity graph.
        if (CURRENT_GRAPH && CURRENT_GRAPH != g_NodeEditorStatus->entityGraph) {
            delete CURRENT_GRAPH;
            CURRENT_GRAPH = nullptr;
        }

        if (context && g_NodeEditorStatus->context) {
            ed::DestroyEditor(g_NodeEditorStatus->context);
            g_NodeEditorStatus->context = nullptr;
        }
    }

    void changeGraph(const char *pathWithoutExt, bool loadEntityGraph) {
        dbg(pathWithoutExt);
        disposeGraph();

        ed::Config config;
        char* configFilePath = g_NodeEditorStatus->contextConfigFile;
        sprintf(configFilePath, "%s.json", pathWithoutExt);
        config.SettingsFile = configFilePath;
        g_NodeEditorStatus->context = ed::CreateEditor(&config);

        if (loadEntityGraph) {
            g_NodeEditorStatus->graph = g_NodeEditorStatus->entityGraph;
        } else {
            char buf[NAME_BUF_SIZE];
            sprintf(buf, "%s.yaml", pathWithoutExt);
            g_NodeEditorStatus->loadOrCreateGraph(buf);
            dbg("change over!");
        }
    }

    void SightNodePort::setKind(int intKind) {
        this->kind = NodePortType(intKind);
    }

    bool SightNodePort::isConnect() const {
        return !connections.empty();
    }

    int SightNodePort::connectionsSize() const {
        return static_cast<int>(connections.size());
    }


    const char *SightNodePort::getDefaultValue() const {
        return this->value.string;
    }

    uint SightNodePort::getType() const {
        return this->type;
    }


    void SightNode::addPort(const SightNodePort &port) {
        if (port.kind == NodePortType::Input) {
            this->inputPorts.push_back(port);
        } else if (port.kind == NodePortType::Output) {
            this->outputPorts.push_back(port);
        } else if (port.kind == NodePortType::Both) {
            this->inputPorts.push_back(port);

            // need change id
            this->outputPorts.push_back(port);
            this->outputPorts.back().id = nextNodeOrPortId();
        } else if (port.kind == NodePortType::Field) {
            this->fields.push_back(port);
        }
    }

    SightNode *SightNode::clone() const{
        auto p = new SightNode();
        p->copyFrom(this, false);
        return p;
    }

    SightNode *SightNode::instantiate(bool generateId) const {
        if (this->templateNode) {
            return this->templateNode->instantiate();
        }

        auto p = new SightNode();
        p->copyFrom(this, true);

        if (!generateId) {
            return p;
        }

        // generate id
        auto genIdFunc = [](std::vector<SightNodePort> & list) {
            for (auto &item : list) {
                item.id = nextNodeOrPortId();
            }
        };

        p->nodeId = nextNodeOrPortId();
        genIdFunc(p->inputPorts);
        genIdFunc(p->outputPorts);
        genIdFunc(p->fields);

        return p;
    }

    void SightNode::copyFrom(const SightNode *node, bool isTemplate) {
        this->nodeId = node->nodeId;
        this->nodeName = node->nodeName;
        if (isTemplate) {
            this->templateNode = node;
        } else {
            this->templateNode = node->templateNode;
        }

        auto copyFunc = [isTemplate](std::vector<SightNodePort> const &src, std::vector<SightNodePort> & dst){
            for (const auto &item : src) {
                dst.push_back(item);
                if (isTemplate) {
                    dst.back().templatePort = &item;
                } else {
                    dst.back().templatePort = item.templatePort;
                }
            }
        };

        copyFunc(node->inputPorts, this->inputPorts);
        copyFunc(node->outputPorts, this->outputPorts);
        copyFunc(node->fields, this->fields);

    }

    SightNodePortConnection SightNode::findConnectionByProcess() {
        SightNodeConnection* connection = nullptr;

        for (const auto &outputPort : outputPorts) {
            if (!outputPort.isConnect()) {
                continue;
            }

            if (outputPort.getType() == IntTypeProcess) {
                //
                connection = outputPort.connections.front();
                break;
            } else {
                if (connection) {
                    // has one.
                    connection = nullptr;
                    break;
                } else {
                    connection = outputPort.connections.front();
                }
            }
        }

        if (connection) {
            return SightNodePortConnection(graph, connection, this);
        }
        return {};
    }


    void SightJsNode::callFunction(const char *name) {

    }

    SightJsNode::SightJsNode() {

    }

    SightJsNode &SightJsNode::operator=(SightNode const & sightNode) {
        this->copyFrom( &sightNode, false);
        return *this;
    }

    void SightNodeConnection::removeRefs(SightNodeGraph *graph) {
        if (!graph) {
            graph = CURRENT_GRAPH;
        }
        auto leftPort = graph->findPort(leftPortId());
        auto rightPort = graph->findPort(rightPortId());

        if (leftPort) {
            auto & c = leftPort->connections;
            c.erase(std::remove(c.begin(), c.end(), this), c.end());
        }
        if (rightPort) {
            auto & c = rightPort->connections;
            c.erase(std::remove(c.begin(), c.end(), this), c.end());
        }
    }

    uint SightNodeConnection::leftPortId() const {
        return this->left;
    }

    uint SightNodeConnection::rightPortId() const {
        return this->right;
    }

    int SightNodeGraph::saveToFile(const char *path, bool set,bool saveAnyway) {
        if (set) {
            this->setFilePath(path);
        }
        if (!saveAnyway && isBroken()) {
            return 1;
        }

        // ctor yaml object
        YAML::Emitter out;
        out << YAML::BeginMap;
        // nodes
        out << YAML::Key << "nodes";
        out << YAML::Value << YAML::BeginMap;
        for (const auto &node : nodes) {
            out << YAML::Key << node.nodeId;
            out << YAML::Value;
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << node.nodeName;
            out << YAML::Key << "id" << YAML::Value << node.nodeId;

            if (node.templateNode) {
                auto address = findTemplateNode( &node);
                if (address) {
                    out << YAML::Key << "template" << YAML::Value << address->fullTemplateAddress;
                }
            }

            auto portWork = [&out](SightNodePort const& item) {
                out << YAML::Key << item.portName;
                out << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "id" << YAML::Value << item.id;
                if (!item.templatePort) {
                    // this is template port
                    out << YAML::Key << "type" << YAML::Value << item.type;
                } else {
                    // todo write values to file.
                    out << YAML::Key << "value" << YAML::Value;
                    switch (item.type) {
                        case IntTypeFloat:
                            out << item.value.f;
                            break;
                        case IntTypeDouble:
                            out << item.value.d;
                            break;
                        case IntTypeInt:
                            out << item.value.i;
                            break;
                        case IntTypeString:
                            out << item.value.string;
                            break;
                        case IntTypeBool:
                            out << item.value.b;
                            break;
                        default:
                            out << YAML::Null;
                            break;
                    }

                }
                out << YAML::EndMap;
            };

            // input and output
            out << YAML::Key << "inputs";
            out << YAML::Value << YAML::BeginMap;
            for (const auto &item : node.inputPorts) {
                portWork(item);
            }
            out << YAML::EndMap;

            out << YAML::Key << "outputs";
            out << YAML::Value << YAML::BeginMap;
            for (const auto &item : node.outputPorts) {
                portWork(item);
            }
            out << YAML::EndMap;

            out << YAML::Key << "fields";
            out << YAML::Value << YAML::BeginMap;
            for (const auto &item : node.fields) {
                portWork(item);
            }
            out << YAML::EndMap;

            //
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        // connections
        out << YAML::Key << "connections";
        out << YAML::Value << YAML::BeginMap;
        for (const auto &connection : connections) {
            out << YAML::Key << connection.connectionId << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "left" << YAML::Value << connection.leftPortId();
            out << YAML::Key << "right" << YAML::Value << connection.rightPortId();
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        // other info
        out << YAML::Key << "nodeOrPortId" << YAML::Value << this->nodeOrPortId.load();

        out << YAML::EndMap;       // end of 1st begin map

        // save file
        std::ofstream outToFile(path, std::ios::out | std::ios::trunc);
        outToFile << out.c_str() << std::endl;
        outToFile.close();
        return 0;
    }

    int SightNodeGraph::load(const char *path, bool isLoadEntity) {
        this->filepath = path;

        std::ifstream fin(path);
        if (!fin.is_open()) {
            return -1;
        }

        int status = 0;
        try {
            auto root = YAML::Load(fin);

            // nodes
            auto nodeRoot = root["nodes"];
            dbg(nodeRoot.size());
            for (const auto &nodeKeyPair : nodeRoot) {
                auto yamlNode = nodeKeyPair.second;

                SightNode sightNode;
                sightNode.nodeId = yamlNode["id"].as<int>();
                sightNode.nodeName = yamlNode["name"].as<std::string>();

                auto portWork = [](YAML::detail::iterator_value const& item, NodePortType nodePortType) -> SightNodePort {
                    auto portName = item.first.as<std::string>();
                    auto values = item.second;
                    auto id = values["id"].as<uint>();
                    auto typeNode = values["type"];
                    uint type = 0;
                    if (typeNode.IsDefined()) {
                        type = typeNode.as<uint>();
                    }

                    SightNodePort port = {
                            portName,
                            id,
                            nodePortType,
                            type,
                    };

                    auto valueNode = values["value"];
                    if (valueNode.IsDefined()) {
                        switch (type) {
                            case IntTypeFloat:
                                port.value.f = valueNode.as<float>();
                                break;
                            case IntTypeDouble:
                                port.value.d = valueNode.as<double>();
                                break;
                            case IntTypeInt:
                                port.value.i = valueNode.as<int>();
                                break;
                            case IntTypeString:
                                sprintf(port.value.string, "%s", valueNode.as<std::string>().c_str());
                                break;
                            case IntTypeBool:
                                port.value.b = valueNode.as<bool>();
                                break;
                            default:
                                port.value.i = 0;
                                break;
                        }
                    }

                    return port;
                };

                auto inputs = yamlNode["inputs"];
                for (const auto &input : inputs) {
                    sightNode.inputPorts.push_back(portWork(input, NodePortType::Input));
                }

                auto outputs = yamlNode["outputs"];
                for (const auto &output : outputs) {
                    sightNode.outputPorts.push_back(portWork(output, NodePortType::Output));
                }

                auto fields = yamlNode["fields"];
                for (const auto &field : fields) {
                    sightNode.fields.push_back(portWork(field, NodePortType::Field));
                }

                auto templateNodeAddress = yamlNode["template"].as<std::string>("");
                if (!templateNodeAddress.empty()) {
                    if (isLoadEntity) {
                        // entity should be into template
                        SightNodeTemplateAddress address = {
                                templateNodeAddress,
                                sightNode
                        };
                        addTemplateNode(address);
                    }

                    auto templateNode = g_NodeEditorStatus->findTemplateNode(templateNodeAddress.c_str());
                    if (!templateNode) {
                        dbg(templateNodeAddress, " template not found.");
                        broken = true;
                        status = 1;
                    } else {
                        // use template node init.
                        auto nodePointer = templateNode->instantiate(false);
                        nodePointer->nodeId = sightNode.nodeId;
                        syncNodePort(sightNode.inputPorts, nodePointer->inputPorts);
                        syncNodePort(sightNode.outputPorts, nodePointer->outputPorts);
                        syncNodePort(sightNode.fields, nodePointer->fields);
                        sightNode = *nodePointer;
                        delete nodePointer;
                    }
//                    sightNode.templateNode = templateNode;
                } else {
                    dbg(sightNode.nodeName, "entity do not have templateNodeAddress, jump it.");
                }

                sightNode.graph = this;
                // sightNode.fixPortTemplate();
                addNode( sightNode);
            }

            // connections
            auto connectionRoot = root["connections"];
            for (const auto &item : connectionRoot) {
                auto connectionId = item.first.as<int>();
                auto left = item.second["left"].as<int>();
                auto right = item.second["right"].as<int>();

                createConnection(left, right, connectionId);
            }

            // other info
            auto temp = root["nodeOrPortId"];
            auto tempNodeOrPortId = 20000;
            if (temp.IsDefined() && !temp.IsNull()) {
                tempNodeOrPortId = temp.as<int>(20000);
            }
            this->nodeOrPortId = tempNodeOrPortId;

            return status;
        }catch (const YAML::BadConversion & e){
            fprintf(stderr, "read file %s error!\n", path);
            fprintf(stderr, "%s\n", e.what());
            this->reset();
            return -2;    // bad file
        }

        // return 0;
    }

    void SightNodeGraph::dispose() {

    }

    const char *SightNodeGraph::getFilePath() const {
        return this->filepath.c_str();
    }

    void SightNodeGraph::setFilePath(const char* path) {
        this->filepath = path;
    }

    void SightNodeGraph::addNode(const SightNode &node) {
        auto p = this->nodes.add(node);
        idMap[node.nodeId] = {
                SightAnyThingType::Node,
                p
        };

        auto nodeFunc = [p, this](std::vector<SightNodePort> & list){
            for (auto &item : list) {
                item.node = p;
                idMap[item.id] = {
                        SightAnyThingType::Port,
                        p,
                        item.id
                };
            }
        };
        CALL_NODE_FUNC(p);

//        for (auto &item : p->inputPorts) {
//            item.node = p;
//            idMap[item.id] = {
//                    SightAnyThingType::Port,
//                    p,
//                    item.id
//            };
//        }
//        for (auto &item : p->outputPorts) {
//            item.node = p;
//            idMap[item.id] =  {
//                    SightAnyThingType::Port,
//                    p,
//                    item.id
//            };
//        }
    }

    SightNode *SightNodeGraph::findNode(int id) {
        return findSightAnyThing(id).asNode();
    }

    SightNode* SightNodeGraph::findNode(SightNodeProcessType processType, int* status /* = nullptr */){
        int count = 0;

        SightNode* p = nullptr;
        for (auto &n : nodes){
            if (findTemplateNode(&n)->options.processFlag == processType){
                count++;
                if (!p) {
                    p = &n;
                }
            }
        }

        if (count == 0) {
            SET_INT_VALUE(status, 2);
            return nullptr;
        } else if (count == 1) {
            SET_INT_VALUE(status, 0);
            return p;
        } else {
            SET_INT_VALUE(status, 1);
            return p;
        }

//        return nullptr;
    }

    SightNodeConnection *SightNodeGraph::findConnection(int id) {
        return findSightAnyThing(id).asConnection();
    }

    uint SightNodeGraph::createConnection(uint leftPortId, uint rightPortId, uint connectionId) {
//        dbg(leftPortId, rightPortId);
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

//        dbg(left, right);
        auto id = connectionId > 0 ? connectionId : nextNodeOrPortId();
        SightNodeConnection connection = {
                id,
                leftPortId,
                rightPortId,
        };
        addConnection(connection, left, right);
        return id;
    }

    void SightNodeGraph::addConnection(const SightNodeConnection &connection, SightNodePort* left, SightNodePort* right) {
        auto p = this->connections.add(connection);
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

        left->connections.push_back(p);
        right->connections.push_back(p);
    }

    const SightArray <SightNode> & SightNodeGraph::getNodes() const {
        return this->nodes;
    }

    const SightArray <SightNodeConnection> & SightNodeGraph::getConnections() const {
        return this->connections;
    }

    SightNodePort *SightNodeGraph::findPort(uint id) {
        return findSightAnyThing(id).asPort();
    }

    const SightAnyThingWrapper &SightNodeGraph::findSightAnyThing(uint id) {
        std::map<uint, SightAnyThingWrapper>::iterator iter;
        if ((iter = idMap.find(id)) == idMap.end()) {
            return invalidAnyThingWrapper;
        }
        return iter->second;
    }

    int SightNodeGraph::delNode(int id) {
        auto result = std::find_if(nodes.begin(), nodes.end(), [id](const SightNode& n){ return n.nodeId == id; });
        if (result == nodes.end()) {
            return -1;
        }

        nodes.erase(result);
        return 0;
    }

    int SightNodeGraph::delConnection(int id) {
        auto result = std::find_if(connections.begin(), connections.end(),
                                   [id](const SightNodeConnection &c) { return c.connectionId == id; });
        if (result == connections.end()) {
            return -1;
        }

        // deal refs.
        result->removeRefs();
        idMap.erase(id);

        connections.erase(result);
        dbg("delete connection" );
        return 0;
    }

    int SightNodeGraph::save() {
        return saveToFile(this->filepath.c_str(), false);
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

    SightArray<SightNode> &SightNodeGraph::getNodes() {
        return this->nodes;
    }

    bool SightNodeGraph::isBroken() const {
        return this->broken;
    }

    int NodeEditorStatus::createGraph(const char *path) {
        if (graph) {
            return -1;     // has one.
        }

        graph = new SightNodeGraph();
        graph->saveToFile(path, true);
        return 0;
    }

    int NodeEditorStatus::loadOrCreateGraph(const char *path) {
        if (graph) {
            return -1;
        }

        graph = new SightNodeGraph();
        if (graph->load(path) != 0) {
            // create one.
            graph->save();
        }

        return 0;
    }

    SightNode* NodeEditorStatus::findTemplateNode(const char* path){
        auto address = resolveAddress(path);
        auto pointer = address;
        // compare and put.
        auto* list = &g_NodeEditorStatus->templateAddressList;

        bool findElement = false;
        SightNode* result = nullptr;
        while (pointer) {
            for(auto iter = list->begin(); iter != list->end(); iter++){
                if (iter->name == pointer->part) {
                    if (!pointer->next) {
                        // the node
                        result = &iter->templateNode;
                    }

                    findElement = true;
                    list = & iter->children;
                    break;
                }
            }
            if (!findElement || result) {
                break;
            }

            pointer = pointer->next;
        }


        freeAddress(address);
        return result;
    }

    NodeEditorStatus::NodeEditorStatus() {
        initTypeMap();

    }

    NodeEditorStatus::~NodeEditorStatus() {
        // todo free memory.

    }

    void initTypeMap() {
        std::string process = "Process";
        std::string _string = "String";
        const char* number = "Number";

        typeMap[process] = IntTypeProcess;
        lowerCase(process);
        typeMap[process] = IntTypeProcess;

        typeMap[_string] = IntTypeString;
        lowerCase(_string);
        typeMap[_string] = IntTypeString;

        typeMap[number] = IntTypeFloat;
        typeMap["int"] = IntTypeInt;
        typeMap["float"] = IntTypeFloat;
        typeMap["double"] = IntTypeDouble;
        typeMap["long"] = IntTypeLong;
        typeMap["bool"] = IntTypeBool;
        typeMap["Color"] = IntTypeColor;
        typeMap["Vector3"] = IntTypeVector3;
        typeMap["Vector4"] = IntTypeVector4;

        for (auto & item: typeMap) {
            reverseTypeMap[item.second] = item.first;
        }

    }

    SightNode *SightAnyThingWrapper::asNode() const {
        if (type != SightAnyThingType::Node) {
            return nullptr;
        }
        return data.node;
    }

    SightNodeConnection* SightAnyThingWrapper::asConnection() const{
        if (type != SightAnyThingType::Connection) {
            return nullptr;
        }
        return data.connection;
    }

    SightNodePort *SightAnyThingWrapper::asPort() const{
        if (type != SightAnyThingType::Port) {
            return nullptr;
        }
        return data.portHandle.get();
    }

    SightAnyThingWrapper::SightAnyThingWrapper(SightAnyThingType type, SightNodeConnection* connection)
        :type(type), data(connection)
    {

    }

    SightAnyThingWrapper::SightAnyThingWrapper(SightAnyThingType type, SightNode *node)
        :type(type), data(node)
    {

    }

    SightAnyThingWrapper::SightAnyThingWrapper(SightAnyThingType type, SightNode *node, uint portId)
        :type(type), data(node, portId)
    {

    }


    void SightNodeTemplateAddress::showContextMenu(const ImVec2 &openPopupPosition) {
        if (children.empty()) {
            if (ImGui::MenuItem(name.c_str())) {
                //
                // dbg("it will create a new node.", this->name);
                auto node = this->templateNode.instantiate();
                addNode(node);
                ed::SetNodePosition(node->nodeId, openPopupPosition);
            }
        } else {
            if (ImGui::BeginMenu(name.c_str())) {
                for (auto &item : children) {
                    item.showContextMenu(openPopupPosition);
                }

                ImGui::EndMenu();
            }
        }
    }

    SightNodeTemplateAddress::SightNodeTemplateAddress() {

    }


    SightNodeTemplateAddress::SightNodeTemplateAddress(std::string name, const SightJsNode& templateNode)
        : name(std::move(name)), templateNode(templateNode)
    {

    }

    SightNodeTemplateAddress::~SightNodeTemplateAddress() {
//        if (templateNode) {
//            delete templateNode;
//            templateNode = nullptr;
//        }
    }

    SightNodeTemplateAddress::SightNodeTemplateAddress(std::string name, const SightNode& templateNode)
        :name(std::move(name))
    {
        this->templateNode = templateNode;
    }

    SightNodeTemplateAddress::SightNodeTemplateAddress(std::string name) : name(std::move(name)) {

    }

    SightNodePort * SightNodePortHandle::get() const {
        if (!node) {
            return nullptr;
        }

        for (auto &item : node->inputPorts) {
            if (item.id == portId) {
                return &item;
            }
        }

        for (auto &item : node->outputPorts) {
            if (item.id == portId) {
                return &item;
            }
        }

        return nullptr;
    }

    SightNodePort *SightNodePortHandle::operator->() const {
        return get();
    }

    SightNodePort &SightNodePortHandle::operator*() const {
        return *get();
    }

    int addTemplateNode(const SightNodeTemplateAddress &templateAddress) {
        if (templateAddress.name.empty()) {
            return -1;
        }

        SightJsNode templateNode = templateAddress.templateNode;
        templateNode.fullTemplateAddress = templateAddress.name;
        auto address = resolveAddress(templateAddress.name);
        auto pointer = address;
        // compare and put.
        auto* list = &g_NodeEditorStatus->templateAddressList;

        while (pointer) {
            bool findElement = false;
            if (!pointer->next) {
                // no next, this is the last element, it should be the node's name.
                list->push_back(SightNodeTemplateAddress(pointer->part, templateNode));
                break;
            }

            for(auto iter = list->begin(); iter != list->end(); iter++){
                if (iter->name == pointer->part) {
                    findElement = true;
                    list = & iter->children;
                    break;
                }
            }

            if (!findElement) {
                list->push_back(SightNodeTemplateAddress(pointer->part ));
                list = & list->back().children;
            }

            pointer = pointer->next;
        }

        freeAddress(address);
        return 0;
    }

    SightNode *generateNode(const UICreateEntity &createEntityData,SightNode *node) {
        if (!node) {
            node = new SightNode();
        }

        auto & id = g_NodeEditorStatus->entityGraph->nodeOrPortId;

        node->nodeName = createEntityData.name;
        node->nodeId = id++;         // All node and port's ids need to be initialized.

        auto c = createEntityData.first;
        auto type = getIntType(c->type, true);

        while (c) {
            // input
            SightNodePort port = {
                    c->name,
                    id++,
                    NodePortType::Input,
                    type,
                    {},
                    {},
                    std::vector<SightNodeConnection*>(),
            };
            // todo distinguish type
            sprintf(port.value.string, "%s", c->defaultValue);
            node->inputPorts.push_back(port);

            // output
            port.id = id++;
            node->outputPorts.push_back(port);

            c = c->next;
        }

        return node;
    }

    int addEntity(const UICreateEntity &createEntityData) {
        SightNode node;
        generateNode(createEntityData, &node);

        // add to templateNode
        SightNodeTemplateAddress templateAddress;
        std::string address = createEntityData.templateAddress;
        const char* prefix = "entity/";
        // check start with entity
        if (address.find(prefix) != 0) {
            address = prefix + address;
        }
        
        // check end with node name.
        if (!endsWith(address, createEntityData.name)) {
            if (!endsWith(address, "/")) {
                address += "/";
            }
            address += createEntityData.name;
        }
        templateAddress.name = address;
        templateAddress.templateNode = node;
        addTemplateNode(templateAddress);

        // convert to SightNode, and add it to entity graph.
        auto graph = g_NodeEditorStatus->entityGraph;
        // let node's template be itself.
        node.templateNode = g_NodeEditorStatus->findTemplateNode(address.c_str());
        dbg(node.nodeName, node.templateNode);
        graph->addNode(node);
        graph->save();
        return 0;
    }


    int loadEntities() {
        if (g_NodeEditorStatus->entityGraph) {
            return 1;
        }

        auto graph = g_NodeEditorStatus->entityGraph = new SightNodeGraph();
        return graph->load("./entity.yaml", true);
    }

    SightJsNode *findTemplateNode(const SightNode *node) {
        auto templateNode = node->templateNode;
        if (templateNode) {
            while (templateNode->templateNode) {
                templateNode = templateNode->templateNode;
            }
        } else {
            templateNode = node;
        }

        auto temp = const_cast<SightNode*>(templateNode);
        auto jsNode = (SightJsNode*) temp;
        return jsNode;
    }

    SightAnyThingData::SightAnyThingData() {
        this->node = nullptr;
    }

    SightAnyThingData::SightAnyThingData(SightNodeConnection *connection) {
        this->connection = connection;
    }

    SightAnyThingData::SightAnyThingData(SightNode *node) {
        this->node = node;
    }

    SightAnyThingData::SightAnyThingData(SightNode *node, uint portId) {
        this->portHandle = { node, portId };
    }


    bool SightNodePortConnection::bad() const {
        return !self || !target || !connection;
    }

    SightNodePortConnection::SightNodePortConnection(SightNodeGraph* graph,SightNodeConnection *connection, SightNode* node) {
        auto leftPointer = graph->findPort(connection->left);
        auto rightPointer = graph->findPort(connection->right);
        this->connection = connection;

        if (leftPointer->node == node) {
            self = leftPointer;
            target= rightPointer;

        } else if (rightPointer->node == node) {
            self = rightPointer;
            target = leftPointer;
        }
    }

    SightNodePortConnection::SightNodePortConnection(SightNodePort *self, SightNodePort *target,
                                                     SightNodeConnection *connection) : self(self), target(target),
                                                                                        connection(connection) {}



    uint getIntType(const std::string &str, bool addIfNotFound) {
        auto a = typeMap.find(str);
        if (a != typeMap.end()) {
            return a->second;
        } else if (addIfNotFound) {
            return addType(str);
        }

        return -1;
    }

    std::string const &getTypeName(int type) {
        auto a = reverseTypeMap.find(type);
        if (a != reverseTypeMap.end()) {
            return a->second;
        }
        return emptyString;
    }

    uint addType(const std::string &name) {
        uint a = ++ (typeIdIncr);
        typeMap[name] = a;
        reverseTypeMap[a] = name;
        return a;
    }


}