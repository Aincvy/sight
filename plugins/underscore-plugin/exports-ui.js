let t = require('./underscore-node-f.cjs');
print(typeof t._, typeof t._.template);

module.globals.underscore = t._;
