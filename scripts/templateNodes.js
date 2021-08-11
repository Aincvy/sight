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



print('template node file end ');