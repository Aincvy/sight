// add template nodes
print('template node file start ');

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "HttpGetReqNode",
    // used for context menu
    __meta_address: "test/http/HttpGetReqNode",

    // other ideas
    __meta_inputs: {
        placeholder: 'String',
    },
    __meta_outputs: {
        uid: 'Long',           //
        sign: 'String',
        timestamp: 'Long',
    },

});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "HttpPostReqNode",
    // used for context menu
    __meta_address: "test/http",

    // other ideas
    __meta_inputs: {
        placeholder: 'String',
    },
    __meta_outputs: {
        uid: 'Long',           //
        sign: 'String',
        timestamp: 'Long',
    },

});



addTemplateNode({
    // meta info, start with __meta

    __meta_name: "If",
    // used for context menu
    __meta_address: "test/logic",

    // other ideas
    __meta_inputs: {
        chainIn: 'Process',
        condition: 'Object',
    },
    __meta_outputs: {
        'true': 'Process',           //
        'false': 'Process',
    },

});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "Print",
    // used for context menu
    __meta_address: "test/debug",

    // other ideas
    __meta_inputs: {
        chainIn: 'Process',
        msg: 'String',
    },
    __meta_outputs: {

    },

});


print('template node file end ');