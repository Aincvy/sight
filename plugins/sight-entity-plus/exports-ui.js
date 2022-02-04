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

sight.entity.addOperation('templateNode', 'generate addTemplateNode() function call', function(entity){

});
