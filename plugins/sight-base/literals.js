addTemplateNode({
    __meta_name: "Number",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.number.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        number: {
            type: 'Number',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Integer",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.number.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        number: {
            type: 'int',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Float",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.number.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        number: {
            type: 'Number',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Double",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.number.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        number: {
            type: 'double',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Char",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.char.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        char: {
            type: 'char',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "String",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.string.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        string: {
            type: 'string',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});


addTemplateNode({
    __meta_name: "Bool",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.bool.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        bool: {
            type: 'bool',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Long",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.long.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        long: {
            type: 'long',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Color",
    __meta_address: "built-in/literal/math",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.color.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        color: {
            type: 'Color',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Vector3",
    __meta_address: "built-in/literal/math",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.vector3.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        vector3: {
            type: 'Vector3',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});


addTemplateNode({
    __meta_name: "Vector4",
    __meta_address: "built-in/literal/math",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.vector4.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        vector4: {
            type: 'Vector4',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "LargeString",
    __meta_address: "built-in/sundry",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.string.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        string: {
            type: 'LargeString',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addType('InOrOut', {
    kind: 'combo box',
    data: ['In', 'Out' ],
    defaultValue: 'In',

});

templateNodes = {};
templateNodes.VarDeclare = {};
templateNodes.VarDeclare.onInstantiate = function onInstantiate() {
    let node = this;
    // names: key: node.id, value: varName;  out: key: varName, value: node.id array
    checkTinyData(node, { names: {}, out: {} });
    let data = tinyData(node);
    // fix var name first.
    let realNames = Object.values(data.names);
    let portVarName = node.portValue('name');
    let oldName = portVarName.get();
    print(oldName);
    if (realNames.length > 0 && realNames.includes(oldName)) {
        let i = 0;
        while (true) {
            let tmp = oldName + (++i);
            if (!realNames.includes(tmp)) {
                print(tmp);
                portVarName.set(tmp);
                oldName = tmp;

                break;
            }
        }
    }

    // put in cache
    data.names[node.id] = oldName;
}

templateNodes.VarDeclare.onReload = function onReload(){
    checkTinyData(this, { names: {}, out: {} });
    let data = tinyData(this);
    let name = this.portValue('name').get();
    let portIn = this.portValue('in');
    let portOut = this.portValue('out');
    if (this.portValue('type').get().index == 0) {
        // in
        data.names[this.id] = name;
        if(portIn){
            portIn.show = true;
        }
        if(portOut){
            portOut.show = false;
        }
    } else {
        // out
        if(portIn){
            portIn.show = false;
        }
        if(portOut){
            portOut.show = true;
        }
        checkValue1(data.out, name, [], a => a.push(this.id));
        // print(name, data.out[name]);
    }
};

templateNodes.VarDeclare.onMsg = function onMsg(type, msg){
    // print(this.id, type, msg);
    if (type === 1) {
        this.portValue('name').set(msg);
    } else if (type === 2) {
        this.portValue('out').type = msg;
    } else if (type === 3) {
        this.portValue('out')?.resetType();
    }
}

templateNodes.VarDeclare.in = {};
templateNodes.VarDeclare.in.onConnect = function onConnect(node, connection){
    // print(connection.self.id, connection.target.id);
    let type = connection.target.type;
    this.type = type;

    // effect out nodes.
    let data = tinyData(node);
    let name = node.portValue('name').get();
    // print(name, data.out[name]);
    loopIf(data.out[name], element => {
        tell(element, 2, type);
    });
};


templateNodes.VarDeclare.in.onDisconnect = function onDisconnect(node, connection){
    this.resetType();

    // effect out nodes.
    let data = tinyData(node);
    let name = node.portValue('name').get();
    if (data.out[name]) {
        data.out[name].forEach(element => {
            tell(element, 3, 0);
        });
    }
};

templateNodes.VarDeclare.type = {};
templateNodes.VarDeclare.type.onValueChange = function onValueChange(node){
    let newValue = this.get();
    let portVarName = node.portValue('name');
    let data = tinyData(node);

    if (newValue.index == 0) {
        // in
        let port = node.portValue('out');
        if (port) {
            port.show = false;
            port.deleteLinks();
        }
        port = node.portValue('in');
        if(port){
            port.show = true;
        }

        // delete out
        let name = portVarName.get();
        if (data.out[name]) {
            let array = data.out[name];
            array.remove(node.id);
        }
    } else {
        // out
        let port = node.portValue('in');
        if(port){
            port.show = false;
            port.deleteLinks();
        }
        port = node.portValue('out');
        if(port){
            port.show = true;
        }

        // delete name
        delete data.names[node.id];

        // change name
        let realNames = Object.values(data.names);
        if (realNames.length > 0) {
            let name = realNames[0];
            portVarName.set(name);

            if (!data.out[name]) {
                data.out[name] = [];
            }
            data.out[name].push(node.id);
        } else {
            portVarName.set('');
        }
    }

    portVarName.errorMsg = '';
}

templateNodes.VarDeclare.name = {};
templateNodes.VarDeclare.name.onValueChange = function onValueChange(node, oldName) {
    // update var name.
    let data = tinyData(node);
    // fix var name first.
    let realNames = Object.values(data.names);
    let portVarName = node.portValue('name');
    let portType = node.portValue('type');
    let type = portType.get();
    let nowName = portVarName.get();

    let out = data.out;
    if (type.index == 0) {
        // in
        if (realNames.includes(nowName) && data.names[node.id] != nowName) {
            portVarName.errorMsg = 'repeat name';
            // this.set(oldName);
            return;
        } else {
            portVarName.errorMsg = '';
        }

        if (data.names[node.id] == nowName) {
            return;
        }

        data.names[node.id] = nowName;
        if (out) {
            //  rename key
            renameKey(out, nowName, oldName);
            let array = out[nowName];
            if (array && array.length > 0) {
                array.forEach(element => {
                    tell(element, 1, nowName);
                });
            }
        }
    } else {
        // out
        print(oldName);

        // delete old name
        if (out[oldName]) {
            let array = out[oldName];
            array.remove(node.id);
        }

        // check name valid
        if (!realNames.includes(nowName)) {
            portVarName.errorMsg = 'invalid var name';
            return;
        } else {
            portVarName.errorMsg = '';
        }

        // add new name
        checkValue1(out, nowName, [], a => a.push(node.id));
    }
}

templateNodes.VarDeclare.name.onAutoComplete = function onAutoComplete(node){
    let portType = node.portValue('type');
    if (portType.get().index == 0) {
        return undefined;
    }

    let data = tinyData(node);
    let realNames = Object.values(data.names);
    return realNames;
};

addTemplateNode({
    __meta_name: "VarDeclare",
    __meta_address: "built-in/sundry",
    __meta_func: {
        generateCodeWork($, $$) {
            $$.options.noCode = true;
            if (this.portValue('type').get().index == 1) {
                // output node, make sure the input has generated.
                let varName = this.portValue('name').get();
                let node = $$.graph.findNode(this, function (n) {
                    if (n.portValue('name')?.get() === varName) {
                        // find type=in node
                        return n.portValue('type')?.get().index == 0;
                    }
                    return false;
                });

                if (node) {
                    $$.ensureNodeGenerated(node.id);
                } else {
                    $$.error(`cannot found the node type=in varName=${varName}`, this);
                }

                return;
            }
            
            $$.__let($.name.value, $.in());
        },
        onReverseActive($, $$){
            // only type=out 
            // check does the input node has generated ?
            // 1. find the varName with type=in.
            // 2. call the node's generateCodeWork function.
            // 
            let varName = this.portValue('name').get();
            let node = $$.graph.findNode(this, function (n) {
                if (n.portValue('name')?.get() === varName) {
                    // find type=in node
                    return n.portValue('type')?.get().index == 0;
                }
                return false;
            });

            if(node) {
                let info = $$.graph.getGenerateInfo(node);
                if (info && info.hasGenerated){
                    // has generated 
                    return $.name.value;
                } else {
                    // request to generate code.
                    generateCode(node);
                    return $.name.value;
                }
            } else {
                $$.error(`cannot found the node type=in varName=${varName}`, this);
            }
        },
    },
    __meta_events: {
        onInstantiate: templateNodes.VarDeclare.onInstantiate,
        // called after graph load
        onReload: templateNodes.VarDeclare.onReload,
        onMsg: templateNodes.VarDeclare.onMsg,
    },
    __meta_inputs: {
        in: {
            type: 'Object',
            onConnect: templateNodes.VarDeclare.in.onConnect,
            onDisconnect: templateNodes.VarDeclare.in.onDisconnect,
        }
        
    },
    __meta_outputs: {
        out: {
            type: 'Object',
            show: false,
        },
    },

    type: {
        type: 'InOrOut',
        onValueChange: templateNodes.VarDeclare.type.onValueChange,
    },

    name: {
        type: 'String',
        defaultValue: 'varName',
        onValueChange: templateNodes.VarDeclare.name.onValueChange,
        onAutoComplete: templateNodes.VarDeclare.name.onAutoComplete,
    },

});

addTemplateNode({
    __meta_name: "Portal",
    __meta_address: "built-in",
    __meta_func: {
        generateCodeWork($, $$) {
            // call in portal.
            // if this is input
            // 1. active the output node.
            // 2. this portal do nothing.
            // 
            $$.options.noCode = true;
            if (this.portValue('type').get().index == 1){
                // print('this is a out portal, do nothing.');
                return;
            }

            let portalName = this.portValue('name').get();
            let node = $$.graph.findNode(this, function(n) {
                if (n.portValue('name')?.get() === portalName ){
                    // find out portal
                    return n.portValue('type')?.get().index == 1;
                }
                return false;
            });
            if(node){
                // print(`active the next portal: ${node.id}, ${node.portValue('name')?.get()}`);
                generateCode(node);
            } else {
                // print(`Do not find the out portal: ${portalName}; [Jump]`);
                $$.error(`Do not find the out portal: ${portalName}`, this);
            }
        },

        onReverseActive($, $$) {
            // do nothing
        },
    },
    __meta_events: {
        onInstantiate: templateNodes.VarDeclare.onInstantiate,
        // called after graph load
        onReload: templateNodes.VarDeclare.onReload,
        onMsg: templateNodes.VarDeclare.onMsg,
    },
    __meta_inputs: {
        // in: {
        //     type: 'Object',
        //     onConnect: templateNodes.VarDeclare.in.onConnect,
        //     onDisconnect: templateNodes.VarDeclare.in.onDisconnect,
        // }
    },
    __meta_outputs: {
        // out: {
        //     type: 'Object',
        //     show: false,
        // },
    },

    type: {
        type: 'InOrOut',
        onValueChange: templateNodes.VarDeclare.type.onValueChange,
    },

    name: {
        type: 'String',
        defaultValue: 'portalName',
        onValueChange: templateNodes.VarDeclare.name.onValueChange,
        onAutoComplete: templateNodes.VarDeclare.name.onAutoComplete,
    },

});

addTemplateNode({
    __meta_name: "CodeBlock",
    __meta_address: "built-in/sundry",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.code.value;
        },
        // do nothing.
        onReverseActive($) {
        },
    },
    __meta_events: {
        onInstantiate(){

        },
        // called after graph load
        onReload(){

        },
        onMsg(){

        },
    },
    __meta_inputs: {
    },
    __meta_outputs: {
    },

    code: {
        type: 'LargeString',
        defaultValue: '',
    },

});

addTemplateNode({
    __meta_name: "Comment",
    __meta_address: "built-in/sundry",
    __meta_func: {
        generateCodeWork($, $$) {
            return `// ${$.comment.value}`;
        },
        // do nothing.
        onReverseActive($) {
        },
    },
    __meta_events: {
        onInstantiate() {

        },
        // called after graph load
        onReload() {

        },
        onMsg() {

        },
    },
    __meta_inputs: {
    },
    __meta_outputs: {
    },

    comment: {
        type: 'LargeString',
        defaultValue: '',
    },

});
