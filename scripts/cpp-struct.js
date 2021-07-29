// used for syntax-highlight

sight = {};
sight.SightJsNode = class {
    nodeName;
    nodeId;

    addPort(port) {

    }
}

sight.NodePortType = class {
    Input = 100;
    Output = 101;
    Both = 102;
    Field = 103;
}

sight.SightNodePort = class {
    portName;
    id;

    setKind(intKind){

    };
};

sight.nextNodeOrPortId = function (){
    return 0;
}

sight.addNode = function (node){

}