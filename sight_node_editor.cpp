//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//

#include <vector>
#include <atomic>
#include <algorithm>
#include <fstream>

#include "sight.h"
#include "sight_node_editor.h"

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

#define CURRENT_GRAPH g_NodeEditorStatus.graph

namespace ed = ax::NodeEditor;

//
static std::atomic<int> nodeOrPortId(10000);
// node editor status
static sight::NodeEditorStatus g_NodeEditorStatus;


namespace sight {

    // define SightNodeGraph::invalidAnyThingWrapper
    const SightAnyThingWrapper SightNodeGraph::invalidAnyThingWrapper = {
            SightAnyThingType::Invalid,
            nullptr
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
                if (ImGui::MenuItem("itemA")) {
                    dbg("item a");
                }
                if (ImGui::BeginMenu("class a")) {
                    if (ImGui::MenuItem("1")) {
                        dbg("click `class a/1`");
                    }

                    if (ImGui::MenuItem("2")) {
                        dbg("click `class a/2`");
                    }

                    ImGui::EndMenu();
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

            drawList->AddCircleFilled(c, 0.5f * rect_w / 2.0f, color, 12 + extra_segments);

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
        int showNode(const SightNode *node) {
            if (!node) {
                return -1;
            }

            ed::BeginNode(node->nodeId);
            ImGui::Dummy(ImVec2(7,5));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0,106,113,255),"%s", node->nodeName.c_str());
            // ImGui::Separator();
            ImGui::Dummy(ImVec2(0,3));

            auto color = ImColor(0,99,160,255);
            // inputPorts
            ImGuiEx_BeginColumn();
            for (const auto &item : node->inputPorts) {
                ed::BeginPin(item.id, ed::PinKind::Input);
                ed::PinPivotAlignment(ImVec2(0, 0.5f));
                ed::PinPivotSize(ImVec2(0, 0));
                showNodePortIcon(color);
                ImGui::SameLine();
                ImGui::Text("%s", item.portName.c_str());
                ed::EndPin();
            }

            ImGuiEx_NextColumn();
            for (const auto &item : node->outputPorts) {
                ed::BeginPin(item.id, ed::PinKind::Output);
                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                ed::PinPivotSize(ImVec2(0, 0));
                ImGui::Text("%s", item.portName.c_str());
                ImGui::SameLine();
                showNodePortIcon(color);
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

            ed::SetCurrentEditor(g_NodeEditorStatus.context);

            // Start interaction with editor.
            ed::Begin("My Editor", ImVec2(0.0, 0.0f));

            // nodes
            for (const auto &node : CURRENT_GRAPH->getNodes()) {
                showNode(&node);
            }

            // Submit Links
//            for (auto &linkInfo : g_Links)
//                ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

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

    }


    int initNodeEditor() {
        ed::Config config;
        config.SettingsFile = "Simple.json";
        g_NodeEditorStatus.context = ed::CreateEditor(&config);
        g_NodeEditorStatus.loadOrCreateGraph("./simple.yaml");

        return 0;
    }

    int destroyNodeEditor() {
        ed::DestroyEditor(g_NodeEditorStatus.context);
        return 0;
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

    int nextNodeOrPortId() {
        return nodeOrPortId++;
    }

    int addNode(SightNode *node) {
        //g_Nodes.push_back(node);
        CURRENT_GRAPH->addNode(*node);
        delete node;
        return 0;
    }

    SightNodeGraph *getCurrentGraph() {
        return CURRENT_GRAPH;
    }

    void loadGraph(const char *path) {
        disposeGraph();

        dbg(path);
        auto graph = new SightNodeGraph();
        graph->load(path);
        CURRENT_GRAPH = graph;
    }

    void disposeGraph() {
        if (CURRENT_GRAPH) {
            delete CURRENT_GRAPH;
            CURRENT_GRAPH = nullptr;
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


    void SightNode::addPort(SightNodePort &port) {
        dbg(port.id,  port.portName);

        if (port.kind == NodePortType::Input) {
            this->inputPorts.push_back(port);
        } else if (port.kind == NodePortType::Output) {
            this->outputPorts.push_back(port);
        } else if (port.kind == NodePortType::Both) {
            // todo
        }
    }

    SightNode *SightNode::clone() const{
        auto p = new SightNode();
        p->copyFrom(this);
        return p;
    }

    SightNode *SightNode::instantiate() {
        if (this->templateNode) {
            return this->templateNode->instantiate();
        }

        auto p = new SightNode();
        p->templateNode = this;
        p->copyFrom(this);

        // generate id
        p->nodeId = nextNodeOrPortId();
        for (auto &item : p->inputPorts) {
            item.id = nextNodeOrPortId();
        }
        for (auto &item : p->outputPorts) {
            item.id = nextNodeOrPortId();
        }

        return p;
    }

    void SightNode::copyFrom(const SightNode *node) {
        this->nodeId = node->nodeId;
        this->nodeName = node->nodeName;
        this->inputPorts.assign(node->inputPorts.begin(), node->inputPorts.end());
        this->outputPorts.assign(node->outputPorts.begin(), node->outputPorts.end());
    }


    void SightJsNode::callFunction(const char *name) {

    }

    void SightNodeConnection::removeRefs() {
        if (left) {
            auto & c = left->connections;
            c.erase(std::remove(c.begin(), c.end(), this), c.end());
        }
        if (right) {
            auto & c = right->connections;
            c.erase(std::remove(c.begin(), c.end(), this), c.end());
        }
    }

    int SightNodeConnection::leftPortId() const {
        return this->left->id;
    }

    int SightNodeConnection::rightPortId() const {
        return this->right->id;
    }

    void SightNodeConnection::addRefs() {
        this->left->connections.push_back(this);
        this->right->connections.push_back(this);
    }

    int SightNodeGraph::saveToFile(const char *path, bool set) {
        if (set) {
            this->setFilePath(path);
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
            out << YAML::Key << "template" << YAML::Value << "not impl";
            out << YAML::Key << "fields" << YAML::Value << "not impl";

            // input and output
            out << YAML::Key << "inputs";
            out << YAML::Value << YAML::BeginMap;
            for (const auto &item : node.inputPorts) {
                out << YAML::Key << item.portName;
                out << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "id" << YAML::Value << item.id;
                out << YAML::EndMap;
            }
            out << YAML::EndMap;
            out << YAML::Key << "outputs";
            out << YAML::Value << YAML::BeginMap;
            for (const auto &item : node.outputPorts) {
                out << YAML::Key << item.portName;
                out << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "id" << YAML::Value << item.id;
                out << YAML::EndMap;
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
        out << YAML::Key << "nodeOrPortId" << YAML::Value << nodeOrPortId.load();

        out << YAML::EndMap;       // end of 1st begin map

        // save file
        std::ofstream outToFile(path, std::ios::out | std::ios::trunc);
        outToFile << out.c_str() << std::endl;
        outToFile.close();
        return 0;
    }

    int SightNodeGraph::load(const char *path) {
        this->filepath = path;

        std::ifstream fin(path);
        if (!fin.is_open()) {
            return -1;
        }

        try {
            auto root = YAML::Load(fin);

            // nodes
            auto nodeRoot = root["nodes"];
            dbg(nodeRoot.size());
            for (const auto &nodeKeyPair : nodeRoot) {
                auto node = nodeKeyPair.second;
                SightNode sightNode;
                sightNode.nodeId = node["id"].as<int>();
                sightNode.nodeName = node["name"].as<std::string>();
                auto templateNodeAddress = node["template"].as<std::string>();

                auto inputs = node["inputs"];
                for (const auto &input : inputs) {
                    auto portName = input.first.as<std::string>();
                    auto values = input.second;
                    auto id = values["id"].as<int>();

                    sightNode.inputPorts.push_back({
                                                           portName,
                                                           id,
                                                           NodePortType::Input
                                                   });
                }

                auto outputs = node["outputs"];
                for (const auto &output : outputs) {
                    auto portName = output.first.as<std::string>();
                    auto values = output.second;
                    auto id = values["id"].as<int>();

                    sightNode.outputPorts.push_back({
                                                            portName,
                                                            id,
                                                            NodePortType::Output
                                                    });
                }

                addNode(sightNode);
            }

            // connections
            auto connectionRoot = root["connections"];
            dbg(connectionRoot.size());
            for (const auto &item : connectionRoot) {
                auto connectionId = item.first.as<int>();
                auto left = item.second["left"].as<int>();
                auto right = item.second["right"].as<int>();

                createConnection(left, right, connectionId);
            }

            // other info
            auto temp = root["nodeOrPortId"];
            if (temp.IsDefined() && !temp.IsNull()) {
                dbg(root["nodeOrPortId"].as<int>());
                nodeOrPortId.store(temp.as<int>(20000));
            } else {
                nodeOrPortId.store(20000);
            }

            return 0;
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
        this->nodes.push_back(node);

        // do ids.
        auto & ref = this->nodes.back();
        idMap[node.nodeId] = {
                SightAnyThingType::Node,
                &ref
        };

        for (auto &item : ref.inputPorts) {
            idMap[item.id] = {
                    SightAnyThingType::Port,
                    &item
            };
        }
        for (auto &item : ref.outputPorts) {
            idMap[item.id] =  {
                    SightAnyThingType::Port,
                    &item
            };
        }
    }

    SightNode *SightNodeGraph::findNode(int id) {
//        auto result = std::find_if(nodes.begin(), nodes.end(), [id](const SightNode& n){ return n.nodeId == id; });
//        if (result == nodes.end()) {
//            return nullptr;
//        }
//        return &(*result);
        return findSightAnyThing(id).asNode();
    }

    SightNodeConnection *SightNodeGraph::findConnection(int id) {
//        auto result = std::find_if(connections.begin(), connections.end(),
//                     [id](const SightNodeConnection &c) { return c.connectionId == id; });
//        if (result == connections.end()) {
//            return nullptr;
//        }
//        return &(*result);
        return findSightAnyThing(id).asConnection();
    }

    int SightNodeGraph::createConnection(int leftPortId, int rightPortId, int connectionId) {
        dbg("try create connection.");
        dbg(leftPortId, rightPortId);
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

        dbg(left, right);
        auto id = connectionId > 0 ? connectionId : nextNodeOrPortId();
        SightNodeConnection connection = {
                id,
                left,
                right
        };
        addConnection(connection);
        return id;
    }

    void SightNodeGraph::addConnection(const SightNodeConnection &connection) {
        this->connections.push_back(connection);

        // do id
        auto & ref = this->connections.back();
        idMap[connection.connectionId] = {
                SightAnyThingType::Connection,
                &ref,
        };

        // refs
        ref.addRefs();
    }

    const std::vector<SightNode> &SightNodeGraph::getNodes() const {
        return this->nodes;
    }

    const std::vector<SightNodeConnection> &SightNodeGraph::getConnections() const {
        return this->connections;
    }

    SightNodePort *SightNodeGraph::findPort(int id) {
        return findSightAnyThing(id).asPort();
    }

    const SightAnyThingWrapper &SightNodeGraph::findSightAnyThing(int id) {
        std::map<int, SightAnyThingWrapper>::iterator iter;
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
        dbg("delete connection");
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
            nodeOrPortId.store(10000);
            graph->save();
        }

        return 0;
    }

    SightNode *SightAnyThingWrapper::asNode() const {
        if (type != SightAnyThingType::Node) {
            return nullptr;
        }
        return (SightNode*) pointer;
    }

    SightNodeConnection* SightAnyThingWrapper::asConnection() const{
        if (type != SightAnyThingType::Connection) {
            return nullptr;
        }
        return (SightNodeConnection*) pointer;
    }

    SightNodePort *SightAnyThingWrapper::asPort() const{
        if (type != SightAnyThingType::Port) {
            return nullptr;
        }
        return (SightNodePort*) pointer;
    }


}