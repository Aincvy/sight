// export some function for other files and plugins.
// this file will be export to js thread context.

let globals = module.globals;


addTemplateNode({
    __meta_name: 'TestComponent',
    __meta_address: 'test/component',

    __meta_component: {
        activeOnReverse: false,
        beforeGenerate($, $$) {
            print('before', this.nodeId);
            return `let a = 1;`;
        },
        afterGenerate($, $$) {
            print('after', this.nodeId);
            print($$.component.nodeId, $$.component.portValue('n').get());
            let msg = $$.component.portValue('msg').get();
            print(msg);
            print($$.helper.varName)
            return `network.AddNewLink(${$$.helper.varName});`;
        },
    },
    msg: 'String',
    n: 'number',
});

