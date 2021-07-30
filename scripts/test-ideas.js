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