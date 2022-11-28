
module.globals.camelize = function camelize(str) {
    return str.replace(/(?:^\w|[A-Z]|\b\w)/g, function (word, index) {
        return index === 0 ? word.toLowerCase() : word.toUpperCase();
    }).replace(/\s+/g, '');
}

/**
 * SymbolInfo =>  SYMBOL_INFO
 * @param {*} str 
 */
module.globals.upperConstantName = function upperConstantName(str) {
    if(typeof str !== 'string') {
        return '';
    }

    // from https://stackoverflow.com/a/40796345/11226492
    const isUpperCase = (string) => /^[A-Z]*$/.test(string)

    let r = '';
    for (let index = 0; index < str.length; index++) {
        const element = str.charAt(index);
        if(index != 0) {
            if(isUpperCase(element)) {
                r += "_";
            }
        } 

        r += element.toUpperCase();
    }

    return r;
}

module.globals.lowerHyphenName = function lowerHyphenName(str){
    if(typeof str !== 'string') {
        return '';
    }

    const isUpperCase = (string) => /^[A-Z]*$/.test(string)
    let r = '';
    for (let index = 0; index < str.length; index++) {
        const element = str.charAt(index);
        if(index != 0) {
            if(isUpperCase(element)) {
                r += "-";
            }
        } 

        r += element.toLowerCase();
    }

    return r;
}

print(module.globals.upperConstantName("SymbolInfo")) 
print(module.globals.lowerHyphenName("SymbolInfo")) 

