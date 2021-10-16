addType('ComboBox', {
    // kind must be the first prop.
    kind: 'combo box',
    data: ['A', 'B', 'C', 'D'],
    defaultValue: 'A',

});

addType('Student');
addType('Key');
addType('Glass');
addType('Word');

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "TestCombo",
    // used for context menu
    __meta_address: "test",
    __meta_func: {
        generateCodeWork($, $$) {
        },
    },

    // other ideas
    __meta_inputs: {
    },
    __meta_outputs: {
        grade: {
            type: 'ComboBox',
            showValue: true,
        },
        data: {
            type: 'Student',
            showValue: true,
        },
        key: {
            type: 'Key',
            showValue: true,
        },
        glass: {
            type: 'Glass',
            showValue: true,
        },
        word: {
            type: 'Word',
            showValue: true,
        },
    },


});

function externalOnDestroyed(node) {
    print("external onDestroyed", node.id);
    let field1 = node.portValue('field1');
    if (field1) {
        print(field1.get());
    }
}

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "EventTest",
    // used for context menu
    __meta_address: "entity",
    __meta_options: {
        enter: true,
    },
    __meta_func: {
        generateCodeWork($) {
        },
    },
    __meta_events: {
        // call after instantiate, called by ui thread.([ui thread] in below.)
        // node is the node data, not the template node.
        onInstantiate(node) {
            print("this is run by ui thread. onInstantiate");
            // print(node.id, node.name);
            print(node.templateAddress());

            checkTinyData(node, {index: 1});
            let data  = tinyData(node);
            if (typeof data === 'object' && typeof data.index === 'number') {
                data.index += 10;
                print(data.index);
                if(data.msg){
                    print(data.msg);
                } else {
                    data.msg = '' + node.id;
                }
            } else {
                print('data not ready.');
            }

            // let field1 = node.portValue('field1');
            // if (field1){
            //     print(field1.get());
            //     field1.set('fff');
            // } 

            let type = node.portValue('type');
            if(type){
                let typeValue = type.get();
                print(`typeValue: ${typeValue}`);
                // print(typeValue.index, typeValue.name);
            }
        },

        // node isn't the template node.
        // onDestroyed(node) {
        //     print("onDestroyed");
        // }
        onDestroyed: externalOnDestroyed,
    },

    field1: {
        type: 'String',
        showValue: true,
        defaultValue: 'abcd',
        // [ui thread]
        onValueChange(node) {
            // update value to auto-complete list.
            print('onValueChange', this.id, this.get());
        },
    },
    type: {
        type: 'InOrOut',

        onValueChange(node) {
            // update value to auto-complete list.
            print('InOrOut onValueChange', this.id);
            let newValue = this.get();
            if(newValue.index == 0){
                let port = node.portValue('out');
                port.show(false);
                port.deleteLinks();
                node.portValue('in').show(true);
            } else {
                let port = node.portValue('in');
                port.show(false);
                port.deleteLinks();
                node.portValue('out').show(true);
            }
        },
    },

    in: {
        type: 'Object',
        kind: 'input',
        showValue: false,
        show: true,
    },
    out: {
        type: 'Object',
        kind: 'output',
        showValue: false,
        show: false,
    },

    button: 'button',

});

function name() {
    // return {
    //     a(){

    //     }
    // };
    function abcd(){

    };
}
