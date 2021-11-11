## Abc

```js
// cpp function 
function addTemplateNode(options){
  // return 0 for successed.
  return 0;
}
```

### Options

- `__meta_name`   The name of this node.
- `__meta_address` The template address of this node.  (Show in right click context meun.)
- `__meta_func`
  - `generateCodeWork` Call when code generate.
  - `onReverseActive` Call when this node is reverse active by another node.
- `__meta_events`
  - In node's event functions, `this` will be point to the node. (See [Nodes.md](./Nodes.md))
  - `onInstantiate` Call after this node is instaniated.
  - `onReload` Call after the graph is loaded over.
  - `onMsg` Call when other node tell msg to this node.
    - `function OnMsg(type, msg)`
- `__meta_inputs`  Children of `__meta_inputs` will be a input port.
- `__meta_outputs` Children of `__meta_outputs` will be a output port. 
- The left items will be ports. (Port-type can be changed, field in default.)

### __meta_func

All functions inside of `__meta_func` will be used for code generate. (Parse graph to code.)

Those functions are called by js thread, not the ui thread.

####generateCodeWork

```js
function generateCodeWork($, $$) {
	print($.msg);
}
```

This function can have 2 params, `$` and `$$`. The param's name cannot be changed.

If there only 1 param, and it called `$`,  then the whole function body will be converted to source.

And the source will be append to the final source.



If there has 2 params,  the function will be eval once, then check the result.

- If result is null or undefined or do not have a result, then do nothing. (It means this node do not generate any code.)
- If result is a function, then the function will be eval. (It will not be recursion. )
- Otherwise, the result will be the generated-code. It will be append to the final source.

- The params's order cannot be changed. `($, $$)`

`onReverseActive` is similar with `generateCodeWork`.

####onReverseActive

```js
function onReverseActive($, $$){
  
}
```



### Port

Sometimes, it's also called `NodePort`.

- `kind` 
  - `"input"`   Input ports.
  - `"output"`   Output ports.
  - `"both"`   Both Input and output ports.
  - `"field"`   Field ports.
  - This option doesn't work when the port is under `__meta_inputs`  or `__meta_outputs`.
- `type` The type of this port. (See [Types.md](./Types.md) for more details.)
- `defaultValue`  
- `showValue`   `true` or `false`.
  - When `showValue=true`, the value will be shown in node ui.
  - Field ports's value will be shown in node ui by default and cannot be changed.
- The property start with `on` word, it is a event. In port's event functions, `this` will be point to the nodePort.
- `onConnect` Call after a connection on this port is created.
- `onDisconnect`  Call before a connection on this port is deleted. 
- `onValueChange` Call after this port's value is changed.
- `onAutoComplete` Call when this port need a complete list.
- Event function signatures.
  - `function onConnect(node, connection)`
  - `function onDisconnect(node, connection)`
  - `function onValueChange(node, oldValue)`
  - `function onAutoComplete(node)`



## Example

```js
addTemplateNode({
    __meta_name: "Char",
    __meta_address: "built-in/literal/basic",
    __meta_func: {
        generateCodeWork($, $$) {
            return $.char.value;  
        },
    },
    __meta_outputs: {
        char: {
            type: 'char',
            showValue: true,   
        },
    },
});
```

