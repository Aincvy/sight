
#pragma once

#include "absl/container/flat_hash_set.h"
#include "sight_node.h"
#include <vector>

namespace sight {

    struct SightNodeGraphOutputJsonConfig {
        std::string nodeRootName = "nodes";
        std::string connectionRootName = "connections";

        bool includeRightConnections = false;
        bool includeNodeIdOnConnectionData = false;

        /**
         * @brief make a new child called `data`.
         * Put `fieldName = fieldValue` into `data`. The seq is `fields, input, output`, ignore the later same name.
         * If port is connected, then ignore the port.
         */
        bool exportData = false;

        // fields
        CaseTypes fieldNameCaseType = CaseTypes::None;
    };

    /**
     * A graph contains many nodes.
     */
    class SightNodeGraph {
    public:
        bool editing = false;
        // for ...
        const static SightAnyThingWrapper invalidAnyThingWrapper;

        SightNodeGraph();
        ~SightNodeGraph();

        /**
         * 
         * @param leftPortId
         * @param rightPortId
         * @return If create success, the connection's id.
         * -1: one of left,right is invalid. -2: they are same. -3: same kind. -4 left only can accept 1 connections
         * -5: id repeat. -6: same node.
         */
        int createConnection(uint leftPortId, uint rightPortId, uint connectionId = 0, int priority = 10);

        /**
         *
         * @param connection
         */
        void addConnection(const SightNodeConnection& connection, SightNodePort* left = nullptr, SightNodePort* right = nullptr);

        /**
         * @brief 
         * 
         * @param node memory should be managed by this SightNodeGraph.

         */
        void registerNode(SightNode* node);

        void registerNodeIds(SightNode* p);
        void unregisterNodeIds(SightNode const* p);

        /**
         *
         * @param id
         * @return
         */
        int delNode(int id);

        int delNode(SightNode const* p);

        // int delNodeByIndex(int index);
        
        /**
         *
         * @param id
         * @return
         */
        int delConnection(int id, bool removeRefs = true);

        /**
         * Find a node by id
         * @param id
         * @return The element of vector, you should not free the result. If not find, nullptr.
         */
        SightNode* findNode(uint id);

        SightNode* findEnterNode(int* status = nullptr);

        /**
         *
         * @return The element of vector, you should not free the result. If not find, nullptr.
         */
        SightNodeConnection* findConnection(int id);

        SightNodePort* findPort(uint id);
        SightNodePortHandle findPortHandle(uint id);

        bool isNode(uint id) const;
        bool isConnection(uint id) const;
        bool isPort(uint id) const;


        /**
         *
         * @param id
         * @return You should not keep the return value, just use it's asXXX function.
         */
        const SightAnyThingWrapper& findSightAnyThing(uint id) const;

        /**
         * You should call this function at init step.
         * after call this function, this->filepath will be `path`.
         * @return CODE_OK success.  
         */
        int load(std::string_view path);

        int save(SaveReason saveReason = SaveReason::User);

        /**
         * Save graph data to file.
         * @param path
         * @param set       if true, then set this->filepath to path
         * @param saveAnyway  if true, broken flag will be omitted.
         * @return  0 success, 1 graph is broken.
         */
        int saveToFile(const char* path = nullptr, bool set = false, bool saveAnyway = false, SaveReason saveReason = SaveReason::User);


        void setFilePath(const char* path);
        const char* getFilePath() const;
        std::string getFileName(std::string_view replaceExt = {}) const;

        /**
         * @brief try `addNode`, `delNode`, `fakeDeleteNode`, `loopOf` and .. functions first.
         * 
         * @return SightArray <SightNode>& 
         */
        SightArray<SightNode>& getNodes();

        /**
         * @brief try `loopOf` function first.
         * 
         * @return const SightArray <SightNode>& 
         */
        const SightArray<SightNode>& getNodes() const;
        const SightArray<SightNodeConnection>& getConnections() const;

        void loopOf(std::function<void(SightNode const*)> func) const;
        void loopOf(std::function<void(SightNode*)> func);

        void loopOf(std::function<void(SightNodeConnection const*)> func) const;
        void loopOf(std::function<void(SightNodeConnection*)> func);


        // NOT USED
        // SightArray<SightNode> & getComponents();

        SightNodeGraphExternalData& getExternalData();

        void callNodeEventsAfterLoad(v8::Isolate* isolate);

        /**
         * @brief call all node's sortConnections() function.
         * 
         */
        void sortConnections();

        /**
         * @brief Verify node, port, connection's Id 
         * For now, only check repeat.
         * @param errorStop 
         * @param onNodeError    callback function, you can modify data in this function
         * @param onPortError    callback function, you can modify data in this function
         * @param onConnectionError   callback function, you can modify data in this function
         * @return int CODE_OK, if no error. CODE_FAIL if error. 
         */
        int verifyId(bool errorStop = true, std::function<void(SightNode*)> onNodeError = nullptr, std::function<void(SightNodePort*)> onPortError = nullptr,
                     std::function<void(SightNodeConnection*)> onConnectionError = nullptr);

        /**
         * @brief Fix repeat, invalid id
         * 
         */
        void checkAndFixIdError();

        void markDirty();
        bool isDirty() const;

        void markBroken(bool broken = true, std::string_view str = {});
        bool isBroken() const;
        std::string const& getBrokenReason() const;

        SightNodeGraphSettings& getSettings();

        /**
         * @brief send parse command to js-thread.
         * 
         */
        void asyncParse() const;

        void setName(std::string_view name);
        std::string_view getName() const;

        /**
         * add id to idMap
         */
        void addPortId(SightNodePort const& port);


        void addNodeId(SightNode* node);


        /**
         * @brief need use js thread to run.
         * 
         * @param path 
         * @param overwrite 
         * @param config 
         * @return int 
         */
        int outputJson(std::string_view path, bool overwrite, SightNodeGraphOutputJsonConfig const& config) const;

        // SightArray<SightComponentContainer>& getComponentContainers();

        // create container
        SightComponentContainer* createComponentContainer();

        // remove container
        bool removeComponentContainer(SightComponentContainer* container);


        /**
         * @brief Result belongs to this graph.
         * 
         * @param original 
         * @return SightComponentContainer* 
         */
        SightComponentContainer* deepClone(SightComponentContainer* original);

        /**
         * @brief Result belongs to this graph.
         * 
         * @param original 
         * @return SightNode* 
         */
        SightNode* deepClone(SightNode* original);


        void addSaveAsJsonHistory(std::string_view path);

        std::vector<std::string> const& getSaveAsJsonHistory() const;
        inline std::string getSaveAsJsonPath() const {
            if (saveAsJsonHistory.empty()) {
                return {};
            }
            return saveAsJsonHistory.front();
        }

        int fakeDeleteNode(uint id);
        int fakeDeleteNode(SightNode* node);

        int fakeDeleteConnection(uint id);
        int fakeDeleteConnection(SightNodeConnection* connection);

        /**
         * @brief Get the Component Container object from a node or a connection
         * 
         * @param anyThingId 
         * @return SightComponentContainer* 
         */
        SightComponentContainer* getComponentContainer(uint anyThingId);

        /**
        * @brief 把 fromNode 替换成 toNode
        * 
        * @param fromNode 
        * @param toNode 
        */
        bool replaceNode(uint fromNode, uint toNode);

        bool insertNodeAtConnectionMid(uint nodeId, uint connectionId);

        bool detachNodeConnections(uint nodeId, bool onlyTitleBarPort = true);


    private:
        // save and read path.
        std::string filepath;
        std::string brokenReason;
        bool broken = false;

        // real nodes
        SightArray<SightNode> nodes{ LITTLE_ARRAY_SIZE };
        // real connections.
        SightArray<SightNodeConnection> connections{ MEDIUM_ARRAY_SIZE };

        // NOT USED
        // SightArray<SightNode> components{ LITTLE_ARRAY_SIZE };
        SightArray<SightComponentContainer> componentContainers{ LITTLE_ARRAY_SIZE };
        // key: node/port/connection id, value: the pointer of the instance.
        std::map<uint, SightAnyThingWrapper> idMap;

        SightNodeGraphExternalData externalData;
        SightNodeGraphSettings settings;

        std::vector<std::string> saveAsJsonHistory;

        /**
         * Dispose graph.
         */
        void dispose();

        /**
         * clear data
         */
        void reset();

        void rebuildIdMap();
    };

    /**
     * @brief Get the Replaceable(suitable) Port object
     * 
     * @param ports 
     * @param targetPort 
     * @param ignoreIds 
     * @return SightNodePort* a pointer which point to an element of ports
     */
    SightNodePort* getReplaceablePort(std::vector<SightNodePort> &ports, SightNodePort const& targetPort, absl::flat_hash_set<uint> const& ignoreIds);
    
}