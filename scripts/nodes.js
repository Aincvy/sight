// sample nodes.

(function (){

    //let a = sight.SightJsNode;
    let node = new sight.SightJsNode();
    node.nodeName = "abc";
    //print("hello");
    //print(node.nodeName);

    let nodeString = JSON.stringify(node);
    print(nodeString);

    let testObject = {
      name: 'name',
      age: 1,
      whatever: [1,2,3]
    };
    nodeString = JSON.stringify(node);
    print(nodeString);

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

    sight.addNode(node1);

    function a(){
        print("this is in function a!");
    }

    print("node init over!");
    a();

    let names = Object.getOwnPropertyNames(node1);
    print("names: " + names.length);
    for (const name of names) {
        print(name);
    }

}());
