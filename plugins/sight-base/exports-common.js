// This file will be loaded by js thread by default.

if (sight.SightNode.prototype.hasOwnProperty('inputPorts')) {
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
