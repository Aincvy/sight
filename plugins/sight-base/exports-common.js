// This file will be loaded by js thread by default.

// if (sight.SightNode.prototype.hasOwnProperty('inputPorts')) {
//     return;
// }
if (typeof module.reloading === 'boolean' && module.reloading){
    print("reloading ... jump once!");
    return; 
}

Object.defineProperty(sight.SightNode.prototype, 'abc', {
    get(){
        return this.id;
    }, 
    set(v) {
        throw '[Readonly] Cannot change this property to ' + v;
    }
});


Object.defineProperty(sight.SightNode.prototype, 'inputPorts', {
    get() {
        return this.getPorts(sight.NodePortType.Input);
    }
});

Object.defineProperty(sight.SightNode.prototype, 'outputPorts', {
    get() {
        return this.getPorts(sight.NodePortType.Output);
    }
});

Object.defineProperty(sight.SightNode.prototype, 'fields', {
    get() {
        return this.getPorts(sight.NodePortType.Field);
    }
});

Object.defineProperty(sight.SightNode.prototype, 'bothPorts', {
    get() {
        return this.getPorts(sight.NodePortType.Both);
    }
});

Object.defineProperty(sight.SightNode.prototype, 'ports', {
    get() {
        let p = this.getPorts(sight.NodePortType.Both);
        return p.concat(this.fields);
    }
});



//  Others

Array.prototype.remove = function (element) {
    let index = this.indexOf(element);
    if (index > 0) {
        this.splice(index, 1);
    }
}

let globals = module.globals;
globals.renameKey = function (obj, nowName, oldName) {
    delete Object.assign(obj, { [nowName]: obj[oldName] })[oldName];
}

globals.loopIf = function (array, f) {
    if (array) {
        array.forEach(f);
    }
}

// 
globals.require = function require(path, module = null, extraData = null){
    if(!module) {
        module = { exports: {}, globals: {} };
    }

    let returns = v8Include(path, module, 1);
    if (extraData) {
        extraData.returns = returns;
    }
    return module.exports;
};

globals.execute = function execute(path = ''){
    if (!path || typeof path !== 'string') {
        return;
    }

    if (path.endsWith(".js")) {
        // js file 
        return v8Include(path, null, 1);
    }
}

module.exportCommon = true;