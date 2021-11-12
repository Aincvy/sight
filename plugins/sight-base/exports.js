// export some function for other files and plugins.
// this file will be run by js thread.
// the module object is shared with other files inside this plugin, except `exports-ui.js`
// mjs file wil do not load.

let globals = module.globals;

if (typeof test !== 'undefined') {
    // the variable is defined
    print('ready to call test');
    test();
}

// Not supported.
// import mjs from './test.mjs'
// print(mjs.number);
// mjs.testMjs();

/**
 * 
 * @param {*} type 0 = js, 1 = ui.
 */
module.onInit = function(type){
    print(type);

}


Array.prototype.remove = function (element) {
    let index = this.indexOf(element);
    if (index > 0) {
        this.splice(index, 1);
    }
}


//  built-in functions power-up
globals.detectId = function(obj) {
    if(!obj){
        return undefined;
    }

    if (typeof obj === 'number') {
        return obj;
    } else if (typeof obj === 'object') {
        // maybe a node object
        if (typeof obj.id === 'number') {
            return obj.id;
        }
    }

    return undefined;
}

/**
 * 
 * @param {*} obj 
 * @param {*} appendToSource 
 * @returns appendToSource=true, it's a bool value. appendToSource=false, it's a string value(code).
 */
globals.reverseActive = function(obj = null, appendToSource = true){
    obj = detectId(obj);
    if(!obj){
        return false;
    }

    if(appendToSource){
        return v8ReverseActive(obj);
    }
    return v8ReverseActiveToCode(obj);
}

globals.generateCode = function (obj = null){
    obj = detectId(obj);
    if (!obj) {
        return false;
    }

    return v8GenerateCode(obj);
}

print(typeof sight.SightNodeGraphWrapper);

/**
 * 
 * @param {*} obj id or node or string(templateAddress)
 * @param {*} filter  filter function (not works with obj=id)
 */
sight.SightNodeGraphWrapper.prototype.findNode = function (obj, filter = (_ => true)){
    if(typeof obj === 'number'){
        return this.findNodeWithId(obj);
    } else if(typeof obj === 'string'){
        return this.findNodeWithFilter(obj, filter);
    } else if(typeof obj === 'object'){
        if (typeof obj.templateAddress !== 'undefined'){
            return this.findNodeWithFilter(obj.templateAddress(), filter);
        }
    }

    return undefined;
}

sight.SightNodeGraphWrapper.prototype.getGenerateInfo = function(obj = undefined){
    let id = detectId(obj);
    if(id){
        return v8GetGenerateInfo(id);
    }

    return undefined;
}
