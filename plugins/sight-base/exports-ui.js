// this file will be ran by ui thread.
// In this file, the module is independent. Do not share with other files inside this plugin.

include('exports-common.js', module);

print(module.exportCommon);

let globals = module.globals ;

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

// node functions


// end of node functions

sight.entityFieldClass.prototype.typedValue = function(){
    let v = this.v8TypedValue();
    if (_.isString(v)) {
        return `'${v}'`;
    }
    return v;
}
