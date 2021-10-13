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
    print("external onDestroyed");
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
        // [ui thread]
        onValueChange(node, newValue) {
            // update value to auto-complete list.

        },
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
