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
    __meta_code: 'if($condition) { $true } else { $false }',
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
    __meta_func: {
      generateCodeWork() {
        return function ($){
            print($.msg);
            let a = `print(${$.msg})`;
        }
      },
    },
    // other ideas
    __meta_inputs: {
        chainIn: 'Process',
        msg: 'String',
    },
    __meta_outputs: {

    },

});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "VarDeclare",
    // used for context menu
    __meta_address: "test/var",
    __meta_func: {
        // this function will be to string(only function body.).
        generateCodeWork($) {
            $.varName.value = $.number();
        },

        // If object do not has `onReverseActive` function, then it will be call generateCodeWork when this function is needed.
        // onReverseActive(nodePort, $options){
        //
        // },
    },
    __meta_inputs: {
        chainIn: 'Process',
        number: 'Number',
    },
    __meta_outputs: {
        chainOut: 'Process',
    },

    varName: {
        type: 'String',
        defaultValue: 'varName',
    },

});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "Add",
    // used for context menu
    __meta_address: "test/math",
    __meta_func($, $$){
        print('Add generateCodeWork.');
        return $.number1() + $.number2();
    },
    // other ideas
    __meta_inputs: {
        chainIn: 'Process',
        number1: 'Number',
        number2: 'Number',
    },
    __meta_outputs: {
        chainOut: 'Process',
        number: 'Number',
    },

});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "Number",
    // used for context menu
    __meta_address: "test/math/literal",
    __meta_func: {
        // this function will be to string(only function body.).
        generateCodeWork($$) {
            $$.options.isPart = true;
            return 1;       // When a output node is returned, it will be map to node's value.
            //print('Number generateCodeWork.');
        },

        // If object do not has `onReverseActive` function, then it will be call generateCodeWork when this function is needed.
        // onReverseActive(nodePort, $options){
        //
        // },
    },
    // other ideas
    __meta_outputs: {
        number: {
            type: 'Number',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },

});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "EnterNode",
    // used for context menu
    __meta_address: "process",
    __meta_options: {
        enter: true,
    },
    __meta_func: {
        generateCodeWork($$) {
            $$.options.isPart = true;
            print('EnterNode generateCodeWork.');
        },

    },

    // other ideas
    __meta_inputs: {

    },
    __meta_outputs: {
        next: 'Process',           //

    },

});

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "TestTypes",
    // used for context menu
    __meta_address: "process",
    __meta_options: {
        enter: true,
    },
    __meta_func: {
        generateCodeWork($$) {
            $$.options.isPart = true;
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


print('template node file end ');