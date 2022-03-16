// export some function for other files and plugins.
// this file will be run by js thread.
// the module object is shared with other files inside this plugin, except `exports-ui.js`
// mjs file wil do not load.

include('exports-common.js', module);
print(typeof module.exportCommon);


let globals = module.globals;

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

sight.GenerateArg$$.prototype.error = function(msg = '', node = undefined, port = undefined){
    let nodeId = detectId(node) ?? 0;
    let portId = detectId(port) ?? 0;

    this.errorReport(msg, nodeId, portId);
}

sight.GenerateArg$$.prototype.oppositeOrValue = function (portFunc) {
    if (portFunc.isConnect) {
        return portFunc();
    } else {
        return portFunc.value;
    }
}

sight.entity.generateCodeWork = function($, $$) {
    
    let selfName = this.getOtherSideValue('', 'VarDeclare', 'name');
    if(!selfName){
        $$.error('Entity need a varName!', this);
        return undefined;
    }
    for (const item of this.inputPorts) {
        // 
        let port = $[item.id];
        if (!port.isConnect) {
            continue;
        }
        
        // 
        let line = `${selfName}.${item.name} = ${port()};\n`;
        // print(line);
        $$.insertSource(line);
    }
};

sight.entity.onReverseActive = function ($, $$) {

    let selfName = this.getOtherSideValue('', 'VarDeclare', 'name');
    if (!selfName) {
        $$.error('Entity need a varName!', this);
        return undefined;
    }
    
    // print($$.reverseActivePort);
    // print(Object.keys($));
    let port = $[$$.reverseActivePort];
    if(!port){
        $$.error('Entity cannot find the reverseActivePort!', this);
        return undefined;
    }
    let name = port.name;
    if(name === ''){
        return selfName;
    }
    return `${selfName}.${port.name}`;
};
print('entity function set over!');

sight.buildTarget = function(name, func = Function(), override = true){
    if(typeof func !== 'function'){
        throw 'Func must be a function.';
    }

    sight.addBuildTarget(name, func, override);
}

sight.buildTarget('Default', function (project) {
    print('start building');
    project.parseAllGraphs();
    print('building over!');
});

addCodeTemplate(sight.DefLanguage(0, 0) , 'Empty Template', '', function () {
    
});

sight.connection.addCodeTemplate('Empty Template', '', function () {
    
});

sight.SightNodeGenerateHelper.prototype.quickVarName = function (code) {
    let varName = this.varName;
    // print(varName);
    // print(this.templateName);
    if (varName != this.templateName) {
        // var declare 
        let tmp = `let ${varName} = ` + code;
        return tmp;
    } else {
        return code;
    }
}