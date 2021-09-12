// register all type

let studentNode = {
    __main_type: 'entity',
    name: {
        type: String,
        defaultName: 'abc',
    },
    age: Number,

    __meta_func: {
        generateCodeWork(){

        },
    },

};

function addTemplateNode(obj) {

}

addTemplateNode({
    // meta info, start with __meta

    __meta_name: "HttpGetReqNode",
    // used for context menu
    __meta_address: "sight://template/nodes/http/HttpGetReqNode",

    // other ideas
    __meta_inputs: {

    },
    __meta_outputs: {
        uid: 'Long',           //
        sign: {
            // in this ways, kind is not effect.
            // kind: Input,
            type: String,
        }
    },

    // fields
    params: {
        type: Object,
        kind: Input,
        unfold_elements: true,
    },

});

function addStmt(string){
    // xxxx
}

addStmt("if($.input1) { $.output1(); } else $.output2(); ");     // it doesn't need `;`. if there has one, it will be removed.
function test($){
    let a = `if(${$.input1}) { ${$.output1()} } else ${$.output2()}`;
}


addTemplateNode({
    // meta info, start with __meta

    __meta_name: "Add",
    // used for context menu
    __meta_address: "test/math",
    __meta_func: {
        // If only 1 param, and it called '$',
        // then the whole function body will be convert to string,unless the return value is a function.
        // If return value is a function, then the function only can have 1 param called '$'. And the function's body will convert to string.
        // If the param is called '$$', then generateCodeWork will be eval, and you can
        //   return a statement or expr or function, if you do like this, the `return` keyword will be omitted, and the other part will convert to string.
        // If has 2 param, it's '$' and '$$'.
        // In this case, you should return a function. the function's behavior will be like above.
        // Or you can return a statement or expr, if you do like this, the `return` keyword will be omitted, and the other part
        //   will be convert to string.
        // The `onReverseActive` will have similar behavior.
        // Zero param will be as 1 param.
        generateCodeWork($, $$) {
            print($.msg);
        },

        // this function will be eval, has similar behavior with generateCodeWork
        // the result function will be to string.
        onReverseActive(nodePort, $options){
            if (nodePort.name === "number") {
                $options.isPart = true;
                return function ($){
                    // In here, do not use `this` keyword to get property. Use `$` var.
                    // If you has some dynamic var, you should use `$.self.x`. Replace `x` to your property name.
                    // If you want to call some functions, `$.self` is ok.
                    return $.number1 + $.number2;
                    // If $options.isPart = true, then it will be omitted `return` keyword.
                    // We will process this to `${$.number1} + ${$.number2}`
                };
            }
        },
    },
    // __meta_code and __meta_func should be only has one.
    // __meta_code equals __meta_func.generateCodeWork
    __meta_code: function ($, $options){
        $options.isPart = true;
        return $.number1 + $.number2;
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

    __meta_name: "Number",
    // used for context menu
    __meta_address: "test/math/literal",
    __meta_func: {
        // this function will be to string(only function body.).
        generateCodeWork($, $options) {
            $options.isPart = true;
            return $.number;       // When a output node is returned, it will be map to node's value.
        },

        // If object do not has `onReverseActive` function, then it will be call generateCodeWork when this function is needed.
        // onReverseActive(nodePort, $options){
        //
        // },
    },
    // other ideas
    __meta_outputs: {
        chainOut: 'Process',
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
    __meta_address: "sight://template/nodes/process",
    __meta_options: {
        enter: true,
    },
    __meta_func: {
        generateCodeWork($, $options) {
            print('from generateCodeWork.');
        },

    },

    // other ideas
    __meta_inputs: {

    },
    __meta_outputs: {
        next: 'Process',           //

    },


});
