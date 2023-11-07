#include "sight_ui_hierarchy.h"
#include "sight_keybindings.h"
#include "sight_log.h"
#include "sight_node.h"
#include "sight_ui.h"

#include <imgui.h>
#include <imgui_node_editor.h>
#include <string>

#include <absl/strings/substitute.h>

#include <IconsMaterialDesign.h>

namespace ed = ax::NodeEditor;

namespace sight {

    static Hierarchy* g_Hierarchy = nullptr;
    
    void updateSelectedNodeFromED() {
        auto uiStatus = currentUIStatus();
        auto& selection = uiStatus->selection;

        int selectedCount = ed::GetSelectedObjectCount();
        if (selectedCount > 0) {
            selection.selectedNodeOrLinks.clear();

            std::vector<ed::NodeId> selectedNodes;
            std::vector<ed::LinkId> selectedLinks;
            selectedNodes.resize(selectedCount);
            selectedLinks.resize(selectedCount);

            int c = ed::GetSelectedNodes(selectedNodes.data(), selectedCount);
            int d = ed::GetSelectedLinks(selectedLinks.data(), selectedCount);
            selectedNodes.resize(c);
            selectedLinks.resize(d);

            uint lastNodeId = 0;
            for (const auto& item : selectedNodes) {
                lastNodeId = static_cast<uint>(item.Get());
                selection.selectedNodeOrLinks.insert(lastNodeId);
            }
            for (const auto& item : selectedLinks) {
                selection.selectedNodeOrLinks.insert(static_cast<uint>(item.Get()));
            }

            // update node name buffer.
            if (selectedNodes.size() == 1) {
                auto n = currentGraph()->findNode(lastNodeId);
                if (n) {
                    sprintf(uiStatus->buffer.inspectorNodeName, "%s", n->nodeName.c_str());
                }
            }
        } else {
            selection.selectedNodeOrLinks.clear();
        }
    }

    void doubleClickNodeOnHierarchy(SightNode* node) {
        auto uiStatus = currentUIStatus();
        auto& selection = uiStatus->selection;
        auto nodeId = node->getNodeId();
        bool isSelected = selection.selectedNodeOrLinks.contains(nodeId);

        bool anySelect = !selection.selectedNodeOrLinks.empty();
        bool appendNode = false;
        if (anySelect) {
            if (!uiStatus->keybindings->controlKey.isKeyDown()) {
                // not control
                selection.selectedNodeOrLinks.clear();
                ed::ClearSelection();
            } else {
                appendNode = true;
            }
        }
        if (isSelected && appendNode) {
            // deselect
            selection.selectedNodeOrLinks.erase(nodeId);
            ed::DeselectNode(nodeId);
        } else {
            selection.selectedNodeOrLinks.insert(nodeId);
            sprintf(uiStatus->buffer.inspectorNodeName, "%s", node->nodeName.c_str());

            ed::SelectNode(nodeId, appendNode);
            ed::NavigateToSelection();
        }
    }

    void showHierarchyWindow() {
        auto uiStatus = currentUIStatus();
        auto const& windowLangKeys = uiStatus->languageKeys->windowNames;

        if (uiStatus->windowStatus.layoutReset) {
            ImGui::SetNextWindowPos(ImVec2(0, 20));
            ImGui::SetNextWindowSize(ImVec2(300, 285));
        }

        ImGui::Begin(windowLangKeys.hierarchy);
        // check and update node editor status
        if (!isNodeEditorReady()) {
            ImGui::End();
            return;
        }

        updateSelectedNodeFromED();

        auto graph = currentGraph();
        if (graph) {
            auto h = currentHierarchy();
            if (h->root) {
                h->root->showChildren();
            }
            
            if (h->lastDoubleClickedNode) {
                doubleClickNodeOnHierarchy(h->lastDoubleClickedNode);

                h->lastDoubleClickedNode = nullptr;
            }
        }

        ImGui::End();
    }

    void initHierarchy() {
        g_Hierarchy = new Hierarchy();
        g_Hierarchy->initEvents();
    
    }

    void disposeHierarchy() {
        if (!g_Hierarchy) {
            return;
        }

        g_Hierarchy->freeEvents();
        delete g_Hierarchy;
        g_Hierarchy = nullptr;
    
    }

    Hierarchy* currentHierarchy() {
        return g_Hierarchy;        
    }

    Hierarchy::Hierarchy()
    {
        root = this->items.add();
    }

    Hierarchy::~Hierarchy()
    {
    
    }

    void Hierarchy::rebuildCache(SightNodeGraph* graph) {
        graph->loopOf([this](SightNode* node) {
            this->addNode(node);
        });

    }


    void Hierarchy::addNode(SightNode* node) {
        // do it simple.
        const auto& templateName = node->templateNode->nodeName;
        auto p = root->findItem(templateName);
        if (!p) {
            p = this->createItem(0, templateName, root);
            p->type = HierarchyItemType::Parent;
        }

        auto i = this->createItem(node->nodeId, node->nodeName, p);
        i->type = HierarchyItemType::Node;
        i->dataPointer.node = node;

        i->applyTextFromNode();

    }


    void Hierarchy::delNode(SightNode const* node) {
        const auto& templateName = node->templateNode->nodeName;
        auto p = root->findItem(templateName);
        auto c = p->findItem(std::to_string(node->nodeId));
        if (c) {
            removeItem(c, p);
        }
    }

    HierarchyItem* Hierarchy::findOrCreate(const std::string& name, HierarchyItem* parent) {
        logError("findOrCreate not implemented");

        return nullptr;
    
    }

    HierarchyItem* Hierarchy::findOrCreate(uint id, HierarchyItem* parent) {

        logError("findOrCreate not implemented");

        return nullptr;
    }

    HierarchyItem* Hierarchy::createItem(uint id, const std::string& name, HierarchyItem* parent) {

        if (!parent) {
            parent = root;
        }

        auto p = this->items.add();
        p->name = name;
        p->text = name;
        p->anyThingId = id;
        p->parent = parent;
        parent->children.push_back(p);

        return p;
    }

    bool Hierarchy::removeItem(HierarchyItem* child, HierarchyItem* parent) {
        for (auto it = parent->children.begin(); it != parent->children.end(); it++) {
            if (*it == child) {
                parent->children.erase(it);
                this->items.remove(child);
                return true;
            }
        }
        return false;
    }

    void Hierarchy::initEvents() {

        if (!this->eventHandlerList.empty()) {
            logWarning("eventIdLists not empty");
            return;
        }

        this->eventHandlerList.push_back(SimpleEventBus::nodeAdded()->addListener([this](SightNode* node) {
            this->addNode(node);
        }));

        this->eventHandlerList.push_back(SimpleEventBus::nodeRemoved()->addListener([this](SightNode const& node) {
            this->delNode(&node);
        }));

        // graphDisposed
        this->eventHandlerList.push_back(SimpleEventBus::graphDisposed()->addListener([this](SightNodeGraph const& graph) {
            this->clearItems();
        }));

        // graphOpened
        this->eventHandlerList.push_back(SimpleEventBus::graphOpened()->addListener([this](SightNodeGraph* graph) {
            // this->rebuildCache(graph);
        }));
    }

    void Hierarchy::freeEvents() {

        for (auto& item : this->eventHandlerList) {
            item();
        }
        this->eventHandlerList.clear();
    
    }

    void Hierarchy::clearItems() {

        root = nullptr;
        this->items.clear();
    
    }

    HierarchyItem* HierarchyItem::findItem(const std::string& name) {
        // find by name from this->children
        for (auto& item : this->children) {
            if (item->name == name) {
                return item;
            }
        }
        return nullptr;

    }

    HierarchyItem* HierarchyItem::findItem(uint id) {
        // find by id from this->children
        for (auto& item : this->children) {
            if (item->anyThingId == id) {
                return item;
            }
        }
        return nullptr;    
    }

    void HierarchyItem::show() {
        if (type == HierarchyItemType::None) {
            return;
        }

        ImGui::SetNextItemOpen(false, ImGuiCond_Once);
        bool nodeOpen = false;
        switch (type) {
        case HierarchyItemType::Node:
            if (dataPointer.node->nodeName != this->name) {
                this->applyTextFromNode();
            }

            nodeOpen = ImGui::TreeNodeEx(this->text.c_str(), ImGuiTreeNodeFlags_None);
            // nodeOpen = ImGui::Selectable(this->text.c_str(), nodeOpen);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                // logDebug("double clicked !");
                // currentHierarchy()->lastDoubleClickedNodeId = dataPointer.node->nodeId;
                currentHierarchy()->lastDoubleClickedNode = dataPointer.node;
            }
            break;
        case HierarchyItemType::None:
            break;
        case HierarchyItemType::Parent:
            nodeOpen = ImGui::TreeNode(this->text.c_str());
            break;
        }

        if (nodeOpen) {
            showChildren();
            ImGui::TreePop();
        }
    }

    void HierarchyItem::showChildren() {
        for (auto child : children) {
            child->show();
        }
    }

    void HierarchyItem::reset() {
        // this function called on return to object pool.

        this->name = "";
        this->parent = nullptr;
        this->children.clear();
        this->type = HierarchyItemType::None;

        this->dataPointer.node = nullptr;
    
    }

    void HierarchyItem::applyTextFromNode() {

        this->text = absl::Substitute(ICON_MD_SQUARE "$0##$1", dataPointer.node->nodeName, dataPointer.node->nodeId);
    }

    
}