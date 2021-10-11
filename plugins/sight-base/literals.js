addTemplateNode({
    __meta_name: "Number",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.number.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        number: {
            type: 'Number',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Integer",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.number.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        number: {
            type: 'int',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Float",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.number.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        number: {
            type: 'Number',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Double",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.number.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        number: {
            type: 'double',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Char",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.char.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        char: {
            type: 'char',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "String",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.string.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        string: {
            type: 'string',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "LargeString",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.string.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        string: {
            type: 'LargeString',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Bool",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.bool.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        bool: {
            type: 'bool',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Long",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.long.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        long: {
            type: 'long',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Color",
    __meta_address: "built-in/literal/math",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.color.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        color: {
            type: 'Color',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

addTemplateNode({
    __meta_name: "Vector3",
    __meta_address: "built-in/literal/math",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.vector3.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        vector3: {
            type: 'Vector3',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});


addTemplateNode({
    __meta_name: "Vector4",
    __meta_address: "built-in/literal/math",
    __meta_func: {
        generateCodeWork($, $$) {
            // $$.options.isPart = true;
            return $.vector4.value;       // When a output node is returned, it will be map to node's value.
        },
    },
    __meta_outputs: {
        vector4: {
            type: 'Vector4',
            showValue: true,       // Show value and the value can be changed by input component
        },
    },
});

