// sample nodes.

(function (){

    //let a = sight.SightJsNode;
    let node = new sight.SightJsNode();
    node.nodeName = "abc";
    print("hello");
    print(node.nodeName);


    let node1 = new sight.SightJsNode();
    node1.nodeName = "from js1";
    node1.nodeId = sight.nextNodeOrPortId();

    const NodePortType = new sight.NodePortType();

    let port1 = new sight.SightNodePort();
    port1.id = sight.nextNodeOrPortId();
    port1.portName = "input1";
    port1.intKind = NodePortType.Input;
    node1.addPort(port1);

    let port2 = new sight.SightNodePort();
    port2.id = sight.nextNodeOrPortId();
    port2.portName = "output1";
    port2.intKind = NodePortType.Input;
    node1.addPort(port2);

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
