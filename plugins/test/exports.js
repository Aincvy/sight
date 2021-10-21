// export some function for other files and plugins.
// this file will be export to js thread context.

let globals = module.globals;

globals.test = function () {
    print('test - test');

    return function(){};
};

if (typeof test !== 'undefined') {
    // the variable is defined
    print('ready to call test');
    test();
}

