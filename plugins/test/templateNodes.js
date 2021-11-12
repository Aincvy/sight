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

    __meta_name: "Print",
    // used for context menu
    __meta_address: "test/debug",
    __meta_func: {
        generateCodeWork($) {
            print($.msg.value);
        },
    },
    // other ideas
    __meta_inputs: {
    },
    __meta_outputs: {
    },

    msg: 'String',
});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "Add",
    // used for context menu
    __meta_address: "test/math",
    __meta_func: {
        generateCodeWork($) {
        },
        onReverseActive($){
            return $.number1() + $.number2();
        }
    },
    // other ideas
    __meta_inputs: {
        number1: 'Number',
        number2: 'Number',
    },
    __meta_outputs: {
        number: 'Number',
    },

});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "EnterNode",
    // used for context menu
    __meta_address: "process",
    __meta_options: {
        processFlag: 'enter',
    },
    __meta_func: {
        generateCodeWork($$) {
            print('EnterNode generateCodeWork.');
        },

    },

    // other ideas
    __meta_inputs: {

    },
    __meta_outputs: {

    },

});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "TestTypes",
    // used for context menu
    __meta_address: "process",
    __meta_func: {
        generateCodeWork($$) {
            print('TestTypes generateCodeWork.');
        },

    },

    // other ideas
    __meta_inputs: {

    },
    __meta_outputs: {
        next: 'Process',           //
    },

    _float: 'float',
    _int: 'int',
    _double: 'double',
    _string: 'String',
    _bool: 'bool',
    _color: 'Color',

});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "TestAppendSource",
    // used for context menu
    __meta_address: "process",
    __meta_func: {
        generateCodeWork($, $$) {
            $$.insertSource(`return ${$.n.value}`);
        },
    },

    // other ideas
    __meta_inputs: {

    },
    __meta_outputs: {
        
    },

    n: 'Number',

});

print('template node file end ');