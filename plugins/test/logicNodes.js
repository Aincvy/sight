addType('ComboBox', {
    // kind must be the first prop.
    kind: 'combo box',
    data: ['A', 'B', 'C', 'D'],
    defaultValue: 'A',

});

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
    },


});