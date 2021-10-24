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

function externalOnDestroyed() {
    let node = this;
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
        onInstantiate() {
            print("this is run by ui thread. onInstantiate");

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
            print('onValueChange', this.id, this.name, this.get());
            print(this.show, this.errorMsg, this.readonly, this.type);

            let t = this.get();
            if(t === '123'){
                this.show = false;
            } else if( t === '456'){
                this.errorMsg = 'this is an error msg';
            } else if( t === '777'){
                this.errorMsg = '';
            }
        },
    },
    type: {
        type: 'InOrOut',

        onValueChange(node) {
            // update value to auto-complete list.
            print('InOrOut onValueChange', this.id);
        },
    },

    number: {
        type: 'Number',

        onValueChange(node, oldValue){
            print(this.get(), oldValue);
        }
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

    button: {
        type: 'button',
        onClick(node){
            print(1);
        }
    },

});

function name() {
    // return {
    //     a(){

    //     }
    // };
    function abcd(){

    };
}


// addTemplateNode({
//     // meta info, start with __meta

//     __meta_name: "DynLoad",
//     // used for context menu
//     __meta_address: "entity",
//     __meta_options: {
//         enter: true,
//     },
//     __meta_func: {
//         generateCodeWork($$) {
//             $$.options.isPart = true;
//             print('TestTypes generateCodeWork.');
//         },

//     },

//     __meta_events: {
//         // call after instantiate, called by ui thread.([ui thread] in below.)
//         // node is the node data, not the template node.
//         onInstantiate() {
//             print('1');
//             print('2');
//         }
//     },

// });

