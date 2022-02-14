function imguiCreate(entity){
    let templateText = `
`
    let func = underscore.template(templateText);
    let result = func({ entity: entity, pkgName: entity.packageName });
    return result;
}

function imguiList(entity) {
    let templateText = `
`
    let func = underscore.template(templateText);
    let result = func({ entity: entity, pkgName: entity.packageName });
    return result;
}

function imguiInfo(entity) {
    let templateText = `
`
    let func = underscore.template(templateText);
    let result = func({ entity: entity, pkgName: entity.packageName });
    return result;
}

function imgui(entity) {
    let result = '';
    result += imguiCreate(entity);
    result += imguiList(entity);
    result += imguiInfo(entity);
    return result;
}

sight.entity.addOperation('imgui-test', 'desc', function(entity){
    // print(typeof underscore, typeof underscore.template);

    let templateText = `
<% if(pkgName) {%> package <%= pkgName%>; <%}%>
 public class <%=entity.simpleName%> {
     <% for (const field of entity.fields){ %>
         private <%=field.type %> <%= field.name %> <% if(field.defaultValue){ %>= <%= field.defaultValue %> <% } %>;
     <% } %>
}
`
    let func = underscore.template(templateText);
    let result = func({ entity: entity, pkgName: entity.packageName });
    return result;
});


sight.entity.addOperation('imgui-create', 'create window', imguiCreate);
sight.entity.addOperation('imgui-list', 'list window', imguiList);
sight.entity.addOperation('imgui-info', 'info window', imguiInfo);
sight.entity.addOperation('imgui', 'create, list, info windows.', imgui);

// addTemplateNode({
//     __meta_name: 'name',
//     __meta_address: 'address',
//     __meta_inputs: {

//     },
//     __meta_outputs: {

//     }
// });

sight.entity.addOperation('templateNode', 'generate addTemplateNode() function call', function(entity){
    let portTemplateText = 
`<%= port.name%> : {
    type: '<%= port.type%>'
    <% if(port.defaultValue){ %>defaultValue: <%=port.typedValue()%>, <% } %>
    <% let options = port.options.portOptions; %>
    <% if(!options.show){ %>show: <%=options.show%>, <% } %>
    <% if(options.showValue){ %>showValue: <%=options.showValue%>, <% } %>
    <% if(options.readonly){ %>readonly: <%=options.readonly%>, <% } %>
},`;

    let portListTemplateText = 
`<% for(const field of entity.fields) { %>
    <% let portType = field.options.portType; %>
    <% if ((portType == sight.NodePortType.Both && filterPortType != sight.NodePortType.Field) || portType == filterPortType) { %>
    <%=f.port(field)%>
    <% } %>
<%}%>
`;

    let nodeTemplateText = 
`addTemplateNode({
    __meta_name: '<%=entity.simpleName%>',
    __meta_address: '<%=entity.templateAddress%>',
    __meta_func: {
        generateCodeWork($, $$) {
        },
        onReverseActive($, $$){
        }
    },
    __meta_inputs: {
        <%=f.portList(entity,sight.NodePortType.Input)%>
    },
    __meta_outputs: {
        <%=f.portList(entity,sight.NodePortType.Output)%>
    },
    <%=f.portList(entity,sight.NodePortType.Field)%>
});`;

    // print(sight.NodePortType.Input, sight.NodePortType.Output, sight.NodePortType.Field, sight.NodePortType.Both );
    let nodeTemplateFunc = _.template(nodeTemplateText);
    let portListTemplateFunc = _.template(portListTemplateText);
    let portTemplateFunc = _.template(portTemplateText);

    let result = nodeTemplateFunc({entity: entity, f: {
        portList: (entity, type) => portListTemplateFunc({ entity: entity,filterPortType: type, f: {
            port: field => portTemplateFunc({ port: field}),
        }}),
    }});
    return result;
});
