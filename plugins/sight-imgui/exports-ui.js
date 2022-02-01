sight.entity.addOperation('imgui-test', 'desc', function(entity){
    print(typeof doT, typeof doT.template);

    let templateText = 
`
public class {{=it.name}} {
    {{ for (const field of it.fields){ }}
        {{=field.name}}
    {{ } }}
}
`;

// {{ for (const field in it.fields){ }}
//     private { {=field.type } } { {=field.name } } { {?field.defaultValue } }= { {=field.defaultValue } } { {?} };
//     { { } }
// }
    let func = doT.template(templateText);
    print(func.toString());
    return func(entity);
});

