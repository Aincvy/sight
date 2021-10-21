// export some function for other files and plugins.
// this file will be run by js thread.
// the module object is shared with other files inside this plugin, except `exports-ui.js`
// mjs file wil do not load.

let globals = module.globals;

globals.test = function () {
    print('test - sight-base');
};

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

    Array.prototype.remove = function (element) {
        let index = this.indexOf(element);
        if (index > 0) {
            this.splice(index, 1);
        }
    }

}

print(typeof sight.SightNode);