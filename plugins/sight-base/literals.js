addTemplateNode({
    __meta_name: "Number",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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
            // $$.options.isPart = true;
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

addTemplateNode({
    __meta_name: "VarDeclare",
    __meta_address: "built-in/sundry",
    __meta_func: {
        generateCodeWork($) {
            $.varName.value = $.object();
        },
        onReverseActive($, $$){
            return $.varName.value;
        },
    },
    __meta_events: {
        onInstantiate() {
            let node = this;
            // names: key: node.id, value: varName;  out: key: varName, value: node.id array
            checkTinyData(node, { names: {}, out: {}});
            let data = tinyData(node);
            // fix var name first.
            let realNames = Object.values(data.names);
            let portVarName = node.portValue('varName');
            let oldName = portVarName.get();
            print(oldName);
            if(realNames.length > 0 && realNames.includes(oldName)){
                let i = 0;
                while(true){
                    let tmp = oldName + (++i);
                    if(!realNames.includes(tmp)){
                        print(tmp);
                        portVarName.set(tmp);
                        oldName = tmp;

                        break;
                    }
                }
            }

            // put in cache
            data.names[node.id] = oldName;
        },

        // called after graph load
        onReload(){
            checkTinyData(this, { names: {}, out: {} });
            let data = tinyData(this);
            let name = this.portValue('varName').get();
            let portIn = this.portValue('in');
            let portOut = this.portValue('out');
            if(this.portValue('type').get().index == 0){
                // in
                data.names[this.id] = name;
                portIn.show(true);
                portOut.show(false);
            }  else {
                // out
                portIn.show(false);
                portOut.show(true);
                // if(!data.out[name]){
                //     data.out[name] = [];
                // }
                // data.out[name].push(this.id);
                checkValue1(data.out, name, [], a => a.push(this.id));
                // print(name, data.out[name]);
            }
        },

        onMsg(type, msg){
            // print(this.id, type, msg);
            if(type === 1){
                this.portValue('varName').set(msg);
            } else if(type === 2) {
                this.portValue('out').type(msg);
            } else if ( type === 3){
                this.portValue('out').resetType();
            }
        },
    },
    __meta_inputs: {
        in: {
            type: 'Object',
            onConnect(node, connection) {
                print(connection.self.id, connection.target.id);
                let type = connection.target.type(0);
                this.type(type);

                // effect out nodes.
                let data = tinyData(node);
                let name = node.portValue('varName').get();
                print(name, data.out[name]);
                // if(data.out[name]){
                //     data.out[name].forEach(element => {
                //         tell(element, 2, type);
                //     });
                // }
                loopIf(data.out[name], element => {
                    tell(element, 2, type);
                });
            },

            onDisconnect(node, connection) {
                this.resetType();

                // effect out nodes.
                let data = tinyData(node);
                let name = node.portValue('varName').get();
                if (data.out[name]) {
                    data.out[name].forEach(element => {
                        tell(element, 3, 0);
                    });
                }
            },
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
        onValueChange(node) {
            let newValue = this.get();
            let portVarName = node.portValue('varName');
            let data = tinyData(node);

            if (newValue.index == 0) {
                // in
                let port = node.portValue('out');
                port.show(false);
                port.deleteLinks();
                node.portValue('in').show(true);

                // delete out
                let name = portVarName.get();
                if (data.out[name]) {
                    let array = data.out[name];
                    let index = array.indexOf(node.id);
                    if(index > -1){
                        array.splice(index, 1);
                    }
                }
            } else {
                // out
                let port = node.portValue('in');
                port.show(false);
                port.deleteLinks();
                node.portValue('out').show(true);
                
                // delete name
                delete data.names[node.id];

                // change name
                let realNames = Object.values(data.names);
                if(realNames.length > 0){
                    let name = realNames[0];
                    portVarName.set(name);
                    
                    if(!data.out[name]){
                        data.out[name] = [];
                    }
                    data.out[name].push(node.id);
                } else {
                    portVarName.set('');
                }
            }

            portVarName.errorMsg('');
        },
    },

    varName: {
        type: 'String',
        defaultValue: 'varName',
        onValueChange(node, oldName) {
            // update var name.
            let data = tinyData(node);
            // fix var name first.
            let realNames = Object.values(data.names);
            let portVarName = node.portValue('varName');
            let portType = node.portValue('type');
            let type = portType.get();
            let nowName = portVarName.get();

            let out = data.out;
            if(type.index == 0){
                // in
                
                if (realNames.includes(nowName)) {
                    portVarName.errorMsg('repeat name');
                    return;
                } else {
                    portVarName.errorMsg('');
                }

                data.names[node.id] = nowName;
                if(out){
                    //  rename key
                    // delete Object.assign(out, { [nowName]: out[oldName] })[oldName];
                    renameKey(out, nowName, oldName);
                    let array = out[nowName];
                    if(array && array.length > 0){
                        array.forEach(element => {
                            tell(element, 1, nowName);
                        });
                    }
                }
            } else {
                // out
                print(1, oldName);

                // delete old name
                if(out[oldName]){
                    let array = out[oldName];
                    array.remove(node.id);
                }
                
                // check name valid
                if (!realNames.includes(nowName)) {
                    portVarName.errorMsg('invalid var name');
                    return;
                } else {
                    portVarName.errorMsg('');
                }

                // add new name
                checkValue1(out, nowName, [], a => a.push(node.id));
            }
        },

        onAutoComplete(node) {
            let portType = node.portValue('type');
            if(portType.get().index == 0){
                return undefined;
            }

            let data = tinyData(node);
            let realNames = Object.values(data.names);
            return realNames;
        }

    },

});

addTemplateNode({
    __meta_name: "Portal",
    __meta_address: "built-in",
    __meta_func: {
        generateCodeWork($) {
            $.varName.value = $.number();
        },
    },
    __meta_inputs: {
        object: 'Object',
    },
    __meta_outputs: {
    },

    varName: {
        type: 'String',
        defaultValue: 'name',
    },

});