let mod = require('doT.js');
print(typeof mod, typeof mod.exports);

let func = mod.exports.template(
`{{? it.name }}
<div>Oh, I love your name, {{=it.name}}!</div>
{{?? it.age === 0}}
<div>Guess nobody named you yet!</div>
{{??}}
You are {{=it.age}} and still don't have a name?
{{?}}`);

let text = func({ "age": 31 });
print(text);

// export doT into global.
module.globals.doT = mod.exports;