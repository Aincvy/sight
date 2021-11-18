// this file will be ran by ui thread.
// In this file, the module is independent. Do not share with other files inside this plugin.

print('ui thread.');
include('exports-common.js');

let globals = module.globals ;

Array.prototype.remove = function (element) {
    let index = this.indexOf(element);
    if (index >= 0) {
        this.splice(index, 1);
    }
}

globals.checkValue = function (obj, key, value = {}){
    if (!obj[key]) {
        print(`set key ${key}`);
        obj[key] = value;
    }
}

globals.checkValue1 = function (obj, key, value, f ) {
    this.checkValue(obj, key, value);
    return f(obj[key]);
}

globals.renameKey = function(obj, nowName, oldName){
    delete Object.assign(obj, { [nowName]: obj[oldName] })[oldName];
}

globals.loopIf = function(array, f){
    if(array){
        array.forEach(f);
    }
}

// node functions


// end of node functions

