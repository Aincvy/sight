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
        onInstantiate(node) {
            // names: key=node.id, value: varName
            checkTinyData(node, { names: {}});
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
        }
    },
    __meta_inputs: {
        in: 'Object',
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
            if (newValue.index == 0) {
                // in
                let port = node.portValue('out');
                port.show(false);
                port.deleteLinks();
                node.portValue('in').show(true);
            } else {
                // out
                let port = node.portValue('in');
                port.show(false);
                port.deleteLinks();
                node.portValue('out').show(true);
                // node.portValue('varName').readonly(true);
                
                // delete name
                let data = tinyData(node);
                delete data.names[node.id];

                // change name
                let realNames = Object.values(data.names);
                if(realNames.length > 0){
                    portVarName.set(realNames[0]);
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
        onValueChange(node) {
            // update var name.
            let data = tinyData(node);
            // fix var name first.
            let realNames = Object.values(data.names);
            let portVarName = node.portValue('varName');
            let portType = node.portValue('type');
            let type = portType.get();
            let oldName = portVarName.get();
            if(oldName === data.names[node.id]){
                return;
            }

            if(type.index == 0){
                // in
                if (realNames.includes(oldName)) {
                    portVarName.errorMsg('repeat name');
                    return;
                } else {
                    portVarName.errorMsg('');
                }

                data.names[node.id] = oldName;
            } else {
                // out
                if (!realNames.includes(oldName)) {
                    portVarName.errorMsg('invalid var name');
                    return;
                } else {
                    portVarName.errorMsg('');
                }
            }
        },

        onAutoComplete(node, text) {
            let portType = node.portValue('type');
            if(portType.get().index == 0){
                return undefined;
            }

            let data = tinyData(node);
            let realNames = Object.values(data.names);
            if(!text){
                return realNames;
            }

            // this should be have a filter.
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