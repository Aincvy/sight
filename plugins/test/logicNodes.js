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