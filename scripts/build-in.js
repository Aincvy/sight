
function print(obj) {
    
}

// node port type
let NodePortTypeInput = 100;
const NodePortType = {};
Object.defineProperty( NodePortType, "Input", {
    value: NodePortTypeInput++,
    writable: false,
    enumerable: true,
    configurable: true
});

Object.defineProperty( NodePortType, "Output", {
    value: NodePortTypeInput++,
    writable: false,
    enumerable: true,
    configurable: true
});

Object.defineProperty( NodePortType, "Both", {
    value: NodePortTypeInput++,
    writable: false,
    enumerable: true,
    configurable: true
});

Object.defineProperty( NodePortType, "Field", {
    value: NodePortTypeInput++,
    writable: false,
    enumerable: true,
    configurable: true
});

