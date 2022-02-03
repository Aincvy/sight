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



