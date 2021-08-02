// sample nodes.

let node1 = new sight.SightJsNode();
node1.nodeId = sight.nextNodeOrPortId();
node1.nodeName = "from js" + node1.nodeId;
node1.extFunction = function (){
    print("123 from js");
}

const NodePortType = new sight.NodePortType();

// print("" + NodePortType.Input);
// print("" + NodePortType.Output);
// print("" + NodePortType.Both);


let port1 = new sight.SightNodePort();
port1.id = sight.nextNodeOrPortId();
port1.portName = "input1";
port1.setKind(NodePortType.Input);
node1.addPort(port1);

let port2 = new sight.SightNodePort();
port2.id = sight.nextNodeOrPortId();
port2.portName = "output1";
port2.setKind(NodePortType.Output);
node1.addPort(port2);

// sight.addNode(node1);

// add template nodes
addTemplateNode({
    // meta info, start with __meta

    __meta_name: "HttpGetReqNode",
    // used for context menu
    __meta_address: "HttpGetReqNode",

    // other ideas
    __meta_inputs: {

    },
    __meta_outputs: {
        uid: 'Long',           //
        sign: 'String',
        timestamp: 'Long',
    },

});


print("node init over!");
